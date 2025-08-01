# CMakeList.txt : CMake project for Astrocelerate, include source and define
# project specific logic here.

# Specify application
cmake_minimum_required(VERSION 3.30)
project(Astrocelerate)

    # Set application info
    set(APP_NAME "Astrocelerate")
    add_compile_definitions(APP_NAME="${APP_NAME}")

    set(APP_VERSION_MAJOR 0)
    set(APP_VERSION_MINOR 1)
    set(APP_VERSION_PATCH 0)
    set(APP_VERSION_IDENTIFIERS "alpha")
    set(APP_VERSION_FULL "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}-${APP_VERSION_IDENTIFIERS}")
    add_compile_definitions(APP_VERSION="${APP_VERSION_FULL}")

    set(APP_VENDOR "Oriviet Aerospace")
    
    set(AUTHOR "Duong Duy Nhat Minh")
    add_compile_definitions(AUTHOR="${AUTHOR}")

    set(AUTHOR_DIACRITIC "Dương Duy Nhật Minh")
    add_compile_definitions(AUTHOR_DIACRITIC="${AUTHOR_DIACRITIC}")

    # Specify VMA version
    set(VMA_VERSION 1002000) # 1.2.0  (Form: ABBBCCC -> A = major, BBB = minor, CCC = patch); The VMA version must match that of the Vulkan version, i.e., VULKAN_VERSION
    add_compile_definitions(VMA_VULKAN_VERSION=VMA_VERSION)


# Enable Hot Reload for MSVC compilers (Edit & Continue).
    # NOTE: Since CMake versions 3.20+ always support POLICY CMP0141, there's no need to check if that policy exists.
cmake_policy(SET CMP0141 NEW)
#set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<CONFIG:Debug,RelWithDebInfo>,EditAndContinue,ProgramDatabase>")


# Set up vcpkg toolchain file
    # NOTE: This setup has been added as a command-line argument instead. See CMakeSettings.json for more information.
#set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")


# Enable Address Sanitizer if in Debug Mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
        # Handle /RTC and /ZI if needed
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
add_executable(Astrocelerate ${SOURCE_FILES})


# Copy resources from source to binary directory (for development builds)
add_custom_command(TARGET Astrocelerate PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/samples"
            "${CMAKE_CURRENT_BINARY_DIR}/samples"
    COMMENT "Copying samples to build directory for development run"
)

add_custom_command(TARGET Astrocelerate PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMENT "Copying assets to build directory for development run"
)

add_custom_command(TARGET Astrocelerate PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/configs"
            "${CMAKE_CURRENT_BINARY_DIR}/configs"
    COMMENT "Copying configs to build directory for development run"
)

add_custom_command(TARGET Astrocelerate PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/samples"
            "${CMAKE_CURRENT_BINARY_DIR}/samples"
    COMMENT "Copying samples to build directory for development run"
)

add_custom_command(TARGET Astrocelerate PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/logs
    COMMENT "Creating logs folder to store log files"
)


# Set the output directory for the executable in the development build
set_target_properties(Astrocelerate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"  # This mirrors the final installation output
)


# Compile options for C++ source files
    # CMake generator expression: $<condition:value>        (If condition is true, value is used; otherwise, value = empty string)
target_compile_options(Astrocelerate PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
    $<$<CXX_COMPILER_ID:GNU>:-finput-charset=UTF-8>
    $<$<CXX_COMPILER_ID:Clang>:-finput-charset=UTF-8>
)


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


    # Handles shader compilation
    function(CompileShaders)
        # Set shader directories
        set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")
        set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/bin/Shaders")

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

            set(CURRENT_SPIRV_FILE_PATH "${SHADER_OUTPUT_DIR}/${SHADER_NAME_WE}.spv")

            # Add a custom command to compile the shader to SPIR-V bytecode
            add_custom_command(
                OUTPUT "${CURRENT_SPIRV_FILE_PATH}"
                COMMAND "${Vulkan_GLSLC_EXECUTABLE}" "${SHADER_SOURCE_DIR}/${SHADER_NAME}" -o "${CURRENT_SPIRV_FILE_PATH}"
                DEPENDS ${SHADER}  # Ensure that the shader file is up to date
                COMMENT "Compiling \"${SHADER_NAME}\" to SPIR-V bytecode..."
                VERBATIM
            )
            message(STATUS "Shader \"${SHADER_NAME}\" processed; generated SPIR-V bytecode file \"${CURRENT_SPIRV_FILE_PATH}\"")

            # Append the generated SPIR-V file to the list of outputs
            list(APPEND SPIRV_BINARY_FILES ${CURRENT_SPIRV_FILE_PATH})
        endforeach()

        # Create a custom build target for shaders
            # NOTE: The `DEPENDS` argument ensures that CMake knows the custom commands are dependent on the actual shader files, so it will re-run the commands if the shader files are modified.
        add_custom_target(
            CompileShaders ALL
            DEPENDS ${SPIRV_BINARY_FILES} # Target depends on all SPIR-V files
        )
        add_dependencies(Astrocelerate CompileShaders)
    endfunction()


    # Handles library DLL discovery and installation
    function(InstallRuntimeDLLs)
        #set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg.exe")
        #
        #if (NOT EXISTS "${VCPKG_EXECUTABLE}")
        #    message(FATAL_ERROR "vcpkg.exe not found at \"${VCPKG_EXECUTABLE}\". Please ensure VCPKG_ROOT is correctly set and vcpkg is bootstrapped.")
        #endif()
        #
        #set(VCPKG_EXPORT_DIR "${CMAKE_BINARY_DIR}/vcpkg_exported_deps")
        #
        ## Custom target to run vcpkg export
        #add_custom_command(
        #    OUTPUT ${CMAKE_SOURCE_DIR}
        #    COMMAND ${VCPKG_ROOT}/vcpkg.exe export
        #            --zip
        #            --output-dir=${VCPKG_EXPORT_DIR}
        #            --triplet=${VCPKG_TARGET_TRIPLET}
        #            --x-all-installed
        #    COMMENT "Exporting vcpkg dependencies to ${VCPKG_EXPORT_DIR}"
        #    VERBATIM
        #)
        #
        #add_custom_target(
        #    VCPKG_EXPORT_DEPS
        #    DEPENDS "${VCPKG_EXPORT_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin"
        #)
        #
        #add_dependencies(Astrocelerate VCPKG_EXPORT_DEPS)


        # Recursively install all DLLs from the vcpkg exported bin directory
        file(GLOB VCPKG_EXPORTED_DLLS "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/*.dll")

        if (VCPKG_EXPORTED_DLLS)
            list(LENGTH VCPKG_EXPORTED_DLLS DLL_COUNT)
            message(STATUS "Including ${DLL_COUNT} vcpkg-exported DLLs for installation.")
            install(FILES ${VCPKG_EXPORTED_DLLS} DESTINATION bin)
        else()
            message(WARNING "No vcpkg-exported DLLs found at \"${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/*.dll\"!")
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
find_package(assimp CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)


# Organize libraries into variables
    # For information on the library names, search for them in "vcpkg/ports/[LIBRARY]/usage" either locally or from vcpkg's repository
set(PKG_LIBS
    Vulkan::Vulkan
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    assimp::assimp
    yaml-cpp::yaml-cpp
)

set(FETCHCONTENT_LIBS
    #assimp
)

set(EXTERNAL_LIBS
    Boxer
)

set(FETCHCONTENT_LIBS_TARGET_INCLUDE_DIRS
    ${glm_SOURCE_DIR} 
    ${GLFW_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${VulkanMemoryAllocator_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
    ${assimp_BINARY_DIR}/include
)


# Automate compilation of shader files into SPIR-V bytecode with glslc.exe
    CompileShaders()


# Add subdirectories
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/boxer")

    # Enable UTF-8 support on Windows machines
    if (WIN32)
       target_compile_definitions(Boxer PRIVATE UNICODE)
    endif (WIN32)


# Link libraries
target_link_libraries(Astrocelerate PRIVATE ${PKG_LIBS} ${FETCHCONTENT_LIBS} ${EXTERNAL_LIBS})


# Include library directories
target_include_directories(Astrocelerate PRIVATE 
    ${HEADER_DIRS}
    ${FETCHCONTENT_LIBS_TARGET_INCLUDE_DIRS}
)


# Installation commands
    # Main executable
install(TARGETS Astrocelerate
        RUNTIME DESTINATION bin
)

    # Compiled shaders
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/Shaders"
        DESTINATION bin
)

    # DLLs
InstallRuntimeDLLs()

    # Simulation samples
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/samples/"
        DESTINATION samples
)

    # Application assets
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
        DESTINATION assets
)

    # Configurations
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/configs/"
        DESTINATION configs
)


# Deployment through CPack
    # General configuration
    set(CPACK_PACKAGE_NAME "${APP_NAME}")
    set(CPACK_MSI_PRODUCT_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
    set(CPACK_PACKAGE_VERSION "${CPACK_MSI_PRODUCT_VERSION}")
    set(CPACK_PACKAGE_VERSION_MAJOR "${APP_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${APP_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${APP_VERSION_PATCH}")
    set(CPACK_PACKAGE_VENDOR "${AUTHOR} (D.B.A. ${APP_VENDOR})")
    #set(CPACK_PACKAGE_CONTACT "email@example.com")

    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Astrocelerate is Vietnam's first independent orbital mechanics and spaceflight simulation engine, designed from the ground up to serve as a sovereign alternative to foreign aerospace software.")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${APP_NAME} ${APP_VERSION_FULL}")

    set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")


    # Platform-specific installers
        # Development: Override platform to generate installers
            set(PLATFORM_WIN32 (WIN32 OR TRUE))
            set(PLATFORM_APPLE (APPLE OR FALSE))
            set(PLATFORM_LINUX (LINUX OR FALSE))

    set(INSTALLER_NAME "${APP_NAME}-${APP_VERSION_FULL}")

    if (PLATFORM_WIN32)
        set(CPACK_PACKAGE_FILE_NAME "${INSTALLER_NAME}-x64-win32")
    
        set(CPACK_GENERATOR "WIX")
        set(CPACK_WIX_VERSION 4)
        set(CPACK_WIX_EXTENSIONS "WixToolset.UI.wixext")

        # GUIDs are crucial for MSI packages for upgrades
        set(CPACK_WIX_UPGRADE_GUID "92C5A6B3-CC39-46C2-BA8A-2EB695156DBF")

        # Banner and dialog bitmaps
        set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/assets/App/Installer/Banner.bmp")
        set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/assets/App/Installer/Dialog.bmp")

        # Custom UI
        set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/scripts/InstallerUI.wxs")
        set(CPACK_WIX_UI_REF "WixUI_InstallDir")

        # Languages (cultures)
        set(CPACK_WIX_CULTURES "en-US")
    endif()

# Include CPack at the end so that it could read the configuration (environmental variables)
# DO NOT PUT THIS BEFORE THE CONFIG PART! I WASTED 12 HOURS OF MY FUCKING LIFE ON THIS!! READ THE DOCS PEOPLE!!!
include(CPack)