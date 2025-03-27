# CMakeList.txt : CMake project for Astrocelerate, include source and define
# project specific logic here.

# Specify application
cmake_minimum_required(VERSION 3.20)
project(Astrocelerate)

    # Set application version
    set(APP_VERSION "v0.1-alpha")
    add_compile_definitions(APP_VERSION="${APP_VERSION}")

    # Specify VMA version
    set(VMA_VERSION 1002000) # 1.2.0  (Form: ABBBCCC -> A = major, BBB = minor, CCC = patch); The VMA version must match that of the Vulkan version, i.e., VULKAN_VERSION
    add_compile_definitions(VMA_VULKAN_VERSION=VMA_VERSION)


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


# Add source files to the project's executable
add_executable(Astrocelerate ${SOURCE_FILES} )


# Find packages
find_package(Vulkan REQUIRED)    # Vulkan SDK
    # Locate glslc.exe
        find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin)
        if (NOT Vulkan_GLSLC_EXECUTABLE)
            message(FATAL_ERROR "glslc.exe not found! Ensure Vulkan SDK is installed and added to PATH.")
        endif()
        message(STATUS "Vulkan glslc.exe executable is in directory \"${Vulkan_GLSLC_EXECUTABLE}\"")


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

    # VMA
    FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG master
    )


# Organize libraries into variables
set(PKG_LIBS
    Vulkan::Vulkan Boxer
)

set(FETCHCONTENT_LIBS
    glfw glm VulkanMemoryAllocator
)


# Make libraries available
FetchContent_MakeAvailable(${FETCHCONTENT_LIBS})


# Automate compilation of shader files into SPIR-V bytecode with glslc.exe
    # Set shader directories
    set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")
    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/compiled_shaders") # Turns out, reading the compiled files from VkPipelineManager with openFile("compiled_shaders/ABCShader.spv") actually attempts to open the file at the starting path CURRENT_BINARY_DIR, not CURRENT_SOURCE_DIR.

    # Find all shader files in src/Shaders
    file(GLOB_RECURSE SHADER_FILES "${SHADER_SOURCE_DIR}/*.glsl.*")

    # Create the output directory for compiled SPIR-V files
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

    # List to hold the generated SPIR-V files
    set(SPIRV_BINARY_FILES)

    # Compile each shader
    foreach(SHADER ${SHADER_FILES})
        # NOTE: `NAME` gets the file name and extension, while `NAME_WE` (presumably "name without extension") gets just the name.
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        get_filename_component(SHADER_NAME_WE ${SHADER} NAME_WE)

        set(SPIRV_FILE "${SHADER_NAME_WE}.spv")

        # Add a custom command to compile the shader to SPIR-V bytecode
        add_custom_command(
            OUTPUT "${SPIRV_FILE}"
            COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${SHADER_SOURCE_DIR}/${SHADER_NAME}" -o "${SHADER_OUTPUT_DIR}/${SPIRV_FILE}"
            DEPENDS ${SHADER}  # Ensure that the shader file is up to date
            COMMENT "Compiling \"${SHADER_NAME}\" to SPIR-V bytecode..."
            VERBATIM
        )
        message(STATUS "Shader \"${SHADER_NAME}\" processed; generated SPIR-V bytecode file \"${SPIRV_FILE}\"")

        # Append the generated SPIR-V file to the list of outputs
        list(APPEND SPIRV_BINARY_FILES ${SPIRV_FILE})
    endforeach()

    # Create a custom build target for shaders
        # NOTE: The `DEPENDS` argument ensures that CMake knows the custom commands are dependent on the actual shader files, so it will re-run the commands if the shader files are modified.
    add_custom_target(
        CompileShaders ALL
        DEPENDS ${SPIRV_BINARY_FILES} # Target depends on all SPIR-V files
    )
    add_dependencies(Astrocelerate CompileShaders)


# Add subdirectories
add_subdirectory("external/boxer")


# Link libraries
target_link_libraries(Astrocelerate PRIVATE ${PKG_LIBS} ${FETCHCONTENT_LIBS})


# Include directories for header-only libraries
target_include_directories(Astrocelerate PRIVATE 
    ${HEADER_DIRS}
    ${glm_SOURCE_DIR} 
    ${GLFW_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${VulkanMemoryAllocator_SOURCE_DIR}/include
)
