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

#include "lightVulkanGraphics.h"

#include "GraphicsModel.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "lightVulkanGraphics/ui/Types.h"

namespace lightGraphics::ui {
class Panel;
class Label;
}

class PhysicsModel : public lightGraphics::GraphicsModel
{
public:
	PhysicsModel(lightGraphics::lightVulkanGraphics& app,
				 std::string const &name);
	virtual ~PhysicsModel();

	// Pure virtual methods that derived classes must implement
	virtual void initialize() = 0;
	virtual void update(float deltaTime) = 0;
	virtual void cleanup() = 0;

	// Common physics methods
	glm::vec3 transverseBasis1(const glm::vec3& direction);
	glm::vec3 transverseBasis2(const glm::vec3& direction);

protected:
	float time = 0.0f;

	// Read-only panel explaining the physics on screen: fixed paragraphs, then
	// one detail paragraph (setInfoDetail) that follows the current view. The
	// panel can be moved, resized, collapsed and scrolled.
	void createInfoPanel(const lightGraphics::ui::Rect& bounds, const std::string& title,
						 const std::vector<std::string>& paragraphs);
	void setInfoDetail(const std::string& text);
	void destroyInfoPanel();

private:
	lightGraphics::ui::Panel* infoPanel = nullptr;
	lightGraphics::ui::Label* infoDetail = nullptr;
};
