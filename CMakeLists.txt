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

find_package(LightVulkanGraphics CONFIG REQUIRED)
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
