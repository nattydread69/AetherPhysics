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

#include "PhysicsApp.h"

#include "AetherDensityVisualizer.h"
#include "AetherPhysicsModel.h"
#include "WavePhysicsModel.h"

#include <chrono>
#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>

// Static keyboard state variables
bool PhysicsApp::keysPressed[GLFW_KEY_LAST + 1] = {false};
bool PhysicsApp::keysJustPressed[GLFW_KEY_LAST + 1] = {false};
bool PhysicsApp::keysJustReleased[GLFW_KEY_LAST + 1] = {false};

PhysicsApp::PhysicsApp(int argc, char* argv[])
	: app("Aether Physics")
{
	if (app.getWindowPointer() == nullptr)
	{
		throw std::runtime_error("Graphics initialization failed");
	}

	if (argc > 1 && std::string(argv[1]) != "aether")
	{
		std::cout << "Unknown model: " << argv[1] << " (using aether)" << std::endl;
	}

	createModel(currentModel);
	app.setCameraLookAt(
		glm::vec3(4.0f, 2.0f, -8.0f),
		glm::vec3(0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	app.finalizeScene();
	showModelMenu();

	app.setUpdateCallback([this](float) {
		handleKeyboardInput();

		static auto lastTime = std::chrono::high_resolution_clock::now();
		const auto currentTime = std::chrono::high_resolution_clock::now();
		const float deltaTime =
			std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;

		if (currentPhysicsModel)
		{
			currentPhysicsModel->update(deltaTime);
		}
	});

	app.run();
}

PhysicsApp::~PhysicsApp()
{
	if (currentPhysicsModel)
	{
		currentPhysicsModel->cleanup();
	}
}

void PhysicsApp::createModel(ModelType model)
{
	switch (model)
	{
	//case ModelType::Aether:
	//	currentPhysicsModel = std::make_unique<AetherPhysicsModel>(app, "Aether Physics");
	//	break;
	case ModelType::AETHER_DENSITY_VISUALIZER:
		std::cout << "Loading Aether Density Visualizer model..." << std::endl;
		currentPhysicsModel = std::make_unique<AetherDensityVisualizer>(app);
		break;
	case ModelType::WAVE_PHYSICS:
		std::cout << "Loading Wave Physics model..." << std::endl;
		currentPhysicsModel = std::make_unique<WavePhysicsModel>(app);
		break;
	}

	currentPhysicsModel->initialize();
}

void PhysicsApp::switchModel(ModelType model)
{
	if (model == currentModel)
	{
		return;
	}

	std::cout << "[PhysicsApp] Switching model..." << std::endl;

	std::cout << "[PhysicsApp] Calling cleanup()..." << std::endl;
	currentPhysicsModel->cleanup();
	std::cout << "[PhysicsApp] Cleanup complete" << std::endl;

	std::cout << "[PhysicsApp] Resetting model pointer..." << std::endl;
	currentPhysicsModel.reset();
	std::cout << "[PhysicsApp] Calling clearObjects()..." << std::endl;
	app.clearObjects();
	std::cout << "[PhysicsApp] clearObjects() complete" << std::endl;

	currentModel = model;

	std::cout << "[PhysicsApp] Creating new model..." << std::endl;
	createModel(model);
	std::cout << "[PhysicsApp] Model creation complete" << std::endl;

	// Don't call finalizeScene() during mode switch - it's only for initial setup
	// The scene is already finalized; we're just updating objects within it

	// Show mode-specific menu on switch and create UI
	if (model == ModelType::WAVE_PHYSICS) {
		WavePhysicsModel* waveMdl = dynamic_cast<WavePhysicsModel*>(currentPhysicsModel.get());
		if (waveMdl) {
			// Position camera perpendicular to view the wave plane straight-on
			app.setCameraLookAt(
				glm::vec3(0.0f, 0.0f, 20.0f),
				glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			waveMdl->createUIPanel();
			showWavePhysicsMenu();
		}
	}
}

void PhysicsApp::handleKeyboardInput()
{
	GLFWwindow* window = app.getWindowPointer();
	if (window == nullptr)
	{
		return;
	}

	// Update key state first so the edge flags are valid for everything below.
	// GLFW_KEY_SPACE is the lowest valid key code.
	for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
	{
		const bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
		keysJustPressed[key] = pressed && !keysPressed[key];
		keysJustReleased[key] = !pressed && keysPressed[key];
		keysPressed[key] = pressed;
	}

	const bool altHeld = keysPressed[GLFW_KEY_LEFT_ALT] || keysPressed[GLFW_KEY_RIGHT_ALT];

	if (keysJustPressed[GLFW_KEY_ESCAPE])
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		return;
	}

	if (keysJustPressed[GLFW_KEY_F1])
	{
		switchModel(ModelType::AETHER_DENSITY_VISUALIZER);
		return;
	}
	if (keysJustPressed[GLFW_KEY_F2])
	{
		switchModel(ModelType::WAVE_PHYSICS);
		return;
	}
	if (keysJustPressed[GLFW_KEY_F3])
	{
		showModelMenu();
	}

	if (!currentPhysicsModel)
	{
		return;
	}

	// Ray length controls (only for Aether Density Visualizer)
	if (currentModel == ModelType::AETHER_DENSITY_VISUALIZER)
	{
		AetherDensityVisualizer* densityViz = dynamic_cast<AetherDensityVisualizer*>(currentPhysicsModel.get());
		if (densityViz)
		{
			if (keysJustPressed[GLFW_KEY_EQUAL] || keysJustPressed[GLFW_KEY_KP_ADD])
			{
				densityViz->setRayLength(densityViz->maxRaySegments + 10);
			}
			if (keysJustPressed[GLFW_KEY_MINUS] || keysJustPressed[GLFW_KEY_KP_SUBTRACT])
			{
				densityViz->setRayLength(densityViz->maxRaySegments - 10);
			}
		}
	}

	// Wave Physics controls
	if (currentModel == ModelType::WAVE_PHYSICS)
	{
		WavePhysicsModel* waveMdl = dynamic_cast<WavePhysicsModel*>(currentPhysicsModel.get());
		if (waveMdl)
		{
			if (altHeld && keysJustPressed[GLFW_KEY_W])
			{
				showWavePhysicsMenu();
				return;
			}
			for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
			{
				if (keysJustPressed[key] && waveMdl->handleKeyPress(key))
				{
					// One action per frame: a medium change rebuilds the model and its UI.
					break;
				}
			}
		}
	}
}

void PhysicsApp::showModelMenu() const
{
	std::cout << "\nAether Physics\n"
		<< "F1: Aether Density Visualizer\n"
		<< "F2: Wave Physics (Solid/Fluid)\n"
		<< "F3: Show this menu\n"
		<< "Alt+W: Wave Physics Controls (when in Wave Physics)\n"
		<< "Esc: Exit\n";
}

void PhysicsApp::showWavePhysicsMenu() const
{
	if (currentModel != ModelType::WAVE_PHYSICS || !currentPhysicsModel) {
		return;
	}

	WavePhysicsModel* waveMdl = dynamic_cast<WavePhysicsModel*>(currentPhysicsModel.get());
	if (waveMdl) {
		waveMdl->printModeMenu();
		waveMdl->printStatus();
	}
}
