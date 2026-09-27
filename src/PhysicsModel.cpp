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

#include "PhysicsModel.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

PhysicsModel::PhysicsModel(lightGraphics::lightVulkanGraphics& app,
						   std::string const &name)
: lightGraphics::GraphicsModel(app, name)
{
}

PhysicsModel::~PhysicsModel()
{
}

glm::vec3 PhysicsModel::transverseBasis1(const glm::vec3& direction)
{
	glm::vec3 normalizedDir = glm::normalize(direction);

	// Find a vector perpendicular to the direction
	if (glm::abs(normalizedDir.y) < 0.9f)
	{
		return glm::normalize(glm::cross(normalizedDir, glm::vec3(0, 1, 0)));
	}
	else
	{
		return glm::normalize(glm::cross(normalizedDir, glm::vec3(1, 0, 0)));
	}
}

glm::vec3 PhysicsModel::transverseBasis2(const glm::vec3& direction)
{
	glm::vec3 normalizedDir = glm::normalize(direction);
	glm::vec3 basis1 = transverseBasis1(direction);
	return glm::normalize(glm::cross(normalizedDir, basis1));
}


