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
#include <random>
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
		SUPERSOLID,
		SUPERFLUID,
		VORTEX_SUPERFLUID    // Superfluid with an elastic vortex lattice (Tkachenko-like transverse mode)
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
	void updateInfoText();
	lightGraphics::ui::Panel* getUIPanel() { return uiPanel; }

private:
	MediumMode mediumMode = MediumMode::SOLID;
	std::optional<MediumMode> pendingMediumMode; // Set by the UI dropdown, applied in update()
	WaveType waveType = WaveType::TRANSVERSE;
	float viscosity = 0.8f;
	float frequency = 0.12f;

	// The simulation runs in canvas units: pixels on a 900x500 canvas (y
	// pointing down) and animation frames. Conversion to world units happens
	// only when drawing (toWorld), and to seconds only in update().
	static constexpr float CANVAS_W = 900.0f;
	static constexpr float CANVAS_H = 500.0f;
	static constexpr float PX = 30.0f / CANVAS_W;    // World units per pixel
	static constexpr float FRAMES_PER_SECOND = 60.0f;
	static constexpr int MAX_FRAMES_PER_UPDATE = 4;  // Slow machines run slower rather than unstable
	static constexpr int SUB_STEPS = 4;
	static constexpr float SOURCE_X = 70.0f;         // Driving plane (pixels)
	static constexpr float AMP = 30.0f;              // Displacement amplitude (pixels)

	struct Node {
		glm::vec2 pos;
		glm::vec2 vel;
		glm::vec2 base;
		bool driven;
	};

	struct Spring {
		int a, b;
		float rest;
	};

	struct Particle {
		glm::vec2 pos;
		glm::vec2 vel;
	};

	struct SolidLattice {
		std::vector<Node> nodes;
		std::vector<Spring> springs;
		float gap;
		int Nx, Ny;
		float periodY;                   // Ny * gap: the lattice repeats vertically with this period
		float spongeStart;
		std::vector<int> objectIndices;  // One sphere per node
		std::vector<int> linkSprings;    // Springs drawn as faint links (the non-diagonal ones)
		std::vector<int> linkIndices;    // One segment per drawn link
	};

	struct FluidSystem {
		std::vector<Particle> particles;
		float minDist;
		float repelK;
		std::vector<int> objectIndices;
		std::vector<std::vector<int>> grid; // Collision grid cells, reused between steps
	};

	// Sparse triangular array of quantized vortices, used by VORTEX_SUPERFLUID.
	// It is a second degree of freedom alongside the fluid tracers: the bulk
	// superfluid has no shear rigidity, but the ordered vortex array has an
	// effective shear elasticity and carries a transverse (Tkachenko-like) mode.
	// For now each vortex follows the analytic travelling mode
	// (stepVortexLattice); a dynamically evolved vortex-displacement model can
	// replace that function without changing the storage or drawing.
	struct VortexNode {
		glm::vec2 base;          // Equilibrium position (canvas pixels)
		glm::vec2 pos;           // Current position
		int objectIndex = -1;
	};

	struct VortexLink {
		int a, b;                // Node indices of neighbouring vortices
		int objectIndex = -1;
	};

	struct VortexLattice {
		std::vector<VortexNode> nodes;
		std::vector<VortexLink> links;
		float spacing = 45.0f;   // Along a row (pixels)
		float rowSpacing = 0.0f; // Chosen so an even number of rows fills the canvas height exactly
	};

	float frameAccumulator = 0.0f;      // Fractional animation frames carried between updates
	std::mt19937 rng{std::random_device{}()};
	std::uniform_real_distribution<float> random01{0.0f, 1.0f};
	glm::vec4 previousClearColor{0.0f};

	// Overlays: the driving plane, a line of the average
	// displacement (solid) or velocity (fluid) across the scene, and for
	// transverse waves in fluids the shear penetration envelope
	int sourceLineIndex = -1;
	std::vector<int> probeSegments;
	std::vector<int> envelopeSegments;        // Viscous shear penetration
	std::vector<int> vortexEnvelopeSegments;  // Vortex-elastic transverse reach (VORTEX_SUPERFLUID)

	std::unique_ptr<SolidLattice> solid;
	std::unique_ptr<FluidSystem> fluid;
	std::unique_ptr<VortexLattice> vortices;

	// UI components
	lightGraphics::ui::Panel* uiPanel = nullptr;
	lightGraphics::ui::DropDown* modeDropdown = nullptr;
	lightGraphics::ui::RadioGroup* waveTypeGroup = nullptr;
	lightGraphics::ui::RadioButton* transverseRadio = nullptr;
	lightGraphics::ui::RadioButton* longitudinalRadio = nullptr;
	lightGraphics::ui::Slider* viscositySlider = nullptr;
	lightGraphics::ui::Slider* frequencySlider = nullptr;

	void buildScene();
	void buildSolid();
	void buildFluid();
	void buildVortexLattice();
	void buildOverlays();
	void stepFrame();
	void stepPhysics(float dt);
	void stepSolid(float dt, float disp, float vel);
	void stepFluid(float dt, float omega);
	void stepVortexLattice(float omega);

	void draw();
	void drawSolid();
	void drawFluid();
	void drawVortexLattice();
	void drawProbeLine(const std::vector<glm::vec2>& points, const glm::vec4& color, float widthPx);
	float viewHeight() const;
	glm::vec3 toWorld(glm::vec2 canvasPos, float z = 0.0f) const;
	int addSegment();
	void placeSegment(int index, glm::vec2 a, glm::vec2 b, float widthPx, const glm::vec4& color, float z);

	struct FluidParams {
		float deltaShear;
		float coupling;
		float jitter;
		float drag;
		float soundSpeed;
		float soundAtten;
		// Share of the fluid that viscosity can drag sideways: 1 for ordinary
		// fluids, 0 for a superfluid at absolute zero (it slips past the plate)
		float normalFraction = 1.0f;

		// Vortex-lattice elasticity, a separate restoring mechanism from
		// viscosity. Only VORTEX_SUPERFLUID sets vortexElasticFraction above 0;
		// its bulk still has normalFraction 0 and no shear modulus. All values
		// are visualisation parameters in canvas units (pixels, frames), not
		// calibrated physical quantities.
		float vortexElasticFraction = 0.0f; // Strength of the vortex-carried transverse mode
		float tkachenkoSpeed = 3.0f;        // cT: transverse wave speed scale of the vortex lattice (px/frame)
		float rotationRate = 0.08f;         // Omega: background rotation sustaining the vortices (rad/frame)
		float vortexAtten = 2000.0f;        // e-folding distance of the vortex mode (px); no viscosity involved
	};

	FluidParams getFluidParams() const;
	// Wavenumber of the vortex-lattice mode at the given driving frequency
	// (px^-1), from the Tkachenko-like dispersion in TkachenkoDispersion.h
	float vortexModeWaveNumber(const FluidParams& params, float omega) const;

	// Vortex displacement relative to the source's, chosen for clarity
	static constexpr float VORTEX_AMPLITUDE_SCALE = 0.7f;
};
