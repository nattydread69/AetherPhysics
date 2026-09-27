// SPDX-License-Identifier: LGPL-3.0-or-later
//
// AetherPhysics
// Copyright (C) 2025 Dr. Nathanael John Inkson
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "AetherDensityVisualizer.h"
#include "GPUCapabilities.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

AetherDensityVisualizer::
AetherDensityVisualizer(lightGraphics::lightVulkanGraphics& app)
	: AetherPhysicsModel(app, "Aether Density Visualizer")
{
	GPUCapabilities::GPUInfo gpuInfo = GPUCapabilities::detectGPU();
	Nx = gpuInfo.latticeResolution;
}

AetherDensityVisualizer::~AetherDensityVisualizer()
{
}

void AetherDensityVisualizer::initialize()
{
	std::cout << "Initializing Aether Density Visualizer..." << std::endl;

	createLattice();
	createFogVolume();

	launchNewRay();
	createRaySegments();

	std::cout << "Created density field with " << sphereObjectIndices.size() << " spheres" << std::endl;
}

void AetherDensityVisualizer::update(float deltaTime)
{
	time += deltaTime;
	if (displayMode == DisplayMode::PARTICLES) {
		updateMotion(deltaTime);
	}
	updateLightRay(deltaTime);
}

void AetherDensityVisualizer::cleanup()
{
	// Graphics objects are cleared by PhysicsApp when switching models, but the
	// fog volume is a separate resource that clearObjects() doesn't touch
	destroyFogVolume();
}

void AetherDensityVisualizer::setDisplayMode(DisplayMode mode)
{
	if (mode == displayMode) return;
	displayMode = mode;

	const bool fog = (mode == DisplayMode::FOG);
	setParticlesVisible(!fog);
	if (fogVolume.isValid()) {
		if (fog) {
			app_.drawVolume(fogVolume, {lightGraphics::RenderLayer::Volume, 0.0f});
		} else {
			app_.hideVolume(fogVolume);
		}
	}

	std::cout << "Density display: " << (fog ? "fog" : "particles") << std::endl;
}

void AetherDensityVisualizer::toggleDisplayMode()
{
	setDisplayMode(displayMode == DisplayMode::PARTICLES ? DisplayMode::FOG : DisplayMode::PARTICLES);
}

void AetherDensityVisualizer::setParticlesVisible(bool visible)
{
	const glm::vec4 color = visible ? particleColor : glm::vec4(0.0f);
	for (int index : sphereObjectIndices) {
		app_.setObjectColor(index, color);
	}
}

void AetherDensityVisualizer::createFogVolume()
{
	const int n = fogResolution;
	std::vector<float> field(static_cast<size_t>(n) * n * n);
	for (int z = 0; z < n; ++z) {
		for (int y = 0; y < n; ++y) {
			for (int x = 0; x < n; ++x) {
				const glm::vec3 pos = (glm::vec3(x, y, z) / static_cast<float>(n - 1) * 2.0f - 1.0f) * fogHalfRange;
				field[x + n * (y + n * z)] = (densityField(pos) - rho0) / delta_rho;
			}
		}
	}

	lightGraphics::Texture3DDescription textureDescription;
	textureDescription.width = n;
	textureDescription.height = n;
	textureDescription.depth = n;
	textureDescription.format = lightGraphics::TextureFormat::R32_SFLOAT;
	fogTexture = app_.createTexture3D(textureDescription, field.data(), field.size() * sizeof(float));

	// Transparent at background density, through aether blue, to pale blue at the peak
	fogTransferFunction = app_.createTransferFunction({
		{0.0f, glm::vec4(0.1f, 0.3f, 0.8f, 0.0f)},
		{0.5f, glm::vec4(0.2f, 0.5f, 0.95f, 0.5f)},
		{1.0f, glm::vec4(0.7f, 0.9f, 1.0f, 1.0f)},
	});

	lightGraphics::VolumeRenderDescription volumeDescription;
	volumeDescription.volumeTexture = fogTexture;
	volumeDescription.transferFunction = fogTransferFunction;
	volumeDescription.volumeMin = glm::vec3(-fogHalfRange);
	volumeDescription.volumeMax = glm::vec3(fogHalfRange);
	volumeDescription.opacityModel = lightGraphics::VolumeOpacityModel::ExponentialExtinction;
	volumeDescription.opacityScale = fogOpacity;
	volumeDescription.raymarchSteps = 128;
	volumeDescription.enableJitter = true;
	fogVolume = app_.createVolume(volumeDescription);
	// Created hidden; setDisplayMode(FOG) draws it
}

void AetherDensityVisualizer::destroyFogVolume()
{
	// The volume references the texture and transfer function, so it goes first
	if (fogVolume.isValid()) {
		app_.destroyVolume(fogVolume);
		fogVolume = {};
	}
	if (fogTransferFunction.isValid()) {
		app_.destroyTransferFunction(fogTransferFunction);
		fogTransferFunction = {};
	}
	if (fogTexture.isValid()) {
		app_.destroyTexture3D(fogTexture);
		fogTexture = {};
	}
}

void AetherDensityVisualizer::createLattice()
{
	// Regular Nx^3 grid, as in the JavaScript model
	const float step = (2.0f * halfRange) / (Nx - 1);
	const size_t totalGridPoints = static_cast<size_t>(Nx) * Nx * Nx;

	sphereObjectIndices.reserve(totalGridPoints);
	spherePositions.reserve(totalGridPoints);
	orbitalRadii.reserve(totalGridPoints);
	orbitalAngles.reserve(totalGridPoints);
	orbitalSpeeds.reserve(totalGridPoints);

	const float sphereSize = sphereRadius * 0.2f;

	for (int i = 0; i < Nx; i++)
	{
		const float xVal = -halfRange + i * step;
		for (int j = 0; j < Nx; j++)
		{
			const float yVal = -halfRange + j * step;
			for (int k = 0; k < Nx; k++)
			{
				const float zVal = -halfRange + k * step;
				const glm::vec3 position(xVal, yVal, zVal);

				// Orbit about the Y axis starting from the particle's grid position
				const float horizontalRadius = std::sqrt(xVal * xVal + zVal * zVal);
				spherePositions.push_back(position);
				orbitalRadii.push_back(horizontalRadius);
				orbitalAngles.push_back(std::atan2(zVal, xVal));
				orbitalSpeeds.push_back(calculateOrbitalSpeed(horizontalRadius));

				app_.addObject(
					lightGraphics::ShapeType::CUBE,
					position,
					glm::vec3(sphereSize),
					particleColor,
					glm::quat(1, 0, 0, 0),
					"Aether Sphere " + std::to_string(sphereObjectIndices.size()),
					1.0f // Mass
				);
				sphereObjectIndices.push_back(app_.getObjectCount() - 1);
			}
		}
	}

	// Large yellow sphere at the centre
	app_.addObject(
		lightGraphics::ShapeType::SPHERE,
		glm::vec3(0.0f),
		glm::vec3(2.0f),
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
		glm::quat(1, 0, 0, 0),
		"Central Aether Core",
		10.0f // Heavy mass
	);
}

void AetherDensityVisualizer::updateMotion(float deltaTime)
{
	for (size_t i = 0; i < spherePositions.size(); i++)
	{
		orbitalAngles[i] = std::fmod(orbitalAngles[i] + orbitalSpeeds[i] * deltaTime,
									 glm::two_pi<float>());

		const glm::vec3 newPosition(
			orbitalRadii[i] * std::cos(orbitalAngles[i]),
			spherePositions[i].y,
			orbitalRadii[i] * std::sin(orbitalAngles[i]));

		spherePositions[i] = newPosition;
		app_.setObjectPosition(sphereObjectIndices[i], newPosition);
	}
}

float AetherDensityVisualizer::calculateOrbitalSpeed(float horizontalRadius) const
{
	// Faster near the centre: (1 + speedVariation) x base speed on the axis,
	// base speed at the lattice corner
	const float maxRadius = halfRange * glm::root_two<float>();
	const float normalizedDistance = std::min(horizontalRadius / maxRadius, 1.0f);
	return baseAngularSpeed * (1.0f + speedVariation * (1.0f - normalizedDistance));
}

float AetherDensityVisualizer::densityField(glm::vec3 const &pos) const
{
	// rho0 + delta_rho * exp(-alpha * (x^2 + y^2 + z^2)), as in the JavaScript model
	return rho0 + delta_rho * std::exp(-alpha * glm::dot(pos, pos));
}

float AetherDensityVisualizer::refractiveIndex(glm::vec3 const &pos) const
{
	return 1.0f + beta * (densityField(pos) - rho0);
}

glm::vec3 AetherDensityVisualizer::refractiveIndexGradient(glm::vec3 const &pos, float dr) const
{
	const glm::vec3 dx(dr, 0.0f, 0.0f);
	const glm::vec3 dy(0.0f, dr, 0.0f);
	const glm::vec3 dz(0.0f, 0.0f, dr);
	return glm::vec3(
		refractiveIndex(pos + dx) - refractiveIndex(pos - dx),
		refractiveIndex(pos + dy) - refractiveIndex(pos - dy),
		refractiveIndex(pos + dz) - refractiveIndex(pos - dz)) / (2.0f * dr);
}

void AetherDensityVisualizer::stepRay(LightRay3D& ray)
{
	// Advance at the local speed of light c0 / n
	const float localSpeed = c0 / refractiveIndex(ray.pos);
	ray.pos += localSpeed * ray.k * rayStepSize;

	// Bend towards higher refractive index
	ray.k += refractiveIndexGradient(ray.pos, 1e-3f) * rayStepSize;
	const float mag = glm::length(ray.k);
	if (mag > 1e-12f) {
		ray.k /= mag;
	}

	// Keep only as many points as the longest trail can show
	ray.path.push_back(ray.pos);
	while (ray.path.size() > static_cast<size_t>(MAX_RAY_SEGMENTS)) {
		ray.path.pop_front();
	}
}

void AetherDensityVisualizer::createRaySegments()
{
	rayObjectIndices.clear();
	rayObjectIndices.reserve(maxRaySegments);

	for (int i = 0; i < maxRaySegments; i++) {
		app_.addObject(
			lightGraphics::ShapeType::CUBE,
			glm::vec3(0.0f),
			glm::vec3(0.05f),
			glm::vec4(1.0f, 0.0f, 1.0f, 0.0f), // Start transparent
			glm::quat(1, 0, 0, 0),
			"Light Ray Segment " + std::to_string(i),
			0.1f
		);
		rayObjectIndices.push_back(app_.getObjectCount() - 1);
	}
}

void AetherDensityVisualizer::launchNewRay()
{
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// Start just outside the lattice on the -X side
	const glm::vec3 pos0(
		-halfRange - 0.2f,
		-halfRange + (2.0f * halfRange) * dist(rng),
		-halfRange + (2.0f * halfRange) * dist(rng));

	// Mostly +X with some random Y and Z
	const float dy = (dist(rng) < 0.5f ? -1.0f : 1.0f) * (0.1f + 0.4f * dist(rng));
	const float dz = (dist(rng) < 0.5f ? -1.0f : 1.0f) * (0.1f + 0.4f * dist(rng));

	currentRay = std::make_unique<LightRay3D>(pos0, glm::vec3(1.0f, dy, dz));
}

void AetherDensityVisualizer::updateLightRay(float deltaTime)
{
	if (!currentRay) return;

	// Fixed integration step, run as many times as this frame's duration needs
	rayStepAccumulator += deltaTime * rayStepsPerSecond;
	int steps = static_cast<int>(rayStepAccumulator);
	rayStepAccumulator -= steps;
	steps = std::min(steps, maxRayStepsPerFrame);

	const float bound = halfRange + 0.3f;
	for (int i = 0; i < steps; i++) {
		stepRay(*currentRay);

		const glm::vec3 tip = currentRay->pos;
		if (std::abs(tip.x) > bound || std::abs(tip.y) > bound || std::abs(tip.z) > bound) {
			launchNewRay();
			break;
		}
	}

	updateRayGraphics();
}

void AetherDensityVisualizer::updateRayGraphics()
{
	if (!currentRay || rayObjectIndices.empty()) return;

	const auto& path = currentRay->path;
	const int pathSize = static_cast<int>(path.size());
	const int segmentsToShow = std::min(pathSize, maxRaySegments);

	for (int i = 0; i < maxRaySegments; i++) {
		if (i < segmentsToShow) {
			const glm::vec3 position = path[pathSize - segmentsToShow + i];

			// Colour by distance from the centre
			const float t = glm::length(position) / (halfRange * 1.5f);
			const glm::vec3 color = rayColor(t);

			// Fade out older segments
			const float fade = std::max(0.1f, static_cast<float>(i + 1) / segmentsToShow);

			app_.setObjectPosition(rayObjectIndices[i], position);
			app_.setObjectColor(rayObjectIndices[i], glm::vec4(color, fade));
		} else {
			app_.setObjectColor(rayObjectIndices[i], glm::vec4(0.0f));
		}
	}
}

void AetherDensityVisualizer::setRayLength(int segments)
{
	segments = std::clamp(segments, MIN_RAY_SEGMENTS, MAX_RAY_SEGMENTS);

	if (segments == maxRaySegments) return;

	// removeObject() shifts every later object down one, so remove from the
	// highest index first to keep the remaining indices valid
	for (auto it = rayObjectIndices.rbegin(); it != rayObjectIndices.rend(); ++it) {
		app_.removeObject(*it);
	}

	maxRaySegments = segments;
	createRaySegments();

	std::cout << "Ray length set to " << maxRaySegments << " segments" << std::endl;
}

glm::vec3 AetherDensityVisualizer::rayColor(float t) const
{
	// Blue near the centre to red at the edge
	t = std::clamp(t, 0.0f, 1.0f);
	return glm::vec3(t, 0.0f, 1.0f - t);
}
