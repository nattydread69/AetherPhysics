# SPDX-License-Identifier: LGPL-3.0-or-later
#
# AetherPhysics
# Copyright (C) 2025 Dr. Nathanael John Inkson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.16)

project(AetherPhysics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE AETHER_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
set(AETHER_SOURCES ${AETHER_SOURCES})
file(GLOB_RECURSE AETHER_HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

# 2.1.1 adds VolumeColorSource, which the density fog needs
find_package(LightVulkanGraphics 2.1.1 CONFIG REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME}
	${AETHER_SOURCES}
	${AETHER_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE
	LightVulkanGraphics::LightVulkanGraphics
	glfw
)

# Standalone numerical check of the vortex-lattice dispersion helper. It needs
# no graphics; run with ctest.
option(AETHER_BUILD_CHECKS "Build numerical checks (run with ctest)" ON)
if (AETHER_BUILD_CHECKS)
	enable_testing()
	add_executable(tkachenko_dispersion_check tests/tkachenko_dispersion_check.cpp)
	target_include_directories(tkachenko_dispersion_check PRIVATE include)
	add_test(NAME tkachenko_dispersion_check COMMAND tkachenko_dispersion_check)
endif()
