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

#include <cstdint>

namespace GPUCapabilities {
	enum class GPUTier {
		MINIMAL,    // < 1GB VRAM, integrated GPU
		LOW,        // 1-2GB VRAM
		MEDIUM,     // 2-4GB VRAM
		HIGH,       // 4-8GB VRAM
		ULTRA       // 8GB+ VRAM, high-end discrete GPU
	};

	struct GPUInfo {
		GPUTier tier;
		uint64_t vramMB;
		bool isDiscrete;
		int latticeResolution;  // Nx value for lattice (Nx^3 particles)
	};

	GPUInfo detectGPU();
	int calculateLatticeResolution(GPUTier tier, uint64_t vramMB, bool isDiscrete);
}
