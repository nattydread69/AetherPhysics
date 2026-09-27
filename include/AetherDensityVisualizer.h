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

#pragma once

#include "AetherPhysicsModel.h"

#include <deque>
#include <memory>
#include <random>
#include <vector>

#include <glm/glm.hpp>

/**
This is based on Chantal Roth's general relativity
density model here:

https://jsfiddle.net/Chenopdodium/n879e5dh/49/
 */
class AetherDensityVisualizer : public AetherPhysicsModel
{
public:
	explicit AetherDensityVisualizer(lightGraphics::lightVulkanGraphics& app);
	virtual ~AetherDensityVisualizer();

	// Override pure virtual methods from PhysicsModel
	virtual void initialize() override final;
	virtual void update(float deltaTime) override final;
	virtual void cleanup() override final;

	// How the density field is shown: the orbiting particle lattice, or volumetric fog
	enum class DisplayMode {
		PARTICLES,
		FOG
	};
	void setDisplayMode(DisplayMode mode);
	void toggleDisplayMode();
	DisplayMode getDisplayMode() const { return displayMode; }

	// Public methods for ray control
	void setRayLength(int segments);
	int maxRaySegments = 50;           // Number of visible ray segments

	static constexpr int MIN_RAY_SEGMENTS = 1;
	static constexpr int MAX_RAY_SEGMENTS = 200;

private:
	// Density field parameters: rho = rho0 + delta_rho * exp(-alpha * r^2)
	float c0 = 1.0f;
	float rho0 = 1.0f;
	float delta_rho = 0.8f;
	float alpha = 0.2f;
	float beta = 1.0f;                 // Refractive index n = 1 + beta * (rho - rho0)
	int Nx = 25;                       // Lattice resolution (Nx^3 particles), set from the GPU tier
	float halfRange = 3.0f;            // Lattice spans [-halfRange, halfRange] on each axis

	DisplayMode displayMode = DisplayMode::PARTICLES;
	const glm::vec4 particleColor{0.1f, 0.3f, 0.8f, 0.7f}; // Blue, slightly transparent

	// Fog shows the density excess over the background, (rho - rho0) / delta_rho,
	// sampled on a fogResolution^3 grid. The fog box is wider than the lattice so
	// the fog fades out before reaching its edges.
	int fogResolution = 64;
	float fogHalfRange = 5.0f;
	float fogOpacity = 0.5f;           // Optical depth per unit length at peak density
	lightGraphics::Texture3DHandle fogTexture;
	lightGraphics::TransferFunctionHandle fogTransferFunction;
	lightGraphics::VolumeHandle fogVolume;

	// Orbital motion about the Y axis
	float baseAngularSpeed = 0.1f;     // Angular speed at the lattice edge (rad/s)
	float speedVariation = 1.0f;       // Extra speed at the centre (1 = twice as fast)

	// Per-particle orbit, in the XZ plane at the particle's own height
	std::vector<float> orbitalRadii;   // Horizontal distance from the Y axis
	std::vector<float> orbitalAngles;  // Current angle about the Y axis
	std::vector<float> orbitalSpeeds;  // Angular speed (rad/s)

	// Light ray traced through the density field
	struct LightRay3D {
		glm::vec3 pos;                 // Current position
		glm::vec3 k;                   // Direction vector (normalized)
		std::deque<glm::vec3> path;    // Most recent positions, oldest first

		LightRay3D(glm::vec3 position, glm::vec3 direction)
			: pos(position), k(glm::normalize(direction)) {
			path.push_back(pos);
		}
	};

	std::unique_ptr<LightRay3D> currentRay;
	std::vector<int> rayObjectIndices; // Graphics object indices for the ray trail
	float rayStepSize = 0.02f;         // Integration step for the ray
	float rayStepsPerSecond = 300.0f;  // Ray integration rate, independent of frame rate
	int maxRayStepsPerFrame = 50;      // Cap so a long frame can't stall the app
	float rayStepAccumulator = 0.0f;   // Fractional steps carried to the next frame

	std::mt19937 rng{std::random_device{}()};

	void createLattice();
	void setParticlesVisible(bool visible);
	void updateMotion(float deltaTime);

	void createFogVolume();
	void destroyFogVolume();
	float calculateOrbitalSpeed(float horizontalRadius) const;

	float densityField(glm::vec3 const &pos) const;
	float refractiveIndex(glm::vec3 const &pos) const;
	glm::vec3 refractiveIndexGradient(glm::vec3 const &pos, float dr) const;

	// Light ray methods
	void createRaySegments();
	void launchNewRay();
	void stepRay(LightRay3D& ray);
	void updateLightRay(float deltaTime);
	void updateRayGraphics();
	glm::vec3 rayColor(float t) const;
};
