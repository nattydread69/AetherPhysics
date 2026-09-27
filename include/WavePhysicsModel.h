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
#include <vector>
#include <memory>
#include <optional>
#include <glm/glm.hpp>

#include "lightVulkanGraphics/ui/Panel.h"
#include "lightVulkanGraphics/ui/widgets/DropDown.h"
#include "lightVulkanGraphics/ui/widgets/RadioButton.h"
#include "lightVulkanGraphics/ui/widgets/Slider.h"

class WavePhysicsModel : public PhysicsModel {
public:
	explicit WavePhysicsModel(lightGraphics::lightVulkanGraphics& app);
	virtual ~WavePhysicsModel();

	virtual void initialize() override;
	virtual void update(float deltaTime) override;
	virtual void cleanup() override;

	enum class MediumMode {
		SOLID,
		VISCOUS,
		LIQUID,
		GAS,
		SUPERSOLID
	};

	enum class WaveType {
		TRANSVERSE,
		LONGITUDINAL
	};

	void setMediumMode(MediumMode mode);
	void setWaveType(WaveType type);
	void setViscosity(float value);
	void setFrequency(float value);

	MediumMode getCurrentMode() const { return mediumMode; }
	WaveType getCurrentWaveType() const { return waveType; }
	float getViscosity() const { return viscosity; }
	float getFrequency() const { return frequency; }

	void printModeMenu() const;
	void printStatus() const;

	// Keyboard controls listed in printModeMenu(). Returns true if the key was handled.
	bool handleKeyPress(int key);

	// UI Management
	void createUIPanel();
	void destroyUIPanel();
	void updateUIValues();
	lightGraphics::ui::Panel* getUIPanel() { return uiPanel; }

private:
	MediumMode mediumMode = MediumMode::SOLID;
	std::optional<MediumMode> pendingMediumMode; // Set by the UI dropdown, applied in update()
	WaveType waveType = WaveType::TRANSVERSE;
	float viscosity = 0.8f;
	float frequency = 0.12f;

	const float SOURCE_X = 70.0f;
	const float AMP = 30.0f;
	const int SUB_STEPS = 2;
	const float SCENE_HEIGHT = 15.0f;
	const float SCENE_WIDTH = 30.0f;

	struct Node {
		int i, j;
		glm::vec3 pos;
		glm::vec3 vel;
		glm::vec3 basePos;
		bool driven;
	};

	struct Spring {
		int nodeA, nodeB;
		float restLength;
	};

	struct Particle {
		glm::vec3 pos;
		glm::vec3 vel;
	};

	struct SolidLattice {
		std::vector<Node> nodes;
		std::vector<Spring> springs;
		float gap;
		int Nx, Ny;
		float spongeStart;
		std::vector<int> objectIndices;
	};

	struct FluidSystem {
		std::vector<Particle> particles;
		float minDist;
		float repelK;
		std::vector<int> objectIndices;
		std::vector<std::vector<size_t>> grid; // Collision grid cells, reused between steps
	};

	std::unique_ptr<SolidLattice> solid;
	std::unique_ptr<FluidSystem> fluid;

	// UI components
	lightGraphics::ui::Panel* uiPanel = nullptr;
	lightGraphics::ui::DropDown* modeDropdown = nullptr;
	lightGraphics::ui::RadioGroup* waveTypeGroup = nullptr;
	lightGraphics::ui::RadioButton* transverseRadio = nullptr;
	lightGraphics::ui::RadioButton* longitudinalRadio = nullptr;
	lightGraphics::ui::Slider* viscositySlider = nullptr;
	lightGraphics::ui::Slider* frequencySlider = nullptr;

	void buildSolid();
	void buildFluid();
	void stepPhysics(float dt);
	void stepSolid(float dt);
	void stepFluid(float dt);

	struct FluidParams {
		float deltaShear;
		float coupling;
		float jitter;
		float drag;
		float soundSpeed;
		float soundAtten;
	};

	FluidParams getFluidParams() const;
};
