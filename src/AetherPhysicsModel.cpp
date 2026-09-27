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

#include "AetherPhysicsModel.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

AetherPhysicsModel::
AetherPhysicsModel(lightGraphics::lightVulkanGraphics& app, std::string const &name)
  : PhysicsModel(app, name)
{
}

AetherPhysicsModel::~AetherPhysicsModel()
{
}

void AetherPhysicsModel::initialize()
{
	std::cout << "Initializing " << getName() << "..." << std::endl;

	createLattice();

	std::cout << "Created charged-lepton structure with "
			  << leptonFamilies.size() << " families, "
			  << sphereObjectIndices.size() << " aether markers, and "
			  << fieldLines.size() << " Coulomb guides" << std::endl;
}

void AetherPhysicsModel::update(float deltaTime)
{
	time += deltaTime;
	updateMotion(time);
}

void AetherPhysicsModel::cleanup()
{
	std::cout << "Cleaning up " << getName() << "..." << std::endl;
}

void AetherPhysicsModel::createLattice()
{
	initializeLeptonFamilies();

	const size_t totalMarkers = leptonFamilies.size() *
		static_cast<size_t>(ringCount * pointsPerRing * axialLayers);

	sphereObjectIndices.clear();
	spherePositions.clear();
	sphereVelocities.clear();
	sphereForces.clear();
	initialPositions.clear();
	swirlNodes.clear();
	fieldLines.clear();

	sphereObjectIndices.reserve(totalMarkers);
	spherePositions.reserve(totalMarkers);
	sphereVelocities.reserve(totalMarkers);
	sphereForces.reserve(totalMarkers);
	initialPositions.reserve(totalMarkers);
	swirlNodes.reserve(totalMarkers);

	for (size_t familyIndex = 0; familyIndex < leptonFamilies.size(); ++familyIndex)
	{
		createLeptonFamily(familyIndex);
		createFieldLines(familyIndex);
	}
}

void AetherPhysicsModel::updateMotion(float time)
{
	for (size_t i = 0; i < swirlNodes.size(); ++i)
	{
		const SwirlNode& node = swirlNodes[i];
		const LeptonFamily& family = leptonFamilies[node.familyIndex];
		const float normalizedRadius = node.ringRadius / family.profileRadius;
		const float profile =
			excitationProfile(family.excitationLevel, normalizedRadius);
		const float brightness =
			0.5f + 0.5f *
			std::sin(time * family.pulseSpeed + node.phaseOffset);
		const float scale =
			swirlNodeRadius *
			(0.80f + 0.30f * std::abs(profile) + 0.20f * brightness);
		const glm::vec3 position = swirlPosition(node, time);

		spherePositions[i] = position;
		app_.setObjectPosition(sphereObjectIndices[i], position);
		app_.setObjectScale(sphereObjectIndices[i], glm::vec3(scale));
		app_.setObjectColor(
			sphereObjectIndices[i],
			excitationColor(family, profile, brightness));
	}

	for (size_t familyIndex = 0; familyIndex < leptonFamilies.size(); ++familyIndex)
	{
		const LeptonFamily& family = leptonFamilies[familyIndex];
		const float pulse =
			0.5f + 0.5f *
			std::sin(time * family.pulseSpeed + static_cast<float>(familyIndex));
		const float coreScale =
			family.coreRadius * (1.0f + 0.08f * std::sin(time * family.pulseSpeed));

		app_.setObjectScale(family.coreObjectIndex, glm::vec3(coreScale));
		app_.setObjectColor(
			family.coreObjectIndex,
			excitationColor(family, 1.0f, pulse));
		app_.setObjectColor(
			family.axisObjectIndex,
			excitationColor(family, 0.4f, 0.4f + 0.6f * pulse));
	}

	for (size_t i = 0; i < fieldLines.size(); ++i)
	{
		const FieldLine& line = fieldLines[i];
		const LeptonFamily& family = leptonFamilies[line.familyIndex];
		const float glow =
			0.45f + 0.35f *
			std::sin(time * 0.9f + static_cast<float>(line.familyIndex) + 0.4f * static_cast<float>(i));
		app_.setObjectColor(
			line.objectIndex,
			excitationColor(family, 0.2f, glow));
	}
}

void AetherPhysicsModel::initializeLeptonFamilies()
{
	leptonFamilies = {
		{
			"Electron",
			0,
			glm::vec3(-5.0f, 0.0f, 0.0f),
			glm::normalize(glm::vec3(0.25f, 1.0f, -0.15f)),
			glm::vec4(0.14f, 0.62f, 1.0f, 1.0f),
			glm::vec4(0.72f, 0.90f, 1.0f, 1.0f),
			0.46f,
			1.35f,
			1.6f,
			1.7f,
			2.2f
		},
		{
			"Muon",
			1,
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::normalize(glm::vec3(-0.18f, 1.0f, 0.28f)),
			glm::vec4(0.98f, 0.66f, 0.18f, 1.0f),
			glm::vec4(1.0f, 0.84f, 0.52f, 1.0f),
			0.58f,
			1.75f,
			1.2f,
			1.15f,
			2.8f
		},
		{
			"Tau",
			2,
			glm::vec3(5.0f, 0.0f, 0.0f),
			glm::normalize(glm::vec3(0.14f, 1.0f, 0.35f)),
			glm::vec4(0.92f, 0.28f, 0.24f, 1.0f),
			glm::vec4(1.0f, 0.60f, 0.54f, 1.0f),
			0.74f,
			2.15f,
			0.85f,
			0.92f,
			3.4f
		}
	};
}

void AetherPhysicsModel::createLeptonFamily(size_t familyIndex)
{
	LeptonFamily& family = leptonFamilies[familyIndex];

	app_.addObject(
		lightGraphics::ShapeType::SPHERE,
		family.center,
		glm::vec3(family.coreRadius),
		family.coreColor,
		glm::quat(1, 0, 0, 0),
		family.name + " Core",
		1.0f);
	family.coreObjectIndex = static_cast<int>(app_.getObjectCount() - 1);

	app_.addCylinderAlongAxis(
		family.center,
		family.spinAxis,
		2.6f + 0.7f * static_cast<float>(family.excitationLevel),
		spinAxisRadius,
		excitationColor(family, 0.5f, 0.65f),
		family.name + " Spin Axis",
		0.1f);
	family.axisObjectIndex = static_cast<int>(app_.getObjectCount() - 1);

	const float layerSpacing = 0.24f * family.coreRadius;

	for (int ringIndex = 0; ringIndex < ringCount; ++ringIndex)
	{
		const float normalizedRadius = 0.35f + 0.35f * static_cast<float>(ringIndex);
		const float ringRadius = family.profileRadius * normalizedRadius;

		for (int layerIndex = 0; layerIndex < axialLayers; ++layerIndex)
		{
			const float axialOffset =
				(static_cast<float>(layerIndex) - 0.5f * static_cast<float>(axialLayers - 1)) *
				layerSpacing;

			for (int pointIndex = 0; pointIndex < pointsPerRing; ++pointIndex)
			{
				SwirlNode node;
				node.familyIndex = familyIndex;
				node.ringRadius = ringRadius;
				node.baseAngle =
					glm::two_pi<float>() * static_cast<float>(pointIndex) /
					static_cast<float>(pointsPerRing);
				node.baseAngle += 0.18f * static_cast<float>(family.excitationLevel);
				node.axialOffset = axialOffset;
				node.phaseOffset =
					0.55f * static_cast<float>(pointIndex) +
					0.9f * static_cast<float>(ringIndex) +
					0.45f * static_cast<float>(layerIndex);

				swirlNodes.push_back(node);

				const float profile =
					excitationProfile(family.excitationLevel, normalizedRadius);
				const float brightness =
					0.55f + 0.45f * std::sin(node.phaseOffset);
				const glm::vec3 position = swirlPosition(node, 0.0f);

				spherePositions.push_back(position);
				initialPositions.push_back(position);
				sphereVelocities.push_back(glm::vec3(0.0f));
				sphereForces.push_back(glm::vec3(0.0f));

				app_.addObject(
					lightGraphics::ShapeType::SPHERE,
					position,
					glm::vec3(swirlNodeRadius),
					excitationColor(family, profile, brightness),
					glm::quat(1, 0, 0, 0),
					family.name + " Mode " + std::to_string(ringIndex) + ":" +
						std::to_string(layerIndex) + ":" + std::to_string(pointIndex),
					0.2f);
				sphereObjectIndices.push_back(static_cast<int>(app_.getObjectCount() - 1));
			}
		}
	}
}

void AetherPhysicsModel::createFieldLines(size_t familyIndex)
{
	const LeptonFamily& family = leptonFamilies[familyIndex];
	const glm::vec3 tangent = transverseBasis1(family.spinAxis);
	const glm::vec3 bitangent = transverseBasis2(family.spinAxis);
	const std::vector<glm::vec3> directions = {
		tangent,
		-tangent,
		bitangent,
		-bitangent,
		glm::normalize(tangent + bitangent),
		glm::normalize(tangent - bitangent)
	};

	for (const glm::vec3& direction : directions)
	{
		const glm::vec3 center =
			family.center + direction * (family.coreRadius + 0.5f * family.fieldLength);

		app_.addCylinderAlongAxis(
			center,
			direction,
			family.fieldLength,
			fieldLineRadius,
			excitationColor(family, 0.2f, 0.55f),
			family.name + " Coulomb Guide",
			0.1f);

		fieldLines.push_back(
			{familyIndex, direction, static_cast<int>(app_.getObjectCount() - 1)});
	}
}

glm::vec3 AetherPhysicsModel::swirlPosition(const SwirlNode& node, float time)
{
	const LeptonFamily& family = leptonFamilies[node.familyIndex];
	const glm::vec3 tangent = transverseBasis1(family.spinAxis);
	const glm::vec3 bitangent = transverseBasis2(family.spinAxis);
	const float normalizedRadius = node.ringRadius / family.profileRadius;
	const float profile =
		excitationProfile(family.excitationLevel, normalizedRadius);
	const float angularPhase =
		node.baseAngle + family.swirlSpeed * profile * time;
	const float radialBreathing =
		1.0f + 0.08f * std::sin(time * family.pulseSpeed + node.phaseOffset);
	const float axialBreathing =
		node.axialOffset +
		0.18f * family.coreRadius * profile *
			std::sin(1.4f * time * family.pulseSpeed + node.phaseOffset);
	const float ringRadius = node.ringRadius * radialBreathing;

	return family.center +
		tangent * std::cos(angularPhase) * ringRadius +
		bitangent * std::sin(angularPhase) * ringRadius +
		family.spinAxis * axialBreathing;
}

float AetherPhysicsModel::excitationProfile(
	int excitationLevel,
	float normalizedRadius) const
{
	const float u2 = normalizedRadius * normalizedRadius;
	const float envelope = std::exp(-u2);

	// Toy family profiles from the paper's visualization section:
	// electron f0, muon f1, tau f2.
	switch (excitationLevel)
	{
		case 0:
			return envelope;
		case 1:
			return (1.0f - u2) * envelope;
		default:
			return (1.0f - 3.0f * u2 + 0.5f * u2 * u2) * envelope;
	}
}

glm::vec4 AetherPhysicsModel::excitationColor(
	const LeptonFamily& family,
	float profile,
	float phase) const
{
	const float brightness = glm::clamp(phase, 0.0f, 1.0f);
	const float amplitude = glm::clamp(std::abs(profile), 0.0f, 1.0f);
	const glm::vec3 base(family.haloColor.r, family.haloColor.g, family.haloColor.b);
	const glm::vec3 signColor = (profile >= 0.0f)
		? glm::vec3(0.95f, 0.98f, 1.0f)
		: glm::vec3(1.0f, 0.45f, 0.32f);
	const glm::vec3 color =
		base * (0.45f + 0.30f * brightness) +
		signColor * (0.20f + 0.35f * amplitude);

	return glm::vec4(
		glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f)),
		1.0f);
}
