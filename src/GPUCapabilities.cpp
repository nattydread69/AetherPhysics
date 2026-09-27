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

#include "GPUCapabilities.h"
#include <iostream>
#include <fstream>
#include <cstring>

#ifdef __linux__
	#include <sys/sysinfo.h>
#elif _WIN32
	#include <windows.h>
#elif __APPLE__
	#include <sys/sysctl.h>
#endif

namespace GPUCapabilities {

static uint64_t getTotalSystemMemoryMB() {
#ifdef __linux__
	struct sysinfo si;
	if (sysinfo(&si) == 0) {
		return (si.totalram * si.mem_unit) / (1024 * 1024);
	}
#elif _WIN32
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex)) {
		return statex.ullTotalPhys / (1024 * 1024);
	}
#elif __APPLE__
	uint64_t mem;
	size_t len = sizeof(mem);
	if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0) {
		return mem / (1024 * 1024);
	}
#endif
	return 0;
}

static uint64_t estimateGPUVRAM(bool isDiscrete) {
	if (!isDiscrete) {
		uint64_t systemMem = getTotalSystemMemoryMB();
		if (systemMem > 0) {
			return systemMem / 2;
		}
		return 2048;
	}

	uint64_t vram = 0;

#ifdef __linux__
	std::ifstream gpuinfo("/proc/driver/nvidia/gpus/0000:01:00.0/information");
	if (gpuinfo.is_open()) {
		std::string line;
		while (std::getline(gpuinfo, line)) {
			if (line.find("Memory") != std::string::npos) {
				size_t pos = line.find_last_of("0123456789");
				if (pos != std::string::npos) {
					std::string numStr = line.substr(0, pos + 1);
					size_t lastSpace = numStr.find_last_of(" \t");
					if (lastSpace != std::string::npos) {
						vram = std::stoull(numStr.substr(lastSpace + 1));
						vram /= 1024;
						break;
					}
				}
			}
		}
		gpuinfo.close();
	}

	if (vram == 0) {
		std::ifstream amdGpuinfo("/sys/class/drm/card0/device/mem_info_vram_total");
		if (amdGpuinfo.is_open()) {
			amdGpuinfo >> vram;
			vram /= (1024 * 1024);
			amdGpuinfo.close();
		}
	}
#endif

	if (vram == 0) {
		vram = 4096;
	}

	return vram;
}

GPUInfo detectGPU() {
	GPUInfo info;
	info.isDiscrete = true;

	info.vramMB = estimateGPUVRAM(info.isDiscrete);

	if (info.vramMB < 1024) {
		info.tier = GPUTier::MINIMAL;
	} else if (info.vramMB < 2048) {
		info.tier = GPUTier::LOW;
	} else if (info.vramMB < 4096) {
		info.tier = GPUTier::MEDIUM;
	} else if (info.vramMB < 8192) {
		info.tier = GPUTier::HIGH;
	} else {
		info.tier = GPUTier::ULTRA;
	}

	info.latticeResolution = calculateLatticeResolution(info.tier, info.vramMB, info.isDiscrete);

	std::cout << "GPU Detection: " << info.vramMB << " MB VRAM";
	switch (info.tier) {
		case GPUTier::MINIMAL:
			std::cout << " (MINIMAL tier)";
			break;
		case GPUTier::LOW:
			std::cout << " (LOW tier)";
			break;
		case GPUTier::MEDIUM:
			std::cout << " (MEDIUM tier)";
			break;
		case GPUTier::HIGH:
			std::cout << " (HIGH tier)";
			break;
		case GPUTier::ULTRA:
			std::cout << " (ULTRA tier)";
			break;
	}
	std::cout << " -> Lattice resolution: " << info.latticeResolution << "^3" << std::endl;

	return info;
}

int calculateLatticeResolution(GPUTier tier, uint64_t vramMB, bool /*isDiscrete*/) {
	int baseResolution;

	switch (tier) {
		case GPUTier::MINIMAL:
			baseResolution = 9;
			break;
		case GPUTier::LOW:
			baseResolution = 13;
			break;
		case GPUTier::MEDIUM:
			baseResolution = 17;
			break;
		case GPUTier::HIGH:
			baseResolution = 25;
			break;
		case GPUTier::ULTRA:
			baseResolution = 33;
			break;
		default:
			baseResolution = 17;
	}

	int adjustedResolution = baseResolution;
	if (vramMB > 8192) {
		if (vramMB > 16384) {
			adjustedResolution = std::min(41, baseResolution + 8);
		} else {
			adjustedResolution = std::min(37, baseResolution + 4);
		}
	}

	return adjustedResolution;
}

}
