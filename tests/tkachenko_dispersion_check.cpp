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

// Deterministic checks of the Tkachenko-like dispersion helper used by the
// wave lab's vortex-lattice superfluid. Prints a table and returns non-zero on
// failure. Parameters are the wave lab's canvas units (pixels, frames).

#include "TkachenkoDispersion.h"

#include <cmath>
#include <cstdio>
#include <limits>

namespace {

int failures = 0;
constexpr double PI = 3.14159265358979323846;

void check(bool condition, const char* what)
{
	if (!condition) {
		std::printf("FAIL: %s\n", what);
		++failures;
	}
}

// omega^2 from k, the forward form of the dispersion relation
double omegaSquared(double k, double cT, double cS, double rotation)
{
	return cT * cT * cS * cS * k * k * k * k / (4.0 * rotation * rotation + cS * cS * k * k);
}

}

int main()
{
	using aether::tkachenkoWaveNumber;
	const float cT = 3.0f, cS = 7.5f, rotation = 0.08f;  // The wave lab's values

	std::printf("%8s %10s %12s %12s\n", "omega", "k (1/px)", "wavelength", "phase speed");
	float previousK = 0.0f;
	for (float omega : {0.05f, 0.12f, 0.20f, 0.30f}) {
		const float k = tkachenkoWaveNumber(omega, cT, cS, rotation);
		std::printf("%8.2f %10.5f %10.1f px %9.3f px/frame\n", omega, k, 2.0 * PI / k, omega / k);

		check(std::isfinite(k) && k > 0.0f, "k is finite and positive over the UI frequency range");
		check(k > previousK, "k increases with omega");
		// The root satisfies the dispersion relation it came from
		const double rel = std::fabs(omegaSquared(k, cT, cS, rotation) - double(omega) * omega) / (double(omega) * omega);
		check(rel < 1e-4, "k satisfies omega^2 = cT^2 cS^2 k^4 / (4 Omega^2 + cS^2 k^2)");
		// Phase speed stays below cT: the mode is softened by rotation and compressibility
		check(omega / k < cT, "phase speed is below cT");
		previousK = k;
	}

	// Limits and parameter trends at omega = 0.12
	const float omega = 0.12f;
	const float kBase = tkachenkoWaveNumber(omega, cT, cS, rotation);
	check(std::fabs(tkachenkoWaveNumber(omega, cT, cS, 0.0f) - omega / cT) < 1e-6f, "no rotation: k = omega / cT");
	check(std::fabs(tkachenkoWaveNumber(omega, cT, std::numeric_limits<float>::infinity(), rotation) - omega / cT) < 1e-6f,
		  "incompressible limit: k = omega / cT");
	check(tkachenkoWaveNumber(omega, 2.0f * cT, cS, rotation) < kBase, "stiffer vortex lattice (larger cT): longer wavelength");
	check(tkachenkoWaveNumber(omega, cT, cS, 2.0f * rotation) > kBase, "faster rotation: softer mode, shorter wavelength");
	check(tkachenkoWaveNumber(omega, cT, 2.0f * cS, rotation) < kBase, "stiffer sound (larger cS): longer wavelength");

	// Invalid input returns 0 (no travelling mode) rather than NaN or infinity
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	for (float k : {tkachenkoWaveNumber(0.0f, cT, cS, rotation), tkachenkoWaveNumber(-0.1f, cT, cS, rotation),
					tkachenkoWaveNumber(nan, cT, cS, rotation), tkachenkoWaveNumber(omega, 0.0f, cS, rotation),
					tkachenkoWaveNumber(omega, inf, cS, rotation), tkachenkoWaveNumber(omega, cT, 0.0f, rotation),
					tkachenkoWaveNumber(omega, cT, nan, rotation), tkachenkoWaveNumber(omega, cT, cS, -0.1f),
					tkachenkoWaveNumber(omega, cT, cS, inf)}) {
		check(k == 0.0f, "invalid input returns 0");
	}
	// Extreme but valid input stays finite and clamped
	const float kHuge = tkachenkoWaveNumber(1e6f, 1e-6f, cS, rotation);
	check(std::isfinite(kHuge) && kHuge <= aether::kMaxTkachenkoWaveNumber, "extreme input is clamped");

	std::printf(failures == 0 ? "All checks passed\n" : "%d check(s) failed\n", failures);
	return failures == 0 ? 0 : 1;
}
