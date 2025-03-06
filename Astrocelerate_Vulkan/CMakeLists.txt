# CMakeList.txt : CMake project for Astrocelerate, include source and define
# project specific logic here.

# Specify application
cmake_minimum_required(VERSION 3.20)
project(Astrocelerate)

    # Set application version
    set(APP_VERSION "v0.1-alpha")
    add_compile_definitions(APP_VERSION="${APP_VERSION}")


# Enable Hot Reload for MSVC compilers (Edit & Continue).
    # Note: Since CMake versions 3.20+ always support POLICY CMP0141, there's no need to check if that policy exists.
cmake_policy(SET CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")


# Enable multi-threaded compilation for MSVC
if (MSVC)
    add_compile_options(/MP)
endif()


# Set C++ standard (allow overriding via CMake cache)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Organize files in real directories instead of filters
set_property(GLOBAL PROPERTY USE_FOLDERS ON)



# Add dependencies
include(FetchContent)

    # Add generated file lists
    include("scripts/HeaderDirs.cmake")
    include("scripts/SourceFiles.cmake")
    include("scripts/HeaderFiles.cmake")


# Find packages
find_package(Vulkan REQUIRED)    # Vulkan SDK


# Install libraries
    # GLFW3
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
    )

    # GLM
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
    )


# Organize libraries into variables
set(PKG_LIBS
    Vulkan::Vulkan
)

set(FETCHCONTENT_LIBS
    glfw glm
)


# Make libraries available
FetchContent_MakeAvailable(${FETCHCONTENT_LIBS})


# Add source files to the project's executable
add_executable(Astrocelerate ${SOURCE_FILES} )


# Link libraries
target_link_libraries(Astrocelerate PRIVATE ${PKG_LIBS} ${FETCHCONTENT_LIBS})


# Include directories for header-only libraries
target_include_directories(Astrocelerate PRIVATE 
    ${HEADER_DIRS}
    ${glm_SOURCE_DIR} 
    ${GLFW_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
)
