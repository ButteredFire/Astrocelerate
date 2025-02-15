# CMakeList.txt : CMake project for Astrocelerate_Vulkan, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Astrocelerate_Vulkan")

# Add source to this project's executable.
add_executable (Astrocelerate_Vulkan "src/Application.cpp" "src/Application.h" "src/Constants.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Astrocelerate_Vulkan PROPERTY CXX_STANDARD 20)
endif()

# Set application version
set(APP_VERSION "v0.1-alpha-vulkan")
add_compile_definitions(APP_VERSION="${APP_VERSION}")

# TODO: Add tests and install targets if needed.
