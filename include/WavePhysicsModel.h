// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "PhysicsModel.h"
#include <vector>
#include <memory>
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

	// UI Management
	void createUIPanel();
	void destroyUIPanel();
	void updateUIValues();
	lightGraphics::ui::Panel* getUIPanel() { return uiPanel; }

private:
	MediumMode mediumMode = MediumMode::SOLID;
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
