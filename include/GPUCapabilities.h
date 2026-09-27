// SPDX-License-Identifier: LGPL-3.0-or-later

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
