# CMakeList.txt : CMake project for Astrocelerate, include source and define
# project specific logic here.

# Specify application
cmake_minimum_required(VERSION 3.20)
project(Astrocelerate)

    # Set application info
    set(APP_NAME "Astrocelerate")
    add_compile_definitions(APP_NAME="${APP_NAME}")

    set(APP_VERSION "v0.1-alpha")
    add_compile_definitions(APP_VERSION="${APP_VERSION}")

    set(AUTHOR "Duong Duy Nhat Minh")
    add_compile_definitions(AUTHOR="${AUTHOR}")

    # Specify source/binary file directories
    add_compile_definitions(APP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_compile_definitions(APP_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

    # Specify VMA version
    set(VMA_VERSION 1002000) # 1.2.0  (Form: ABBBCCC -> A = major, BBB = minor, CCC = patch); The VMA version must match that of the Vulkan version, i.e., VULKAN_VERSION
    add_compile_definitions(VMA_VULKAN_VERSION=VMA_VERSION)


# Enable Hot Reload for MSVC compilers (Edit & Continue).
    # Note: Since CMake versions 3.20+ always support POLICY CMP0141, there's no need to check if that policy exists.
cmake_policy(SET CMP0141 NEW)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")


# Set up vcpkg toolchain file
    # NOTE: This setup has been added as a command-line argument instead. See CMakeSettings.json for more information.
#set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")


# Set up MSVC configuration properties
if (MSVC)
    add_compile_options(
        /MP  # Multithreaded compilation
        #/WX  # Warnings as errors
    )
endif()


# Enable Address Sanitizer
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
        # Handle /RTC and /ZI as mentioned above if needed.
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
    endif()
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


# Functions
    # Handles connectivity detection
    function(hasInternetConnection isOnline)
        set(TEST_URL "https://www.google.com/robots.txt")
        set(TEMP_FILE "${CMAKE_BINARY_DIR}/internet_test.txt")

        file(DOWNLOAD
            ${TEST_URL}
            ${TEMP_FILE}
            TIMEOUT 5
            STATUS downloadStatus
        )

        list(GET downloadStatus 0 statusCode)

        if(statusCode EQUAL 0)
            set(${isOnline} TRUE PARENT_SCOPE)
        else()
            set(${isOnline} FALSE PARENT_SCOPE)
        endif()

        file(REMOVE ${TEMP_FILE})
    endfunction()


    # Handles dependency fetching
    function(FetchDependency depName gitRepo gitTag offlineDir)
        if (IS_ONLINE)
            message(STATUS "Fetching ${depName} from online repository: ${gitRepo}")
            FetchContent_Declare(
                ${depName}
                GIT_REPOSITORY ${gitRepo}
                GIT_TAG ${gitTag}
            )

            FetchContent_MakeAvailable(${depName})
        else()
            message(STATUS "Using offline source for ${depName}: ${offlineDir}")
            FetchContent_Declare(
                ${depName}
                SOURCE_DIR ${offlineDir}
            )
        endif()
    endfunction()


# Find packages
find_package(Vulkan REQUIRED)    # Vulkan SDK
    # Locate glslc.exe
        find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin)
        if (NOT Vulkan_GLSLC_EXECUTABLE)
            message(FATAL_ERROR "glslc.exe not found! Ensure Vulkan SDK is installed and added to PATH.")
        endif()
        message(STATUS "Vulkan glslc.exe executable is in directory \"${Vulkan_GLSLC_EXECUTABLE}\"")

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)


# Install libraries
    # Define offline mode
    hasInternetConnection(IS_ONLINE)

    if (IS_ONLINE)
        message(STATUS "Machine is connected to the internet. CMake configuration has been set to online mode. Fetching dependencies...")
    else()
        message(STATUS "No internet connection detected! CMake configuration has been set to offline mode. Loading dependencies from offline cache...")
    endif()

    #    # Online mode: Fetch from remote source
    #set(GLFW_GIT_REPO "https://github.com/glfw/glfw.git")
    #set(GLM_GIT_REPO "https://github.com/g-truc/glm.git")
    #set(VMA_GIT_REPO "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git")
    set(ASSIMP_GIT_REPO "https://github.com/assimp/assimp.git")
    #
    #    # Offline mode: Fetch from local source
    #set(GLFW_OFFLINE_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/glfw-src")
    #set(GLM_OFFLINE_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/glm-src")
    #set(VMA_OFFLINE_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/vulkanmemoryallocator-src")
    set(ASSIMP_OFFLINE_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/assimp-src")
    #set(ASSIMP_OFFLINE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/assimp-build")

    # Assimp
    fetchDependency(
        assimp
        ${ASSIMP_GIT_REPO}
        v5.4.3
        ${ASSIMP_OFFLINE_SOURCE_DIR}
    )


# Organize libraries into variables
    # For information on the library names, search for them in "vcpkg/ports/[LIBRARY]/usage" either locally or from vcpkg's repository
set(PKG_LIBS
    Vulkan::Vulkan
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    Boxer
)

set(FETCHCONTENT_LIBS
    assimp
)

set(FETCHCONTENT_LIBS_TARGET_INCLUDE_DIRS
    ${glm_SOURCE_DIR} 
    ${GLFW_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${VulkanMemoryAllocator_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
)


# Make libraries available
#FetchContent_MakeAvailable(${FETCHCONTENT_LIBS})

    # Cache libraries for fallback in case of no internet connection (disabled for now)
    if (IS_ONLINE AND 0)
        message(STATUS "Creating dependency cache for offline fallback...")
        file(
            COPY "${CMAKE_CURRENT_BINARY_DIR}/_deps"
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/_offline_deps_cache"
        )
    endif()


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


# Include library directories
target_include_directories(Astrocelerate PRIVATE 
    ${HEADER_DIRS}
    ${FETCHCONTENT_LIBS_TARGET_INCLUDE_DIRS}
)
