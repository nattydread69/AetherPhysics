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

#include "PhysicsModel.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "lightVulkanGraphics/ui/GuiContext.h"
#include "lightVulkanGraphics/ui/Panel.h"
#include "lightVulkanGraphics/ui/widgets/Label.h"
#include "lightVulkanGraphics/ui/widgets/Separator.h"

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

void PhysicsModel::createInfoPanel(const lightGraphics::ui::Rect& bounds, const std::string& title,
								   const std::vector<std::string>& paragraphs)
{
	destroyInfoPanel();

	infoPanel = app_.gui().createPanel(title, bounds, lightGraphics::ui::PanelFlags::Default);

	// Wrapped labels don't break on newlines, so each paragraph is its own label
	for (const std::string& paragraph : paragraphs) {
		infoPanel->add<lightGraphics::ui::Label>(paragraph)->setWordWrap(true);
	}
	infoPanel->add<lightGraphics::ui::Separator>();
	infoDetail = infoPanel->add<lightGraphics::ui::Label>("");
	infoDetail->setWordWrap(true);
}

void PhysicsModel::setInfoDetail(const std::string& text)
{
	if (infoDetail) {
		infoDetail->setText(text);
	}
}

void PhysicsModel::destroyInfoPanel()
{
	if (infoPanel) {
		app_.gui().destroyPanel(infoPanel);
		infoPanel = nullptr;
		infoDetail = nullptr;
	}
}
