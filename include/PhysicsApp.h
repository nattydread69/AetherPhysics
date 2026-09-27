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

#include <memory>
#include <GLFW/glfw3.h>

class PhysicsApp
{
public:
	PhysicsApp(int argc, char* argv[]);
	~PhysicsApp();

private:
	enum class ModelType {
		Aether,
		AETHER_DENSITY_VISUALIZER,
		WAVE_PHYSICS,
	};

	void createModel(ModelType model);
	void switchModel(ModelType model);
	void handleKeyboardInput();
	static bool keysPressed[GLFW_KEY_LAST + 1];
	static bool keysJustPressed[GLFW_KEY_LAST + 1];
	static bool keysJustReleased[GLFW_KEY_LAST + 1];
	void showModelMenu() const;
	void showWavePhysicsMenu() const;

	lightGraphics::lightVulkanGraphics app;
	std::unique_ptr<PhysicsModel> currentPhysicsModel;
	ModelType currentModel = ModelType::AETHER_DENSITY_VISUALIZER;
};
