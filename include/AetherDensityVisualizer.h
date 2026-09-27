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

#pragma once

#include "AetherPhysicsModel.h"
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

	// Public methods for ray control
	void setRayLength(int segments);
	int maxRaySegments = 50;           // Maximum number of visible ray segments

protected:
	// Density visualization parameters
	float maxRadius = 20.0f;           // Maximum radius for sphere distribution
	float densityFalloff = 0.1f;       // Controls how quickly density decreases with distance
	int totalSpheres = 1000;           // Total number of spheres to create
	float c0 = 1.0;
    float rho0 = 1.0;
    float delta_rho = 0.8;
    float alpha = 0.2;
    float beta = 1.0;
    float Nx = 25;                    // Higher resolution lattice (25^3 = 15625 particles)
    float halfRange = 3;


	// Orbital motion parameters
	float baseRotationSpeed = 0.003f;    // Base rotation speed multiplier (slower)
	float speedVariation = 1.0f;       // How much speed varies from center to edge (gentler)
	glm::vec3 centerPoint = glm::vec3(0.0f); // Center point for orbital motion

	// Sphere data for orbital motion
	std::vector<float> orbitalRadii;   // Distance from center for each sphere
	std::vector<float> orbitalAngles;  // Current angle for each sphere
	std::vector<float> orbitalSpeeds;  // Orbital speed for each sphere
	std::vector<glm::vec3> orbitalAxes; // Rotation axis for each sphere (random)

	// Central sphere
	int centralSphereIndex = -1;       // Index of the central yellow sphere

	// Light ray parameters
	struct LightRay3D {
		glm::vec3 pos;           // Current position
		glm::vec3 k;             // Direction vector (normalized)
		std::vector<glm::vec3> path;  // Path history

		LightRay3D(glm::vec3 position, glm::vec3 direction)
			: pos(position), k(glm::normalize(direction)) {
			path.push_back(pos);
		}

		void step(float dtLocal, float c0, float rho0, float delta_rho, float alpha, float beta, AetherDensityVisualizer* parent);
		const std::vector<glm::vec3>& getPath() const { return path; }
	};

	// Light ray data
	LightRay3D* currentRay = nullptr;
	std::vector<int> rayObjectIndices; // Graphics object indices for the ray trail
	float dt = 0.02f;                  // Time step for ray integration
	int maxPathLength = 1000;          // Maximum path points to store
	int stepsPerFrame = 5;             // Ray steps per frame

private:
	virtual void createLattice();
	virtual void updateMotion(float time);
	virtual void increaseRotationSpeed();
	virtual void decreaseRotationSpeed();
	float refractiveIndex(glm::vec3 const &pos);
	float gradientRefractiveIndex(glm::vec3 const &pos);
	glm::vec3 gradientRefractiveIndexVector(glm::vec3 const &pos);

	float calculateDensity(glm::vec3 const &pos);
	float densityField(glm::vec3 const &pos, float rho0, float delta_rho, float alpha);
	glm::vec3 generateRandomPosition();
	glm::vec3 generateRandomAxis();
	float calculateOrbitalSpeed(float distance);

	// Light ray methods
	void createLightRay();
	void launchNewRay();
	void updateLightRay(float deltaTime);
	void updateRayGraphics();
	glm::vec3 viridisColor(float t);
	glm::vec3 gradientRefractiveIndexVector(glm::vec3 const &pos, float dr, float rho0, float delta_rho, float alpha);

};
