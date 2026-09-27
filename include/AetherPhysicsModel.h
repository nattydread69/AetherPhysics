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

#include "PhysicsModel.h"

#include <string>
#include <vector>

/**
 * @brief AetherPhysicsModel class
 *
 * This class implements the common charged-lepton visualization used by the
 * aether theory models.
 *
 * @author Nathanael John Inkson
 * @date 2025-09-30
 */
class AetherPhysicsModel : public PhysicsModel
{
public:
	explicit AetherPhysicsModel(lightGraphics::lightVulkanGraphics& app,
								std::string const &name);
	virtual ~AetherPhysicsModel();

	// Override pure virtual methods from PhysicsModel
	virtual void initialize() override;
	virtual void update(float deltaTime) override;
	virtual void cleanup() override;

protected:
	void updateMotion(float time);

	// Physics data storage
	std::vector<int> sphereObjectIndices;
	std::vector<glm::vec3> spherePositions;
	std::vector<glm::vec3> sphereVelocities;
	std::vector<glm::vec3> sphereForces;

	// Physics constants
	const float sphereRadius = 0.2f;
	const int latticeSize = 10; // Size of the cubic lattice

	// Wave motion parameters
	std::vector<glm::vec3> initialPositions; // Store original positions (pos0)

	// Common helper methods
	void createLattice();

private:
	struct LeptonFamily
	{
		std::string name;
		int excitationLevel = 0;
		glm::vec3 center = glm::vec3(0.0f);
		glm::vec3 spinAxis = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec4 coreColor = glm::vec4(1.0f);
		glm::vec4 haloColor = glm::vec4(1.0f);
		float coreRadius = 0.5f;
		float profileRadius = 1.5f;
		float swirlSpeed = 1.0f;
		float pulseSpeed = 1.0f;
		float fieldLength = 2.0f;
		int coreObjectIndex = -1;
		int axisObjectIndex = -1;
	};

	struct SwirlNode
	{
		size_t familyIndex = 0;
		float ringRadius = 0.0f;
		float baseAngle = 0.0f;
		float axialOffset = 0.0f;
		float phaseOffset = 0.0f;
	};

	struct FieldLine
	{
		size_t familyIndex = 0;
		glm::vec3 direction = glm::vec3(1.0f, 0.0f, 0.0f);
		int objectIndex = -1;
	};

	void initializeLeptonFamilies();
	void createLeptonFamily(size_t familyIndex);
	void createFieldLines(size_t familyIndex);
	glm::vec3 swirlPosition(const SwirlNode& node, float time);
	float excitationProfile(int excitationLevel, float normalizedRadius) const;
	glm::vec4 excitationColor(
		const LeptonFamily& family,
		float profile,
		float phase) const;

	std::vector<LeptonFamily> leptonFamilies;
	std::vector<SwirlNode> swirlNodes;
	std::vector<FieldLine> fieldLines;

	const int ringCount = 5;
	const int pointsPerRing = 12;
	const int axialLayers = 3;
	const float swirlNodeRadius = 0.09f;
	const float spinAxisRadius = 0.035f;
	const float fieldLineRadius = 0.03f;
};
