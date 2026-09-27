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
#include <cmath>
#include <algorithm>
#include <random>
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
	app_.setClearColor(previousClearColor);
}

void WavePhysicsModel::initialize()
{
	std::cout << "Initializing Wave Physics Model..." << std::endl;

	// The original's canvas background (#1e293b)
	previousClearColor = app_.getClearColor();
	app_.setClearColor(glm::vec4(30.0f / 255.0f, 41.0f / 255.0f, 59.0f / 255.0f, 1.0f));

	buildScene();
	std::cout << "Wave Physics Model ready. Lattice: " << solid->Nx << "x" << solid->Ny
			  << " (" << solid->nodes.size() << " nodes, " << solid->springs.size()
			  << " springs)" << std::endl;
}

void WavePhysicsModel::cleanup()
{
	destroyUIPanel();
	solid.reset();
	fluid.reset();
	sourceLineIndex = -1;
	probeSegments.clear();
	envelopeSegments.clear();

	// Remove all physics objects from the graphics system
	app_.clearObjects();
}

void WavePhysicsModel::setMediumMode(MediumMode mode)
{
	// Clear the old objects and UI, then rebuild both for the new medium
	cleanup();
	mediumMode = mode;

	// Viscosity presets per medium, as in the original's setMode (it only
	// affects the Viscous medium's parameters)
	if (mode == MediumMode::VISCOUS) viscosity = 0.8f;
	else if (mode == MediumMode::LIQUID) viscosity = 0.2f;
	else if (mode == MediumMode::GAS || mode == MediumMode::SUPERSOLID) viscosity = 0.0f;

	buildScene();

	time = 0.0f;
	frameAccumulator = 0.0f;
	createUIPanel();
}

void WavePhysicsModel::setWaveType(WaveType type)
{
	waveType = type;
	time = 0.0f;
	updateInfoText();
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

	if (uiPanel) {
		destroyUIPanel();
	}

	auto& guiContext = app_.gui();
	uiPanel = guiContext.createPanel("Wave Physics Control", {10, 10, 300, 310}, lightGraphics::ui::PanelFlags::None);

	// Mode selection dropdown
	uiPanel->add<lightGraphics::ui::Label>("Medium Mode");
	modeDropdown = uiPanel->add<lightGraphics::ui::DropDown>(
		"",
		std::vector<std::string>{"Solid", "Viscous", "Liquid", "Gas", "Supersolid"},
		static_cast<int>(mediumMode)
	);
	modeDropdown->setOnChange([this](int index) {
		// Changing medium destroys this panel, which must not happen inside one of
		// its own widget callbacks, so apply it at the start of the next update()
		pendingMediumMode = static_cast<MediumMode>(index);
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
		"",
		0.0f, 1.0f, viscosity
	);
	viscositySlider->setOnChange([this](float value) {
		setViscosity(value);
	});

	// Frequency slider
	uiPanel->add<lightGraphics::ui::Label>("Frequency");
	frequencySlider = uiPanel->add<lightGraphics::ui::Slider>(
		"",
		0.05f, 0.3f, frequency
	);
	frequencySlider->setOnChange([this](float value) {
		setFrequency(value);
	});

	createInfoPanel({10, 330, 300, 380}, "About this view", {
		"A plate on the left shakes the medium and the disturbance travels to the "
		"right. Light is a transverse wave, so this asks the classic aether question: "
		"what kind of medium could carry it? A transverse (shear) wave only travels in "
		"a medium that springs back when sheared, like a solid.",
		"The line across the middle is the average displacement (solids) or velocity "
		"(fluids) at each distance from the plate. Colour shows which way each part is "
		"moving. In the solids the wave emerges from the springs; in the fluids a "
		"prescribed wave field moves the particles.",
		"Based on Chantal Roth's Wave Physics Lab.",
	});
	updateInfoText();

}

void WavePhysicsModel::destroyUIPanel()
{
	destroyInfoPanel();
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

	// Media use F5-F9 (and keypad 1-5): the library already binds the top-row
	// digits 1-4 to its own render modes (wireframe, unlit, ...)
	switch (key) {
		case GLFW_KEY_F5: case GLFW_KEY_KP_1: setMediumMode(MediumMode::SOLID); break;
		case GLFW_KEY_F6: case GLFW_KEY_KP_2: setMediumMode(MediumMode::VISCOUS); break;
		case GLFW_KEY_F7: case GLFW_KEY_KP_3: setMediumMode(MediumMode::LIQUID); break;
		case GLFW_KEY_F8: case GLFW_KEY_KP_4: setMediumMode(MediumMode::GAS); break;
		case GLFW_KEY_F9: case GLFW_KEY_KP_5: setMediumMode(MediumMode::SUPERSOLID); break;
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

void WavePhysicsModel::updateInfoText()
{
	const bool transverse = (waveType == WaveType::TRANSVERSE);
	std::string text = transverse
		? "Transverse wave (T to switch): the medium moves across the direction of travel. "
		: "Longitudinal wave (T to switch): the medium moves along the direction of travel, "
		  "making compressions like sound. ";

	switch (mediumMode) {
		case MediumMode::SOLID:
			text += transverse
				? "Solid: supported. Shear travels as an elastic wave through the spring lattice."
				: "Solid: supported. Compression travels as an elastic wave through the spring lattice.";
			text += " The right-hand edge absorbs the wave so it doesn't reflect.";
			break;
		case MediumMode::SUPERSOLID:
			text += "Supersolid: near-lossless. The same lattice with almost no damping, so "
				"waves persist much longer.";
			break;
		case MediumMode::VISCOUS:
			text += transverse
				? "Viscous fluid: strongly attenuated. The shear motion decays exponentially "
				  "with distance; the faint curve near the bottom shows how fast. More "
				  "viscosity (slider or +/-) lets it reach further."
				: "Viscous fluid: supported. A sound-like wave travels, damped by viscosity.";
			break;
		case MediumMode::LIQUID:
			text += transverse
				? "Liquid: not supported. With no shear rigidity the motion stays near the "
				  "plate and dies fast."
				: "Liquid: supported. Pressure (sound) waves travel through it.";
			break;
		case MediumMode::GAS:
			text += transverse
				? "Gas: not supported. No shear rigidity; mostly random thermal motion."
				: "Gas: supported. Sound travels through compressions and rarefactions.";
			break;
	}

	setInfoDetail(text);
}

void WavePhysicsModel::printModeMenu() const
{
	std::cout << "\n╔════════════════════════════════════════╗\n"
		<< "║       WAVE PHYSICS MODEL - MODES       ║\n"
		<< "╠════════════════════════════════════════╣\n"
		<< "║  F5: SOLID        - Elastic lattice    ║\n"
		<< "║  F6: VISCOUS      - Tunable viscosity  ║\n"
		<< "║  F7: LIQUID       - Incompressible     ║\n"
		<< "║  F8: GAS          - Low density fluid  ║\n"
		<< "║  F9: SUPERSOLID   - Lossless demo      ║\n"
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

// ---------------------------------------------------------------------------
// Port of Chantal Roth's Wave Physics Lab. Physics runs in her units (canvas
// pixels, y down; time in animation frames) with her constants; only draw()
// converts to world units.
// ---------------------------------------------------------------------------

namespace {

// Minimal signed delta in a periodic domain (her wrapDelta). The extra
// "+ period" keeps negative deltas right, which std::fmod alone does not.
float wrapDelta(float d, float period)
{
	return std::fmod(std::fmod(d + period * 0.5f, period) + period, period) - period * 0.5f;
}

float wrapPosition(float y, float period)
{
	return std::fmod(std::fmod(y, period) + period, period);
}

// s in [-1, 1] -> blue .. red (her divergingColor)
glm::vec4 divergingColor(float s, float alpha)
{
	const float t = (glm::clamp(s, -1.0f, 1.0f) + 1.0f) * 0.5f;
	return glm::vec4(glm::mix(60.0f, 240.0f, t) / 255.0f,
					 glm::mix(120.0f, 80.0f, t) / 255.0f,
					 glm::mix(240.0f, 60.0f, t) / 255.0f,
					 alpha);
}

glm::vec4 rgba(float r, float g, float b, float a)
{
	return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

}

void WavePhysicsModel::buildScene()
{
	if (mediumMode == MediumMode::SOLID || mediumMode == MediumMode::SUPERSOLID) {
		buildSolid();
	} else {
		buildFluid();
	}
	buildOverlays();
}

void WavePhysicsModel::buildSolid()
{
	solid = std::make_unique<SolidLattice>();
	SolidLattice& s = *solid;

	s.gap = 18.0f;
	const float startX = SOURCE_X;
	const float endX = CANVAS_W - 25.0f;
	s.Nx = static_cast<int>(std::floor((endX - startX) / s.gap)) + 1;
	s.Ny = static_cast<int>(std::floor(CANVAS_H / s.gap));
	// The original wraps at the canvas height (500) although the rows repeat
	// every Ny * gap (486), which leaves the top-to-bottom springs stretched by
	// 14 px at rest. Wrapping at the rows' own period removes that.
	s.periodY = s.Ny * s.gap;
	s.spongeStart = CANVAS_W * 0.82f;

	auto idx = [&s](int i, int j) { return i * s.Ny + j; };

	s.nodes.reserve(s.Nx * s.Ny);
	for (int i = 0; i < s.Nx; ++i) {
		for (int j = 0; j < s.Ny; ++j) {
			const glm::vec2 pos(startX + i * s.gap, (j + 0.5f) * s.gap);
			s.nodes.push_back({pos, glm::vec2(0.0f), pos, i == 0});
		}
	}

	const float diag = s.gap * glm::root_two<float>();
	for (int i = 0; i < s.Nx; ++i) {
		for (int j = 0; j < s.Ny; ++j) {
			const int a = idx(i, j);
			if (i + 1 < s.Nx) {
				s.springs.push_back({a, idx(i + 1, j), s.gap});                          // right (not periodic in x)
			}
			s.springs.push_back({a, idx(i, (j + 1) % s.Ny), s.gap});                     // vertical (periodic in y)
			if (i + 1 < s.Nx) {
				s.springs.push_back({a, idx(i + 1, (j + 1) % s.Ny), diag});              // diagonals, to reduce
				s.springs.push_back({a, idx(i + 1, (j - 1 + s.Ny) % s.Ny), diag});       // "fabric" anisotropy
			}
		}
	}

	// Faint links first so the nodes draw over them
	for (int k = 0; k < static_cast<int>(s.springs.size()); ++k) {
		if (s.springs[k].rest <= s.gap * 1.01f) {
			s.linkSprings.push_back(k);
			s.linkIndices.push_back(addSegment());
		}
	}

	s.objectIndices.reserve(s.nodes.size());
	for (const Node& n : s.nodes) {
		const float diameter = (n.driven ? 7.0f : 6.0f) * PX;
		app_.addObject(lightGraphics::ShapeType::SPHERE, toWorld(n.pos), glm::vec3(diameter),
					   divergingColor(0.0f, 0.75f), glm::quat(1, 0, 0, 0), "Node", 0.1f);
		s.objectIndices.push_back(static_cast<int>(app_.getObjectCount()) - 1);
	}
}

void WavePhysicsModel::buildFluid()
{
	fluid = std::make_unique<FluidSystem>();
	FluidSystem& f = *fluid;

	int count = 2200;
	if (mediumMode == MediumMode::VISCOUS) count = 2400;
	if (mediumMode == MediumMode::GAS) count = 1400;

	const float minX = SOURCE_X + 25.0f;
	f.particles.reserve(count);
	for (int i = 0; i < count; ++i) {
		const glm::vec2 pos(minX + random01(rng) * (CANVAS_W - minX), random01(rng) * CANVAS_H);
		const glm::vec2 vel((random01(rng) - 0.5f) * 0.3f, (random01(rng) - 0.5f) * 0.3f);
		f.particles.push_back({pos, vel});
	}
	// Short-range repulsion keeps tracers from clumping
	f.minDist = (mediumMode == MediumMode::GAS) ? 6.0f : 7.0f;
	f.repelK = (mediumMode == MediumMode::GAS) ? 0.55f : 0.70f;

	const float diameter = 2.0f * ((mediumMode == MediumMode::GAS) ? 3.2f : 2.8f) * PX;
	f.objectIndices.reserve(count);
	for (const Particle& part : f.particles) {
		app_.addObject(lightGraphics::ShapeType::SPHERE, toWorld(part.pos), glm::vec3(diameter),
					   divergingColor(0.0f, 0.7f), glm::quat(1, 0, 0, 0), "Tracer", 0.04f);
		f.objectIndices.push_back(static_cast<int>(app_.getObjectCount()) - 1);
	}
}

void WavePhysicsModel::buildOverlays()
{
	sourceLineIndex = addSegment();

	probeSegments.clear();
	const int bins = solid ? 140 : 160;
	for (int i = 0; i + 1 < bins; ++i) {
		probeSegments.push_back(addSegment());
	}

	envelopeSegments.clear();
	if (fluid) {
		for (float x = SOURCE_X + 4.0f; x < CANVAS_W; x += 4.0f) {
			envelopeSegments.push_back(addSegment());
		}
	}
}

// ---- Stepping ----------------------------------------------------------------

void WavePhysicsModel::update(float deltaTime)
{
	if (pendingMediumMode) {
		const MediumMode mode = *pendingMediumMode;
		pendingMediumMode.reset();
		setMediumMode(mode);
		printStatus();
	}

	// The original advances one animation frame per display frame. Run whole
	// frames at 60 per second, capped so a slow machine runs slower rather
	// than taking huge, unstable steps.
	frameAccumulator += deltaTime * FRAMES_PER_SECOND;
	int frames = static_cast<int>(frameAccumulator);
	frameAccumulator -= frames;
	if (frames > MAX_FRAMES_PER_UPDATE) {
		frames = MAX_FRAMES_PER_UPDATE;
		frameAccumulator = 0.0f;
	}
	for (int f = 0; f < frames; ++f) {
		stepFrame();
	}

	draw();
}

void WavePhysicsModel::stepFrame()
{
	const float dt = 1.0f / SUB_STEPS;
	for (int s = 0; s < SUB_STEPS; ++s) {
		stepPhysics(dt);
	}
}

void WavePhysicsModel::stepPhysics(float dt)
{
	time += dt;

	const float omega = frequency;
	const float disp = AMP * std::sin(time * omega);
	const float vel = AMP * omega * std::cos(time * omega);

	if (solid) {
		stepSolid(dt, disp, vel);
	}
	if (fluid) {
		stepFluid(dt, omega);
	}
}

void WavePhysicsModel::stepSolid(float dt, float disp, float vel)
{
	SolidLattice& s = *solid;
	const bool super = (mediumMode == MediumMode::SUPERSOLID);

	// Spring constants tuned for a stable, clean plane wave
	const float kSpring = super ? 0.09f : 0.085f;
	const float kDamp = super ? 0.0f : 0.030f;

	// 1) Drive the left column as a plane source
	for (Node& n : s.nodes) {
		if (!n.driven) continue;
		if (waveType == WaveType::TRANSVERSE) {
			n.pos = glm::vec2(n.base.x, wrapPosition(n.base.y + disp, s.periodY));
			n.vel = glm::vec2(0.0f, vel);
		} else {
			n.pos = glm::vec2(n.base.x + disp, n.base.y);
			n.vel = glm::vec2(vel, 0.0f);
		}
	}

	// 2) Spring forces, with periodic y
	for (const Spring& spring : s.springs) {
		Node& a = s.nodes[spring.a];
		Node& b = s.nodes[spring.b];

		const glm::vec2 d(b.pos.x - a.pos.x, wrapDelta(b.pos.y - a.pos.y, s.periodY));
		const float distSq = glm::dot(d, d);
		if (distSq < 1e-6f) continue;

		const float dist = std::sqrt(distSq);
		const glm::vec2 n = d / dist;
		const float vrel = glm::dot(b.vel - a.vel, n);
		const glm::vec2 force = (kSpring * (dist - spring.rest) + kDamp * vrel) * n;

		// Driven nodes are a prescribed source
		if (!a.driven) a.vel += force * dt;
		if (!b.driven) b.vel -= force * dt;
	}

	// 3) Integrate, damp, and absorb in the sponge layer so the wave doesn't
	//    reflect off the right edge
	const float linDamp = super ? 0.9997f : 0.9965f;
	for (Node& n : s.nodes) {
		if (n.driven) continue;

		n.vel *= linDamp;
		if (n.pos.x > s.spongeStart) {
			const float t = glm::clamp((n.pos.x - s.spongeStart) / (CANVAS_W - s.spongeStart), 0.0f, 1.0f);
			n.vel *= super ? (1.0f - 0.06f * t * t) : (1.0f - 0.18f * t * t);
		}

		n.pos += n.vel * dt;
		n.pos.y = wrapPosition(n.pos.y, s.periodY);
	}
}

WavePhysicsModel::FluidParams WavePhysicsModel::getFluidParams() const
{
	// Chantal's demo parameters (lengths in pixels):
	// - deltaShear: how far transverse motion penetrates
	// - coupling: how strongly tracers follow the imposed field
	// - jitter: random motion, making the medium look more gas-like
	// - drag: damps random motion so viscous looks sluggish
	FluidParams p{25.0f, 0.6f, 0.12f, 0.02f, 7.0f, 600.0f};

	if (mediumMode == MediumMode::VISCOUS) {
		p.deltaShear = glm::mix(35.0f, 140.0f, viscosity);
		p.coupling = glm::mix(0.70f, 0.92f, viscosity);
		p.jitter = glm::mix(0.06f, 0.02f, viscosity);
		p.drag = glm::mix(0.03f, 0.08f, viscosity);
		p.soundSpeed = 6.5f;
		p.soundAtten = glm::mix(520.0f, 240.0f, viscosity);
	} else if (mediumMode == MediumMode::LIQUID) {
		p = {18.0f, 0.55f, 0.10f, 0.02f, 7.5f, 700.0f};  // Almost no transverse penetration; sound propagates
	} else if (mediumMode == MediumMode::GAS) {
		p = {10.0f, 0.35f, 0.55f, 0.01f, 5.5f, 520.0f};  // Transverse dies immediately; lots of thermal motion
	}

	return p;
}

void WavePhysicsModel::stepFluid(float dt, float omega)
{
	FluidSystem& f = *fluid;
	const FluidParams params = getFluidParams();
	const float disp = AMP * std::sin(time * omega);

	// The driving plane moves in x for longitudinal waves
	const float driveX = (waveType == WaveType::LONGITUDINAL) ? (SOURCE_X + disp) : SOURCE_X;

	// 1) Impose an analytic velocity field, as the original does:
	//    transverse: oscillatory shear layer  vy ~ exp(-x/delta) cos(wt - x/delta)
	//    longitudinal: travelling sound wave  vx ~ exp(-x/L) cos(wt - kx)
	const float kShear = 1.0f / std::max(8.0f, params.deltaShear);
	const float kSound = omega / std::max(2.0f, params.soundSpeed);
	const float damp = std::exp(-params.drag * dt);

	for (Particle& p : f.particles) {
		const float xDist = std::max(0.0f, p.pos.x - driveX);
		glm::vec2 field(0.0f);
		if (waveType == WaveType::TRANSVERSE) {
			const float env = std::exp(-kShear * xDist);
			field.y = (AMP * omega) * std::cos(omega * time - kShear * xDist) * env;
		} else {
			const float env = std::exp(-xDist / std::max(80.0f, params.soundAtten));
			field.x = (AMP * omega) * std::cos(omega * time - kSound * xDist) * env;
		}

		p.vel += (field - p.vel) * params.coupling * dt;               // Relax towards the field
		const float j = params.jitter * dt;                             // Jitter: gas noisy, viscous quiet
		p.vel += glm::vec2(random01(rng) - 0.5f, random01(rng) - 0.5f) * j;
		p.vel *= damp;                                                  // Drag
	}

	// 2) Short-range repulsion. Like the original, every pair is visited from
	//    both sides, so each pushes twice. Unlike it, neighbour rows wrap in y,
	//    matching the periodic distance.
	constexpr float CELL = 14.0f;
	const int cols = static_cast<int>(std::ceil(CANVAS_W / CELL));
	const int rows = static_cast<int>(std::ceil(CANVAS_H / CELL));
	f.grid.resize(static_cast<size_t>(cols) * rows);
	for (auto& cell : f.grid) {
		cell.clear();
	}
	auto cellOf = [&](const glm::vec2& pos) {
		return glm::ivec2(glm::clamp(static_cast<int>(std::floor(pos.x / CELL)), 0, cols - 1),
						  glm::clamp(static_cast<int>(std::floor(pos.y / CELL)), 0, rows - 1));
	};
	for (int k = 0; k < static_cast<int>(f.particles.size()); ++k) {
		const glm::ivec2 c = cellOf(f.particles[k].pos);
		f.grid[c.y * cols + c.x].push_back(k);
	}

	const float minDSq = f.minDist * f.minDist;
	for (int k = 0; k < static_cast<int>(f.particles.size()); ++k) {
		Particle& p = f.particles[k];
		const glm::ivec2 c = cellOf(p.pos);
		for (int dy = -1; dy <= 1; ++dy) {
			const int ny = (c.y + dy + rows) % rows;
			for (int dx = -1; dx <= 1; ++dx) {
				const int nx = c.x + dx;
				if (nx < 0 || nx >= cols) continue;
				for (int q : f.grid[ny * cols + nx]) {
					if (q == k) continue;
					Particle& other = f.particles[q];
					const glm::vec2 d(other.pos.x - p.pos.x, wrapDelta(other.pos.y - p.pos.y, CANVAS_H));
					const float d2 = glm::dot(d, d);
					if (d2 > 0.0001f && d2 < minDSq) {
						const float dist = std::sqrt(d2);
						const glm::vec2 push = (d / dist) * (1.0f - dist / f.minDist) * f.repelK * dt;
						p.vel -= push;
						other.vel += push;
					}
				}
			}
		}
	}

	// 3) Integrate. Periodic in y; recycled in x so nothing reflects off the edges.
	const float right = CANVAS_W + 20.0f;
	const float left = SOURCE_X - 40.0f;
	for (Particle& p : f.particles) {
		p.pos += p.vel * dt;
		p.pos.y = wrapPosition(p.pos.y, CANVAS_H);

		if (p.pos.x > right) {
			p.pos = glm::vec2(SOURCE_X + 25.0f + random01(rng) * 20.0f, random01(rng) * CANVAS_H);
			p.vel *= 0.2f;
		}
		if (p.pos.x < left) {
			p.pos = glm::vec2(CANVAS_W - 25.0f - random01(rng) * 20.0f, random01(rng) * CANVAS_H);
			p.vel *= 0.2f;
		}
	}
}

// ---- Drawing -------------------------------------------------------------------

float WavePhysicsModel::viewHeight() const
{
	return solid ? solid->periodY : CANVAS_H;
}

glm::vec3 WavePhysicsModel::toWorld(glm::vec2 canvasPos, float z) const
{
	// Centre the canvas on the origin and flip y (canvas y points down)
	return glm::vec3((canvasPos.x - CANVAS_W * 0.5f) * PX, (viewHeight() * 0.5f - canvasPos.y) * PX, z);
}

int WavePhysicsModel::addSegment()
{
	app_.addObject(lightGraphics::ShapeType::CUBE, glm::vec3(0.0f), glm::vec3(0.0f),
				   glm::vec4(0.0f), glm::quat(1, 0, 0, 0), "Line", 0.0f);
	return static_cast<int>(app_.getObjectCount()) - 1;
}

void WavePhysicsModel::placeSegment(int index, glm::vec2 a, glm::vec2 b, float widthPx, const glm::vec4& color, float z)
{
	// A thin box from a to b stands in for a canvas line
	const glm::vec3 wa = toWorld(a, z);
	const glm::vec3 wb = toWorld(b, z);
	const glm::vec3 d = wb - wa;
	const float length = glm::length(d);
	const float width = widthPx * PX;
	const glm::quat rotation = glm::angleAxis(std::atan2(d.y, d.x), glm::vec3(0.0f, 0.0f, 1.0f));
	app_.updateObjectProperties(index, (wa + wb) * 0.5f, glm::vec3(length + width * 0.5f, width, width), rotation);
	app_.setObjectColor(index, color);
}

void WavePhysicsModel::drawProbeLine(const std::vector<glm::vec2>& points, const glm::vec4& color, float widthPx)
{
	for (size_t i = 0; i < probeSegments.size(); ++i) {
		if (i + 1 < points.size()) {
			placeSegment(probeSegments[i], points[i], points[i + 1], widthPx, color, 0.02f);
		} else {
			app_.setObjectColor(probeSegments[i], glm::vec4(0.0f));  // Unused this frame
		}
	}
}

void WavePhysicsModel::draw()
{
	const bool super = (mediumMode == MediumMode::SUPERSOLID);
	const float h = viewHeight();

	// Driving plane
	const float disp = AMP * std::sin(time * frequency);
	const float sx = (waveType == WaveType::LONGITUDINAL) ? SOURCE_X + disp : SOURCE_X;
	placeSegment(sourceLineIndex, glm::vec2(sx, 0.0f), glm::vec2(sx, h), 2.0f,
				 super ? rgba(168, 85, 247, 0.81f) : rgba(96, 165, 250, 0.81f), 0.01f);

	if (solid) drawSolid();
	if (fluid) drawFluid();
}

void WavePhysicsModel::drawSolid()
{
	const SolidLattice& s = *solid;
	const bool super = (mediumMode == MediumMode::SUPERSOLID);
	const bool transverse = (waveType == WaveType::TRANSVERSE);

	// Faint links (cardinal springs only), drawn along the shortest wrapped path
	const glm::vec4 linkColor = super ? rgba(168, 85, 247, 0.056f) : rgba(255, 255, 255, 0.048f);
	for (size_t k = 0; k < s.linkSprings.size(); ++k) {
		const Spring& spring = s.springs[s.linkSprings[k]];
		const glm::vec2 a = s.nodes[spring.a].pos;
		const glm::vec2 b = s.nodes[spring.b].pos;
		placeSegment(s.linkIndices[k], a, glm::vec2(b.x, a.y + wrapDelta(b.y - a.y, s.periodY)), 1.0f, linkColor, -0.02f);
	}

	// Nodes coloured by displacement
	for (size_t k = 0; k < s.nodes.size(); ++k) {
		const Node& n = s.nodes[k];
		const float d = transverse ? wrapDelta(n.pos.y - n.base.y, s.periodY) : (n.pos.x - n.base.x);
		app_.setObjectPosition(s.objectIndices[k], toWorld(n.pos));
		app_.setObjectColor(s.objectIndices[k], divergingColor(d / (AMP * 1.2f), n.driven ? 0.9f : 0.75f));
	}

	// Probe line: average displacement at each x, which makes the wave easy to read
	constexpr int bins = 140;
	std::vector<float> sum(bins, 0.0f);
	std::vector<int> count(bins, 0);
	for (const Node& n : s.nodes) {
		if (n.driven) continue;
		const int bx = glm::clamp(static_cast<int>(std::floor(n.pos.x / CANVAS_W * bins)), 0, bins - 1);
		sum[bx] += transverse ? wrapDelta(n.pos.y - n.base.y, s.periodY) : (n.pos.x - n.base.x);
		count[bx] += 1;
	}
	// The original also plots empty bins as zero. Bins are 6.4 px wide but the
	// lattice columns are 18 px apart, so that zigzags; join only filled bins.
	std::vector<glm::vec2> points;
	for (int i = 0; i < bins; ++i) {
		if (count[i] == 0) continue;
		points.emplace_back((i + 0.5f) / bins * CANVAS_W, s.periodY * 0.5f + sum[i] / count[i] * 0.55f);
	}
	drawProbeLine(points, super ? rgba(216, 180, 254, 0.56f) : rgba(96, 165, 250, 0.56f), 2.0f);
}

void WavePhysicsModel::drawFluid()
{
	const FluidSystem& f = *fluid;
	const bool transverse = (waveType == WaveType::TRANSVERSE);
	const bool gas = (mediumMode == MediumMode::GAS);

	// Tracers coloured by the velocity component along the wave's motion,
	// normalised by the source's velocity scale
	const float vScale = std::max(1e-3f, AMP * frequency);
	for (size_t k = 0; k < f.particles.size(); ++k) {
		const Particle& p = f.particles[k];
		const float comp = transverse ? p.vel.y : p.vel.x;
		app_.setObjectPosition(f.objectIndices[k], toWorld(p.pos));
		app_.setObjectColor(f.objectIndices[k], divergingColor(comp / (vScale * 1.4f), gas ? 0.55f : 0.70f));
	}

	// Probe line: average velocity at each x, which shows the attenuation clearly
	constexpr int bins = 160;
	std::vector<float> sum(bins, 0.0f);
	std::vector<int> count(bins, 0);
	for (const Particle& p : f.particles) {
		const int bx = glm::clamp(static_cast<int>(std::floor(p.pos.x / CANVAS_W * bins)), 0, bins - 1);
		sum[bx] += transverse ? p.vel.y : p.vel.x;
		count[bx] += 1;
	}
	std::vector<glm::vec2> points;
	for (int i = 0; i < bins; ++i) {
		if (count[i] == 0) continue;  // Join only filled bins, as for the solid
		points.emplace_back((i + 0.5f) / bins * CANVAS_W, CANVAS_H * 0.5f + sum[i] / count[i] * (55.0f / vScale));
	}
	drawProbeLine(points, rgba(226, 232, 240, 0.64f), 2.0f);

	// Shear penetration envelope near the bottom, for transverse waves only
	const float k = 1.0f / std::max(8.0f, getFluidParams().deltaShear);
	glm::vec2 prev(SOURCE_X, CANVAS_H * 0.85f - 55.0f);
	for (size_t i = 0; i < envelopeSegments.size(); ++i) {
		const float x = SOURCE_X + 4.0f * (i + 1);
		const glm::vec2 next(x, CANVAS_H * 0.85f - std::exp(-k * (x - SOURCE_X)) * 55.0f);
		placeSegment(envelopeSegments[i], prev, next, 1.5f,
					 transverse ? rgba(255, 255, 255, 0.13f) : glm::vec4(0.0f), 0.02f);
		prev = next;
	}
}
