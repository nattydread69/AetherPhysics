// SPDX-License-Identifier: LGPL-3.0-or-later

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
