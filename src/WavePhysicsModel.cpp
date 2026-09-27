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

#include "WavePhysicsModel.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "lightVulkanGraphics/ui/GuiContext.h"
#include "lightVulkanGraphics/ui/Panel.h"
#include "lightVulkanGraphics/ui/widgets/DropDown.h"
#include "lightVulkanGraphics/ui/widgets/RadioButton.h"
#include "lightVulkanGraphics/ui/widgets/Slider.h"
#include "lightVulkanGraphics/ui/widgets/TextBox.h"
#include "lightVulkanGraphics/ui/widgets/Label.h"
#include "lightVulkanGraphics/ui/widgets/Separator.h"

#include <GLFW/glfw3.h>

WavePhysicsModel::WavePhysicsModel(lightGraphics::lightVulkanGraphics& app)
	: PhysicsModel(app, "Wave Physics Model")
{
}

WavePhysicsModel::~WavePhysicsModel()
{
	cleanup();
}

void WavePhysicsModel::initialize()
{
	std::cout << "Initializing Wave Physics Model..." << std::endl;
	buildSolid();
	std::cout << "Wave Physics Model ready. Lattice: " << solid->Nx << "x" << solid->Ny
			  << " (" << solid->nodes.size() << " nodes, " << solid->springs.size()
			  << " springs)" << std::endl;
}

void WavePhysicsModel::cleanup()
{
	std::cout << "[WavePhysicsModel::cleanup] Starting cleanup" << std::endl;
	std::cout << "[WavePhysicsModel::cleanup] Calling destroyUIPanel()..." << std::endl;
	destroyUIPanel();
	std::cout << "[WavePhysicsModel::cleanup] destroyUIPanel() complete" << std::endl;

	// Clear the stored indices
	if (solid) {
		std::cout << "[WavePhysicsModel::cleanup] Clearing solid objectIndices and resetting..." << std::endl;
		solid->objectIndices.clear();
		solid.reset();
		std::cout << "[WavePhysicsModel::cleanup] Solid reset complete" << std::endl;
	}
	if (fluid) {
		std::cout << "[WavePhysicsModel::cleanup] Clearing fluid objectIndices and resetting..." << std::endl;
		fluid->objectIndices.clear();
		fluid.reset();
		std::cout << "[WavePhysicsModel::cleanup] Fluid reset complete" << std::endl;
	}

	// Remove all physics objects from graphics system
	std::cout << "[WavePhysicsModel::cleanup] Removing objects from graphics system..." << std::endl;
	app_.clearObjects();
	std::cout << "[WavePhysicsModel::cleanup] Graphics objects cleared" << std::endl;

	std::cout << "[WavePhysicsModel::cleanup] Cleanup finished" << std::endl;
}

void WavePhysicsModel::setMediumMode(MediumMode mode)
{
	std::cout << "[WavePhysicsModel] setMediumMode() - Safe transaction pattern" << std::endl;

	// Clear all existing objects in one atomic operation
	std::cout << "[WavePhysicsModel] Clearing old objects..." << std::endl;
	cleanup();  // Clears UI and resets object lists

	std::cout << "[WavePhysicsModel] Creating new objects for mode..." << std::endl;
	mediumMode = mode;

	if (mode == MediumMode::SOLID || mode == MediumMode::SUPERSOLID) {
		buildSolid();
	} else {
		buildFluid();
	}

	time = 0.0f;

	// Recreate UI panel after physics objects
	std::cout << "[WavePhysicsModel] Recreating UI panel..." << std::endl;
	createUIPanel();

	std::cout << "[WavePhysicsModel] setMediumMode() complete" << std::endl;
	std::cout << "Switched to mode: ";
	switch (mode) {
		case MediumMode::SOLID: std::cout << "SOLID"; break;
		case MediumMode::VISCOUS: std::cout << "VISCOUS"; break;
		case MediumMode::LIQUID: std::cout << "LIQUID"; break;
		case MediumMode::GAS: std::cout << "GAS"; break;
		case MediumMode::SUPERSOLID: std::cout << "SUPERSOLID"; break;
	}
	std::cout << std::endl;
}

void WavePhysicsModel::setWaveType(WaveType type)
{
	waveType = type;
	time = 0.0f;
	std::cout << "Wave type: " << (type == WaveType::TRANSVERSE ? "TRANSVERSE" : "LONGITUDINAL") << std::endl;
}

void WavePhysicsModel::setViscosity(float value)
{
	viscosity = glm::clamp(value, 0.0f, 1.0f);
}

void WavePhysicsModel::setFrequency(float value)
{
	frequency = glm::clamp(value, 0.05f, 0.3f);
}

void WavePhysicsModel::createUIPanel()
{
	std::cout << "[WavePhysicsModel::createUIPanel] Starting UI panel creation" << std::endl;

	if (uiPanel) {
		std::cout << "[WavePhysicsModel::createUIPanel] Destroying existing panel" << std::endl;
		destroyUIPanel();
	}

	std::cout << "[WavePhysicsModel::createUIPanel] Getting GUI context" << std::endl;
	auto& guiContext = app_.gui();
	std::cout << "[WavePhysicsModel::createUIPanel] Creating panel" << std::endl;
	uiPanel = guiContext.createPanel("Wave Physics Control", {10, 10, 300, 500}, lightGraphics::ui::PanelFlags::None);
	std::cout << "[WavePhysicsModel::createUIPanel] Panel created successfully" << std::endl;

	// Mode selection dropdown
	uiPanel->add<lightGraphics::ui::Label>("Medium Mode");
	modeDropdown = uiPanel->add<lightGraphics::ui::DropDown>(
		"##mode",
		std::vector<std::string>{"Solid", "Viscous", "Liquid", "Gas", "Supersolid"},
		static_cast<int>(mediumMode)
	);
	modeDropdown->setOnChange([this](int index) {
		setMediumMode(static_cast<MediumMode>(index));
		printStatus();
	});

	uiPanel->add<lightGraphics::ui::Separator>();

	// Wave type selection using RadioGroup
	uiPanel->add<lightGraphics::ui::Label>("Wave Type");
	waveTypeGroup = new lightGraphics::ui::RadioGroup();
	waveTypeGroup->setOnChange([this](int value) {
		setWaveType(static_cast<WaveType>(value));
		printStatus();
	});

	transverseRadio = uiPanel->add<lightGraphics::ui::RadioButton>(
		"Transverse (Shear)",
		waveTypeGroup,
		static_cast<int>(WaveType::TRANSVERSE)
	);

	longitudinalRadio = uiPanel->add<lightGraphics::ui::RadioButton>(
		"Longitudinal (Sound)",
		waveTypeGroup,
		static_cast<int>(WaveType::LONGITUDINAL)
	);

	waveTypeGroup->setValue(static_cast<int>(waveType));

	uiPanel->add<lightGraphics::ui::Separator>();

	// Viscosity slider
	uiPanel->add<lightGraphics::ui::Label>("Viscosity");
	viscositySlider = uiPanel->add<lightGraphics::ui::Slider>(
		"##viscosity",
		0.0f, 1.0f, viscosity
	);
	viscositySlider->setOnChange([this](float value) {
		setViscosity(value);
	});

	// Frequency slider
	uiPanel->add<lightGraphics::ui::Label>("Frequency");
	frequencySlider = uiPanel->add<lightGraphics::ui::Slider>(
		"##frequency",
		0.05f, 0.3f, frequency
	);
	frequencySlider->setOnChange([this](float value) {
		setFrequency(value);
	});

	std::cout << "Wave Physics UI panel created" << std::endl;
}

void WavePhysicsModel::destroyUIPanel()
{
	if (uiPanel) {
		auto& guiContext = app_.gui();
		guiContext.destroyPanel(uiPanel);
		uiPanel = nullptr;
		modeDropdown = nullptr;
		if (waveTypeGroup) {
			delete waveTypeGroup;
			waveTypeGroup = nullptr;
		}
		transverseRadio = nullptr;
		longitudinalRadio = nullptr;
		viscositySlider = nullptr;
		frequencySlider = nullptr;
	}
}

void WavePhysicsModel::updateUIValues()
{
	if (!uiPanel) return;

	if (modeDropdown) {
		modeDropdown->setSelectedIndex(static_cast<int>(mediumMode), false);
	}
	if (waveTypeGroup) {
		waveTypeGroup->setValue(static_cast<int>(waveType));
	}
	if (viscositySlider) {
		viscositySlider->setValue(viscosity, false);
	}
	if (frequencySlider) {
		frequencySlider->setValue(frequency, false);
	}
}

bool WavePhysicsModel::handleKeyPress(int key)
{
	constexpr float VISCOSITY_STEP = 0.1f;
	constexpr float FREQUENCY_STEP = 0.01f;

	switch (key) {
		case GLFW_KEY_1: case GLFW_KEY_KP_1: setMediumMode(MediumMode::SOLID); break;
		case GLFW_KEY_2: case GLFW_KEY_KP_2: setMediumMode(MediumMode::VISCOUS); break;
		case GLFW_KEY_3: case GLFW_KEY_KP_3: setMediumMode(MediumMode::LIQUID); break;
		case GLFW_KEY_4: case GLFW_KEY_KP_4: setMediumMode(MediumMode::GAS); break;
		case GLFW_KEY_5: case GLFW_KEY_KP_5: setMediumMode(MediumMode::SUPERSOLID); break;
		case GLFW_KEY_T:
			setWaveType(waveType == WaveType::TRANSVERSE ? WaveType::LONGITUDINAL : WaveType::TRANSVERSE);
			break;
		case GLFW_KEY_EQUAL: case GLFW_KEY_KP_ADD:
			setViscosity(viscosity + VISCOSITY_STEP);
			break;
		case GLFW_KEY_MINUS: case GLFW_KEY_KP_SUBTRACT:
			setViscosity(viscosity - VISCOSITY_STEP);
			break;
		case GLFW_KEY_PERIOD:
			setFrequency(frequency + FREQUENCY_STEP);
			break;
		case GLFW_KEY_COMMA:
			setFrequency(frequency - FREQUENCY_STEP);
			break;
		case GLFW_KEY_M:
			printModeMenu();
			return true;
		default:
			return false;
	}

	updateUIValues();
	printStatus();
	return true;
}

void WavePhysicsModel::printModeMenu() const
{
	std::cout << "\n╔════════════════════════════════════════╗\n"
		<< "║       WAVE PHYSICS MODEL - MODES       ║\n"
		<< "╠════════════════════════════════════════╣\n"
		<< "║  1: SOLID         - Elastic lattice    ║\n"
		<< "║  2: VISCOUS       - Tunable viscosity  ║\n"
		<< "║  3: LIQUID        - Incompressible     ║\n"
		<< "║  4: GAS           - Low density fluid  ║\n"
		<< "║  5: SUPERSOLID    - Lossless demo      ║\n"
		<< "║                                        ║\n"
		<< "║  T:   Toggle wave type (Trans/Long)    ║\n"
		<< "║  +/=: Increase viscosity               ║\n"
		<< "║  -:   Decrease viscosity               ║\n"
		<< "║  >/.: Increase frequency               ║\n"
		<< "║  </,: Decrease frequency               ║\n"
		<< "║  M:   Show this menu                   ║\n"
		<< "║  F1:  Back to density visualizer       ║\n"
		<< "║  Esc: Exit                             ║\n"
		<< "╚════════════════════════════════════════╝\n";
}

void WavePhysicsModel::printStatus() const
{
	std::string modeStr;
	switch (mediumMode) {
		case MediumMode::SOLID: modeStr = "SOLID"; break;
		case MediumMode::VISCOUS: modeStr = "VISCOUS"; break;
		case MediumMode::LIQUID: modeStr = "LIQUID"; break;
		case MediumMode::GAS: modeStr = "GAS"; break;
		case MediumMode::SUPERSOLID: modeStr = "SUPERSOLID"; break;
	}

	std::string waveStr = (waveType == WaveType::TRANSVERSE) ? "TRANSVERSE" : "LONGITUDINAL";

	std::cout << "\n┌─ Wave Physics Status ─┐\n"
		<< "│ Mode:      " << modeStr << "\n"
		<< "│ Wave Type: " << waveStr << "\n"
		<< "│ Viscosity: " << static_cast<int>(viscosity * 100) << "%\n"
		<< "│ Frequency: " << frequency << "\n"
		<< "└────────────────────────┘\n";
}

void WavePhysicsModel::buildSolid()
{
	solid = std::make_unique<SolidLattice>();
	const float gap = 0.4f;
	const float startX = SOURCE_X / 450.0f * SCENE_WIDTH - SCENE_WIDTH * 0.5f;
	const float endX = SCENE_WIDTH * 0.5f - 0.5f;
	const int Nx = static_cast<int>((endX - startX) / gap) + 1;
	const int Ny = static_cast<int>(SCENE_HEIGHT / gap);

	solid->gap = gap;
	solid->Nx = Nx;
	solid->Ny = Ny;
	solid->spongeStart = SCENE_WIDTH * 0.41f - SCENE_WIDTH * 0.5f;
	solid->nodes.reserve(Nx * Ny);
	solid->objectIndices.reserve(Nx * Ny);

	std::cout << "Building solid lattice: " << Nx << "x" << Ny << " = " << (Nx*Ny) << " nodes" << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> colorVar(-0.1f, 0.1f);

	for (int i = 0; i < Nx; ++i) {
		for (int j = 0; j < Ny; ++j) {
			const float x = startX + i * gap;
			const float y = (j + 0.5f) * gap - SCENE_HEIGHT * 0.5f;

			Node node;
			node.i = i;
			node.j = j;
			node.pos = glm::vec3(x, y, 0.0f);
			node.basePos = node.pos;
			node.vel = glm::vec3(0.0f);
			node.driven = (i == 0);

			solid->nodes.push_back(node);

			if (node.driven) {
				app_.addObject(
					lightGraphics::ShapeType::SPHERE,
					node.pos,
					glm::vec3(0.08f),
					glm::vec4(1.0f, 0.65f, 0.0f, 1.0f),
					glm::quat(1, 0, 0, 0),
					"Driven Node " + std::to_string(i) + ":" + std::to_string(j),
					0.1f
				);
			} else {
				glm::vec4 color(0.3f, 0.6f, 0.95f, 0.8f);
				color.x += colorVar(gen);
				color.y += colorVar(gen);
				color.z += colorVar(gen);

				app_.addObject(
					lightGraphics::ShapeType::SPHERE,
					node.pos,
					glm::vec3(0.06f),
					glm::vec4(glm::clamp(glm::vec3(color), 0.0f, 1.0f), 0.8f),
					glm::quat(1, 0, 0, 0),
					"Node " + std::to_string(i) + ":" + std::to_string(j),
					0.1f
				);
			}
			solid->objectIndices.push_back(app_.getObjectCount() - 1);
		}
	}

	auto nodeIdx = [Ny](int i, int j) { return i * Ny + j; };
	const float diag = gap * glm::sqrt(2.0f);

	for (int i = 0; i < Nx; ++i) {
		for (int j = 0; j < Ny; ++j) {
			const int a = nodeIdx(i, j);

			if (i + 1 < Nx) {
				solid->springs.push_back({a, nodeIdx(i + 1, j), gap});
			}
			solid->springs.push_back({a, nodeIdx(i, (j + 1) % Ny), gap});

			if (i + 1 < Nx) {
				solid->springs.push_back({a, nodeIdx(i + 1, (j + 1) % Ny), diag});
				solid->springs.push_back({a, nodeIdx(i + 1, (j - 1 + Ny) % Ny), diag});
			}
		}
	}

	std::cout << "Solid created: " << solid->nodes.size() << " nodes, "
		<< solid->objectIndices.size() << " object indices, "
		<< solid->springs.size() << " springs" << std::endl;
}

void WavePhysicsModel::buildFluid()
{
	fluid = std::make_unique<FluidSystem>();

	int count = 1000;
	if (mediumMode == MediumMode::VISCOUS) count = 1200;
	if (mediumMode == MediumMode::GAS) count = 600;

	const float minX = SOURCE_X / 450.0f * SCENE_WIDTH - SCENE_WIDTH * 0.5f + 0.5f;
	fluid->particles.reserve(count);
	fluid->objectIndices.reserve(count);
	fluid->minDist = (mediumMode == MediumMode::GAS) ? 0.12f : 0.14f;
	fluid->repelK = (mediumMode == MediumMode::GAS) ? 0.55f : 0.70f;

	std::cout << "Building fluid with " << count << " particles" << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> posDist(-SCENE_WIDTH * 0.5f, SCENE_WIDTH * 0.5f);
	std::uniform_real_distribution<float> velDist(-0.3f, 0.3f);

	for (int i = 0; i < count; ++i) {
		Particle p;
		p.pos = glm::vec3(
			minX + (SCENE_WIDTH - minX) * (i % 100) / 100.0f,
			posDist(gen),
			0.0f
		);
		p.vel = glm::vec3(velDist(gen), velDist(gen), 0.0f);

		fluid->particles.push_back(p);

		glm::vec4 color(0.3f, 0.6f, 0.95f, 0.7f);
		app_.addObject(
			lightGraphics::ShapeType::CUBE,
			p.pos,
			glm::vec3(0.04f),
			color,
			glm::quat(1, 0, 0, 0),
			"Particle " + std::to_string(i),
			0.04f
		);
		fluid->objectIndices.push_back(app_.getObjectCount() - 1);
	}

	std::cout << "Fluid created: " << fluid->particles.size() << " particles, "
		<< fluid->objectIndices.size() << " object indices" << std::endl;
}

void WavePhysicsModel::update(float deltaTime)
{
	static int frameCount = 0;
	static auto lastPrintTime = std::chrono::high_resolution_clock::now();
	auto frameStartTime = std::chrono::high_resolution_clock::now();

	time += deltaTime;
	const float dt = deltaTime / SUB_STEPS;

	auto physicsStartTime = std::chrono::high_resolution_clock::now();
	for (int s = 0; s < SUB_STEPS; ++s) {
		stepPhysics(dt);
	}
	auto physicsEndTime = std::chrono::high_resolution_clock::now();
	float physicsMs = std::chrono::duration<float, std::milli>(physicsEndTime - physicsStartTime).count();

	auto updateStartTime = std::chrono::high_resolution_clock::now();
	try {
		if (solid && !solid->objectIndices.empty() && solid->objectIndices.size() == solid->nodes.size()) {
			for (size_t i = 0; i < solid->nodes.size(); ++i) {
				app_.setObjectPosition(solid->objectIndices[i], solid->nodes[i].pos);

				float disp = (waveType == WaveType::TRANSVERSE)
					? (solid->nodes[i].pos.y - solid->nodes[i].basePos.y)
					: (solid->nodes[i].pos.x - solid->nodes[i].basePos.x);
				disp = glm::clamp(disp / (AMP * 1.2f / 450.0f * SCENE_WIDTH), -1.0f, 1.0f);

				float r = 0.3f + 0.35f * disp;
				float g = 0.6f;
				float b = 0.95f - 0.35f * disp;

				app_.setObjectColor(solid->objectIndices[i],
					glm::vec4(glm::clamp(glm::vec3(r, g, b), 0.0f, 1.0f), 0.8f));
			}
		}

		if (fluid && !fluid->objectIndices.empty() && fluid->objectIndices.size() == fluid->particles.size()) {
			for (size_t i = 0; i < fluid->particles.size(); ++i) {
				app_.setObjectPosition(fluid->objectIndices[i], fluid->particles[i].pos);

				float vcomp = (waveType == WaveType::TRANSVERSE)
					? fluid->particles[i].vel.y
					: fluid->particles[i].vel.x;
				float omega = frequency;
				float vScale = std::max(0.001f, AMP * omega / 450.0f * SCENE_WIDTH);
				float s = glm::clamp(vcomp / (vScale * 1.4f), -1.0f, 1.0f);

				float t = (s + 1.0f) * 0.5f;
				float r = glm::mix(60.0f, 240.0f, t) / 255.0f;
				float g = glm::mix(120.0f, 80.0f, t) / 255.0f;
				float b = glm::mix(240.0f, 60.0f, t) / 255.0f;

				app_.setObjectColor(fluid->objectIndices[i], glm::vec4(r, g, b, 0.7f));
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Exception in update: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown exception in update" << std::endl;
	}
	auto updateEndTime = std::chrono::high_resolution_clock::now();
	float updateMs = std::chrono::duration<float, std::milli>(updateEndTime - updateStartTime).count();

	auto frameEndTime = std::chrono::high_resolution_clock::now();
	float frameMs = std::chrono::duration<float, std::milli>(frameEndTime - frameStartTime).count();

	frameCount++;
	auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration<float>(now - lastPrintTime).count() >= 1.0f) {
		float fps = frameCount / std::chrono::duration<float>(now - lastPrintTime).count();
		std::cout << "[PERF] FPS: " << std::fixed << std::setprecision(1) << fps
			<< " | Frame: " << std::setprecision(2) << frameMs << "ms"
			<< " | Physics: " << physicsMs << "ms"
			<< " | Update: " << updateMs << "ms" << std::endl;
		frameCount = 0;
		lastPrintTime = now;
	}
}

void WavePhysicsModel::stepPhysics(float dt)
{
	if (solid) {
		stepSolid(dt);
	} else if (fluid) {
		stepFluid(dt);
	}
}

void WavePhysicsModel::stepSolid(float dt)
{
	const float omega = frequency;
	const float disp = AMP * std::sin(time * omega) / 450.0f * SCENE_WIDTH;
	const float vel = AMP * omega * std::cos(time * omega) / 450.0f * SCENE_WIDTH;

	const float kSpring = (mediumMode == MediumMode::SUPERSOLID) ? 0.09f : 0.085f;
	const float kDamp = (mediumMode == MediumMode::SUPERSOLID) ? 0.0f : 0.030f;

	for (auto& node : solid->nodes) {
		if (!node.driven) continue;

		if (waveType == WaveType::TRANSVERSE) {
			node.pos.x = node.basePos.x;
			node.pos.y = node.basePos.y + disp;
			node.vel.x = 0.0f;
			node.vel.y = vel;
		} else {
			node.pos.x = node.basePos.x + disp;
			node.pos.y = node.basePos.y;
			node.vel.x = vel;
			node.vel.y = 0.0f;
		}
	}

	for (const auto& spring : solid->springs) {
		auto& a = solid->nodes[spring.nodeA];
		auto& b = solid->nodes[spring.nodeB];

		glm::vec3 delta = b.pos - a.pos;
		float dist = glm::length(delta);

		if (dist < 1e-6f) continue;

		float ext = dist - spring.restLength;
		glm::vec3 n = delta / dist;
		float dvrel = glm::dot(b.vel - a.vel, n);
		float f = kSpring * ext + kDamp * dvrel;
		glm::vec3 force = f * n;

		if (!a.driven) {
			a.vel += force * dt;
		}
		if (!b.driven) {
			b.vel -= force * dt;
		}
	}

	const float spongeStart = solid->spongeStart;
	for (auto& node : solid->nodes) {
		if (node.driven) continue;

		const float linDamp = (mediumMode == MediumMode::SUPERSOLID) ? 0.9997f : 0.9965f;
		node.vel *= linDamp;

		if (node.pos.x > spongeStart) {
			float t = glm::clamp((node.pos.x - spongeStart) / (SCENE_WIDTH * 0.5f - spongeStart), 0.0f, 1.0f);
			float sponge = (mediumMode == MediumMode::SUPERSOLID) ? (1.0f - 0.06f * t * t) : (1.0f - 0.18f * t * t);
			node.vel *= sponge;
		}

		node.pos += node.vel * dt;

		if (node.pos.y < -SCENE_HEIGHT * 0.5f) node.pos.y += SCENE_HEIGHT;
		if (node.pos.y >= SCENE_HEIGHT * 0.5f) node.pos.y -= SCENE_HEIGHT;
	}
}

WavePhysicsModel::FluidParams WavePhysicsModel::getFluidParams() const
{
	FluidParams p;

	if (mediumMode == MediumMode::VISCOUS) {
		p.deltaShear = glm::mix(35.0f, 140.0f, viscosity);
		p.coupling = glm::mix(0.70f, 0.92f, viscosity);
		p.jitter = glm::mix(0.06f, 0.02f, viscosity);
		p.drag = glm::mix(0.03f, 0.08f, viscosity);
		p.soundSpeed = 6.5f;
		p.soundAtten = glm::mix(520.0f, 240.0f, viscosity);
	} else if (mediumMode == MediumMode::LIQUID) {
		p.deltaShear = 18.0f;
		p.coupling = 0.55f;
		p.jitter = 0.10f;
		p.drag = 0.02f;
		p.soundSpeed = 7.5f;
		p.soundAtten = 700.0f;
	} else if (mediumMode == MediumMode::GAS) {
		p.deltaShear = 10.0f;
		p.coupling = 0.35f;
		p.jitter = 0.55f;
		p.drag = 0.01f;
		p.soundSpeed = 5.5f;
		p.soundAtten = 520.0f;
	} else {
		p.deltaShear = 25.0f;
		p.coupling = 0.6f;
		p.jitter = 0.12f;
		p.drag = 0.02f;
		p.soundSpeed = 7.0f;
		p.soundAtten = 600.0f;
	}

	return p;
}

void WavePhysicsModel::stepFluid(float dt)
{
	static std::mt19937 gen(std::random_device{}());
	static std::uniform_real_distribution<float> jitterDist(-0.5f, 0.5f);

	const float omega = frequency;
	const float disp = AMP * std::sin(time * omega) / 450.0f * SCENE_WIDTH;
	const float vel = AMP * omega * std::cos(time * omega) / 450.0f * SCENE_WIDTH;
	const float sourceX = SOURCE_X / 450.0f * SCENE_WIDTH - SCENE_WIDTH * 0.5f;

	const FluidParams params = getFluidParams();
	const float kShear = 1.0f / std::max(8.0f, params.deltaShear);
	const float kSound = omega / std::max(2.0f, params.soundSpeed);

	for (auto& p : fluid->particles) {
		float xDist = std::max(0.0f, p.pos.x - sourceX);
		float vxField = 0.0f, vyField = 0.0f;

		if (waveType == WaveType::TRANSVERSE) {
			float env = std::exp(-kShear * xDist);
			float phase = (omega * time) - (kShear * xDist);
			vyField = (AMP * omega) * std::cos(phase) * env / 450.0f * SCENE_WIDTH;
		} else {
			float env = std::exp(-xDist / std::max(0.8f, params.soundAtten / 100.0f));
			float phase = (omega * time) - (kSound * xDist);
			vxField = (AMP * omega) * std::cos(phase) * env / 450.0f * SCENE_WIDTH;
		}

		p.vel.x += (vxField - p.vel.x) * params.coupling * dt;
		p.vel.y += (vyField - p.vel.y) * params.coupling * dt;

		float j = params.jitter * dt;
		p.vel.x += jitterDist(gen) * j;
		p.vel.y += jitterDist(gen) * j;

		float damp = std::exp(-params.drag * dt);
		p.vel *= damp;
	}

	const float minD = fluid->minDist;
	const float minDSq = minD * minD;
	const float repelK = fluid->repelK;
	const float SCENE_H = SCENE_HEIGHT;

	// Spatial partitioning: divide into grid cells
	const float cellSize = minD * 2.0f;
	const int gridWidth = (int)std::ceil(SCENE_WIDTH / cellSize) + 1;
	const int gridHeight = (int)std::ceil(SCENE_HEIGHT / cellSize) + 1;
	std::vector<std::vector<size_t>> grid(gridWidth * gridHeight);

	// Populate grid
	for (size_t i = 0; i < fluid->particles.size(); ++i) {
		const auto& p = fluid->particles[i];
		int gx = (int)std::floor((p.pos.x + SCENE_WIDTH * 0.5f) / cellSize);
		int gy = (int)std::floor((p.pos.y + SCENE_HEIGHT * 0.5f) / cellSize);
		gx = std::max(0, std::min(gx, gridWidth - 1));
		gy = std::max(0, std::min(gy, gridHeight - 1));
		grid[gy * gridWidth + gx].push_back(i);
	}

	// Check collisions only between nearby particles
	for (size_t i = 0; i < fluid->particles.size(); ++i) {
		auto& pi = fluid->particles[i];
		int gx = (int)std::floor((pi.pos.x + SCENE_WIDTH * 0.5f) / cellSize);
		int gy = (int)std::floor((pi.pos.y + SCENE_HEIGHT * 0.5f) / cellSize);
		gx = std::max(0, std::min(gx, gridWidth - 1));
		gy = std::max(0, std::min(gy, gridHeight - 1));

		// Check this cell and 8 neighbors
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				int nx = gx + dx;
				int ny = gy + dy;
				if (nx >= 0 && nx < gridWidth && ny >= 0 && ny < gridHeight) {
					for (size_t j : grid[ny * gridWidth + nx]) {
						if (i >= j) continue; // Only check each pair once
						auto& pj = fluid->particles[j];

						glm::vec3 delta = pj.pos - pi.pos;
						delta.y = std::fmod(delta.y + SCENE_H * 0.5f, SCENE_H) - SCENE_H * 0.5f;

						float d2 = glm::dot(delta, delta);
						if (d2 > 0.0001f && d2 < minDSq) {
							float d = std::sqrt(d2);
							float push = (1.0f - d / minD);
							glm::vec3 f = (delta / d) * push * repelK * dt;
							pi.vel -= f;
							pj.vel += f;
						}
					}
				}
			}
		}
	}

	const float right = SCENE_WIDTH * 0.5f + 0.4f;
	const float left = sourceX - 0.8f;
	static std::uniform_real_distribution<float> posDist(0.0f, 1.0f);

	for (auto& p : fluid->particles) {
		p.pos += p.vel * dt;

		if (p.pos.y < -SCENE_HEIGHT * 0.5f) p.pos.y += SCENE_HEIGHT;
		if (p.pos.y >= SCENE_HEIGHT * 0.5f) p.pos.y -= SCENE_HEIGHT;

		if (p.pos.x > right) {
			p.pos.x = sourceX + 0.5f + posDist(gen) * 0.4f;
			p.pos.y = posDist(gen) * SCENE_HEIGHT - SCENE_HEIGHT * 0.5f;
			p.vel *= 0.2f;
		}
		if (p.pos.x < left) {
			p.pos.x = SCENE_WIDTH * 0.5f - 0.5f - posDist(gen) * 0.4f;
			p.pos.y = posDist(gen) * SCENE_HEIGHT - SCENE_HEIGHT * 0.5f;
			p.vel *= 0.2f;
		}
	}
}
