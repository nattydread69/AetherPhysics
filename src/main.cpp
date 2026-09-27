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

#include "lightVulkanGraphics.h"
#include "PhysicsApp.h"

#include <iostream>
#include <stdexcept>


int main(int argc, char* argv[])
{
	try
	{
		PhysicsApp app(argc, argv);
	}
	catch (const std::exception& e)
	{
		std::fprintf(stderr, "Fatal: %s\n", e.what());
		return 2;
	}

    return 0;
}
