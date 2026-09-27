// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Light Vulkan Graphics
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
#include <glm/fwd.hpp>
#include <iostream>
#include <algorithm>
#include <random>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

AetherDensityVisualizer::
AetherDensityVisualizer(lightGraphics::lightVulkanGraphics& app)
	: AetherPhysicsModel(app, "Aether Density Visualizer")
{
	GPUCapabilities::GPUInfo gpuInfo = GPUCapabilities::detectGPU();
	Nx = gpuInfo.latticeResolution;
}

AetherDensityVisualizer::~AetherDensityVisualizer()
{
	if (currentRay) {
		delete currentRay;
		currentRay = nullptr;
	}
}

void AetherDensityVisualizer::initialize()
{
	std::cout << "Initializing Aether Density Visualizer..." << std::endl;

	// Create the density field directly
	createLattice();

	// Create the light ray
	createLightRay();

	std::cout << "Created density field with " << totalSpheres << " spheres" << std::endl;
}

void AetherDensityVisualizer::update(float deltaTime)
{
	// Update orbital motion
	time += deltaTime;
	updateMotion(time);

	// Update light ray
	updateLightRay(deltaTime);
}

void AetherDensityVisualizer::cleanup()
{
	// Clean up any AetherDensityVisualizer specific resources
	std::cout << "Cleaning up Aether Density Visualizer..." << std::endl;
}

void AetherDensityVisualizer::createLattice()
{
	// Calculate grid parameters similar to JavaScript code
	int Nx = static_cast<int>(this->Nx);
	float step = (2.0f * halfRange) / (Nx - 1);

	// Calculate total number of grid points
	int totalGridPoints = Nx * Nx * Nx;

	// Reserve space for all spheres
	sphereObjectIndices.reserve(totalGridPoints);
	spherePositions.reserve(totalGridPoints);
	sphereVelocities.reserve(totalGridPoints);
	sphereForces.reserve(totalGridPoints);
	initialPositions.reserve(totalGridPoints);
	orbitalRadii.reserve(totalGridPoints);
	orbitalAngles.reserve(totalGridPoints);
	orbitalSpeeds.reserve(totalGridPoints);
	orbitalAxes.reserve(totalGridPoints);

	// Random number generator for orbital parameters
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * glm::pi<float>());
	std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);

	// Create regular grid of spheres with density-based properties
	// Similar to JavaScript: for(let i=0; i<Nx; i++) { for(let j=0; j<Nx; j++) { for(let k=0; k<Nx; k++) {
	for (int i = 0; i < Nx; i++)
	{
		float xVal = -halfRange + i * step;
		for (int j = 0; j < Nx; j++)
		{
			float yVal = -halfRange + j * step;
			for (int k = 0; k < Nx; k++)
			{
				float zVal = -halfRange + k * step;

				// Create grid point position
				glm::vec3 position(xVal, yVal, zVal);
				float distance = glm::length(position - centerPoint);

				// Store initial position
				spherePositions.push_back(position);
				initialPositions.push_back(position);
				sphereVelocities.push_back(glm::vec3(0.0f));
				sphereForces.push_back(glm::vec3(0.0f));

				// Calculate orbital parameters
				orbitalRadii.push_back(distance);
				orbitalAngles.push_back(angleDist(gen));
				orbitalSpeeds.push_back(calculateOrbitalSpeed(distance));
				orbitalAxes.push_back(generateRandomAxis());

				// Blue aether particles - small and uniform
				// All particles have the same blue color
				glm::vec4 color(0.1f, 0.3f, 0.8f, 0.7f);  // Blue with slight transparency

				// Uniform size - make grid visible
				float sphereSize = sphereRadius * 0.2f;  // Larger particles to show grid clearly

				// Add sphere to scene and store object index
				app_.addObject(
					lightGraphics::ShapeType::CUBE,
					position,
					glm::vec3(sphereSize),
					color,
					glm::quat(1, 0, 0, 0), // No rotation initially
					"Aether Sphere " + std::to_string(sphereObjectIndices.size()),
					1.0f // Mass
				);
				sphereObjectIndices.push_back(app_.getObjectCount() - 1);
			}
		}
	}

	// Update totalSpheres to reflect actual number created
	totalSpheres = static_cast<int>(sphereObjectIndices.size());

	// Add a large yellow sphere at the center point
	app_.addObject(
		lightGraphics::ShapeType::SPHERE,
		centerPoint,
		glm::vec3(2.0f), // Large radius
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // Bright yellow color
		glm::quat(1, 0, 0, 0), // No rotation
		"Central Aether Core",
		10.0f // Heavy mass
	);
	// Store the central sphere index for potential future use
	centralSphereIndex = app_.getObjectCount() - 1;
}

void AetherDensityVisualizer::updateMotion(float time)
{
	// Update orbital motion for all spheres
	for (size_t i = 0; i < spherePositions.size(); i++)
	{
		// Update orbital angle
		orbitalAngles[i] += orbitalSpeeds[i] * time * baseRotationSpeed;

		// Simple circular motion in XY plane for now to avoid complex calculations
		float x = orbitalRadii[i] * std::cos(orbitalAngles[i]);
		float z = orbitalRadii[i] * std::sin(orbitalAngles[i]);
		float y = spherePositions[i].y;

		glm::vec3 newPosition(x, y, z);

		// Update sphere position
		spherePositions[i] = newPosition;

		// Update the graphics object
		app_.setObjectPosition(sphereObjectIndices[i], newPosition);
	}
}

void AetherDensityVisualizer::increaseRotationSpeed()
{
	baseRotationSpeed *= 1.2f;
	std::cout << "Rotation speed increased to: " << baseRotationSpeed << std::endl;
}

void AetherDensityVisualizer::decreaseRotationSpeed()
{
	baseRotationSpeed *= 0.8f;
	std::cout << "Rotation speed decreased to: " << baseRotationSpeed << std::endl;
}

float AetherDensityVisualizer::calculateDensity(glm::vec3 const &pos)
{
	// Exponential falloff: density decreases as distance increases
	//return std::exp(-densityFalloff * distance);
	// rho0=1.0, delta_rho=0.8, alpha=0.2)
	return rho0 + delta_rho*std::exp(-alpha*(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z));
}

float AetherDensityVisualizer::densityField(glm::vec3 const &pos, float rho0, float delta_rho, float alpha)
{
	// This matches the JavaScript densityField function
	// rho0 + delta_rho * exp(-alpha * (x^2 + y^2 + z^2))
	return rho0 + delta_rho * std::exp(-alpha * (pos.x * pos.x + pos.y * pos.y + pos.z * pos.z));
}

glm::vec3 AetherDensityVisualizer::generateRandomPosition()
{
	std::random_device rd;
	std::mt19937 gen(rd());

	// Use rejection sampling to create density-based distribution
	while (true)
	{
		// Generate random position in cube
		std::uniform_real_distribution<float> posDist(-maxRadius, maxRadius);
		glm::vec3 candidate(posDist(gen), posDist(gen), posDist(gen));
		float distance = glm::length(candidate);

		// Reject if outside max radius
		if (distance > maxRadius) continue;

		// Accept with probability proportional to density
		float density = calculateDensity(candidate);
		std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
		if (probDist(gen) < density) {
			return candidate;
		}
	}
}

glm::vec3 AetherDensityVisualizer::generateRandomAxis()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);

	glm::vec3 axis(axisDist(gen), axisDist(gen), axisDist(gen));
	return glm::normalize(axis);
}

float AetherDensityVisualizer::calculateOrbitalSpeed(float distance)
{
	// Speed increases as distance decreases (faster near center)
	// Use inverse relationship: speed = baseSpeed / (1 + distance)
	float normalizedDistance = distance / maxRadius;
	return baseRotationSpeed * (1.0f + speedVariation * (1.0f - normalizedDistance));
}

float AetherDensityVisualizer::refractiveIndex(glm::vec3 const &pos)
{
	float rho = densityField(pos, rho0, delta_rho, alpha);
	return 1.0 + beta*(rho - rho0);
}

float AetherDensityVisualizer::gradientRefractiveIndex(glm::vec3 const &pos)
{
	return densityField(pos, rho0, delta_rho, alpha);
}

glm::vec3 AetherDensityVisualizer::gradientRefractiveIndexVector(glm::vec3 const &pos)
{
	float dr = 0.01f; // Small step for numerical differentiation
	const float dndx = (gradientRefractiveIndex(glm::vec3(pos.x+dr,pos.y,pos.z))
	 - gradientRefractiveIndex(glm::vec3(pos.x-dr,pos.y,pos.z)))/(2*dr);
	const float dndy = (gradientRefractiveIndex(glm::vec3(pos.x,pos.y+dr,pos.z))
	 - gradientRefractiveIndex(glm::vec3(pos.x,pos.y-dr,pos.z)))/(2*dr);
	const float dndz = (gradientRefractiveIndex(glm::vec3(pos.x,pos.y,pos.z+dr))
	 - gradientRefractiveIndex(glm::vec3(pos.x,pos.y,pos.z-dr)))/(2*dr);
	return glm::vec3(dndx, dndy, dndz);
}

// Light ray implementation
void AetherDensityVisualizer::LightRay3D::step(float dtLocal, float c0, float rho0, float delta_rho, float alpha, float beta, AetherDensityVisualizer* parent)
{
	// Calculate refractive index at current position
	float nVal = 1.0f + beta * (parent->densityField(pos, rho0, delta_rho, alpha) - rho0);

	// Calculate local speed
	float localSpeed = c0 / nVal;

	// Update position
	pos.x += localSpeed * k.x * dtLocal;
	pos.y += localSpeed * k.y * dtLocal;
	pos.z += localSpeed * k.z * dtLocal;

	// Calculate gradient of refractive index
	float dr = 1e-3f;
	glm::vec3 gradN = parent->gradientRefractiveIndexVector(pos, dr, rho0, delta_rho, alpha);

	// Update direction vector
	k.x += gradN.x * dtLocal;
	k.y += gradN.y * dtLocal;
	k.z += gradN.z * dtLocal;

	// Normalize direction vector
	float mag = glm::length(k);
	if (mag > 1e-12f) {
		k /= mag;
	}

	// Add current position to path
	path.push_back(pos);

	// Limit path length
	if (path.size() > 1000) {
		path.erase(path.begin());
	}
}

void AetherDensityVisualizer::createLightRay()
{
	// Launch initial ray
	launchNewRay();

	// Create multiple spheres for the ray trail
	rayObjectIndices.clear();
	rayObjectIndices.reserve(maxRaySegments);

	for (int i = 0; i < maxRaySegments; i++) {
		app_.addObject(
			lightGraphics::ShapeType::CUBE,
			glm::vec3(0.0f),
			glm::vec3(0.05f), // Smaller spheres for trail
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
	// Generate random starting position and direction similar to JavaScript
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// Starting position: slightly outside the range
	float x0 = -halfRange - 0.2f;
	float y0 = -halfRange + (2.0f * halfRange) * dist(gen);
	float z0 = -halfRange + (2.0f * halfRange) * dist(gen);
	glm::vec3 pos0(x0, y0, z0);

	// Direction: mostly in +X direction with some random Y and Z components
	float dx = 1.0f;
	float dy = (dist(gen) < 0.5f ? -1.0f : 1.0f) * (0.1f + 0.4f * dist(gen));
	float dz = (dist(gen) < 0.5f ? -1.0f : 1.0f) * (0.1f + 0.4f * dist(gen));
	glm::vec3 dir0(dx, dy, dz);
	dir0 = glm::normalize(dir0);

	// Create new ray
	if (currentRay) {
		delete currentRay;
	}
	currentRay = new LightRay3D(pos0, dir0);
}

void AetherDensityVisualizer::updateLightRay(float /*deltaTime*/)
{
	if (!currentRay) return;

	// Step the ray multiple times per frame
	for (int i = 0; i < stepsPerFrame; i++) {
		currentRay->step(dt, c0, rho0, delta_rho, alpha, beta, this);
	}

	// Check if ray has gone outside bounds
	glm::vec3 tip = currentRay->pos;
	if (std::abs(tip.x) > halfRange + 0.3f ||
		std::abs(tip.y) > halfRange + 0.3f ||
		std::abs(tip.z) > halfRange + 0.3f) {
		launchNewRay();
	}

	// Update graphics
	updateRayGraphics();
}

void AetherDensityVisualizer::updateRayGraphics()
{
	if (!currentRay || rayObjectIndices.empty()) return;

	const std::vector<glm::vec3>& path = currentRay->getPath();
	int pathSize = static_cast<int>(path.size());
	int segmentsToShow = std::min(pathSize, maxRaySegments);

	// Update positions and colors for visible segments
	for (int i = 0; i < maxRaySegments; i++) {
		if (i < segmentsToShow) {
			// Show this segment
			int pathIndex = pathSize - segmentsToShow + i;
			glm::vec3 position = path[pathIndex];

			// Calculate color based on position (viridis-like coloring)
			float t = glm::length(position) / (halfRange * 1.5f);
			t = std::max(0.0f, std::min(1.0f, t));
			glm::vec3 color = viridisColor(t);

			// Fade out older segments (fade from back to front)
			float alpha = static_cast<float>(i + 1) / segmentsToShow;
			alpha = std::max(0.1f, alpha); // Minimum visibility

			// Update position and color
			app_.setObjectPosition(rayObjectIndices[i], position);
			app_.setObjectColor(rayObjectIndices[i], glm::vec4(color, alpha));
		} else {
			// Hide this segment (make transparent)
			app_.setObjectColor(rayObjectIndices[i], glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}

void AetherDensityVisualizer::setRayLength(int segments)
{
	segments = std::max(1, std::min(segments, 200)); // Clamp between 1 and 200

	if (segments == maxRaySegments) return; // No change needed

	// Remove old ray objects
	for (int index : rayObjectIndices) {
		app_.removeObject(index);
	}
	rayObjectIndices.clear();

	// Update max segments
	maxRaySegments = segments;

	// Create new ray objects
	rayObjectIndices.reserve(maxRaySegments);
	for (int i = 0; i < maxRaySegments; i++) {
		app_.addObject(
			lightGraphics::ShapeType::CUBE,
			glm::vec3(0.0f),
			glm::vec3(0.05f), // Smaller spheres for trail
			glm::vec4(1.0f, 0.0f, 1.0f, 0.0f), // Start transparent
			glm::quat(1, 0, 0, 0),
			"Light Ray Segment " + std::to_string(i),
			0.1f
		);
		rayObjectIndices.push_back(app_.getObjectCount() - 1);
	}

	std::cout << "Ray length set to " << maxRaySegments << " segments" << std::endl;
}

glm::vec3 AetherDensityVisualizer::viridisColor(float t)
{
	t = std::max(0.0f, std::min(1.0f, t));
	float r = t;
	float g = 0.0f;
	float b = 1.0f - t;
	return glm::vec3(r, g, b);
}

// Helper function for gradient calculation with step size
glm::vec3 AetherDensityVisualizer::gradientRefractiveIndexVector(glm::vec3 const &pos, float dr, float /*rho0*/, float /*delta_rho*/, float /*alpha*/)
{
	const float dndx = (gradientRefractiveIndex(glm::vec3(pos.x+dr,pos.y,pos.z))
	 - gradientRefractiveIndex(glm::vec3(pos.x-dr,pos.y,pos.z)))/(2*dr);
	const float dndy = (gradientRefractiveIndex(glm::vec3(pos.x,pos.y+dr,pos.z))
	 - gradientRefractiveIndex(glm::vec3(pos.x,pos.y-dr,pos.z)))/(2*dr);
	const float dndz = (gradientRefractiveIndex(glm::vec3(pos.x,pos.y,pos.z+dr))
	 - gradientRefractiveIndex(glm::vec3(pos.x,pos.y,pos.z-dr)))/(2*dr);
	return glm::vec3(dndx, dndy, dndz);
}
