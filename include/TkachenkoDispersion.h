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

#include <algorithm>
#include <cmath>
#include <limits>

// Dispersion of a Tkachenko-like transverse mode of a vortex lattice in a
// rotating, compressible superfluid (simplified form):
//
//     omega^2 = cT^2 cS^2 k^4 / (4 Omega^2 + cS^2 k^2)
//
//   omega  driving angular frequency
//   cT     transverse wave speed scale of the vortex lattice (its effective
//          shear elasticity; the bulk superfluid itself has none)
//   cS     sound speed of the superfluid
//   Omega  background rotation rate that sustains the vortex array
//   k      wavenumber of the vortex-lattice mode
//
// Limits: cS -> infinity (incompressible) gives omega = cT k; small k with
// finite cS gives the soft quadratic mode omega ~ cT cS k^2 / (2 Omega).
//
// Units are whatever the caller uses. The wave lab passes its canvas units
// (pixels, animation frames), so these are visualisation parameters, not
// calibrated physical quantities.
namespace aether {

// Maximum wavenumber returned, in the caller's inverse length units. For the
// wave lab (pixels) this is a 2*pi px wavelength, far shorter than anything the
// UI frequency range produces; it only guards against runaway values.
inline constexpr float kMaxTkachenkoWaveNumber = 1.0f;

// Positive root of the dispersion relation for k given omega. With y = k^2:
//
//     cT^2 cS^2 y^2 - omega^2 cS^2 y - 4 Omega^2 omega^2 = 0
//
// Dividing by cS^2 and taking the positive root:
//
//     y = (omega^2 + sqrt(omega^4 + 16 cT^2 Omega^2 omega^2 / cS^2)) / (2 cT^2)
//
// Both terms in the numerator are non-negative, so there is no cancellation,
// and cS = infinity is handled (the correction term vanishes).
//
// Returns 0 (a spatially uniform oscillation, i.e. no travelling mode) for
// invalid input: non-finite or non-positive omega or cT, non-positive or NaN
// cS, or negative or non-finite Omega. Otherwise the result is finite, positive
// and at most kMaxTkachenkoWaveNumber.
inline float tkachenkoWaveNumber(float omega, float cT, float cS, float rotationRate)
{
	const bool valid = std::isfinite(omega) && omega > 0.0f &&
		std::isfinite(cT) && cT > 0.0f &&
		!std::isnan(cS) && cS > 0.0f &&
		std::isfinite(rotationRate) && rotationRate >= 0.0f;
	if (!valid) {
		return 0.0f;
	}

	// Work in double: omega^4 and the rotation term span many orders of magnitude
	const double w2 = static_cast<double>(omega) * omega;
	const double cT2 = static_cast<double>(cT) * cT;
	const double rotationTerm = std::isinf(cS)
		? 0.0
		: 16.0 * cT2 * rotationRate * rotationRate * w2 / (static_cast<double>(cS) * cS);
	const double discriminant = std::max(0.0, w2 * w2 + rotationTerm);  // Guard roundoff
	const double y = (w2 + std::sqrt(discriminant)) / (2.0 * cT2);
	const double k = std::sqrt(y);

	if (!std::isfinite(k) || k <= 0.0) {
		return 0.0f;
	}
	return static_cast<float>(std::min(k, static_cast<double>(kMaxTkachenkoWaveNumber)));
}

}
