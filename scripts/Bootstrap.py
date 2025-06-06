"""
This Python script automates the CMake setup process.
"""


import os
import subprocess
import sys
import shutil
from pathlib import Path

# Name of the script (used for loggging)
fileName = __file__.replace(str(Path(__file__).parent.parent), "").replace("\\", "/")[1:]


def getToolchainFile(vcpkgRootPath):
    """ Constructs the path to the vcpkg toolchain file. """

    vcpkgRootPath = vcpkgRootPath.replace("\\", "/")
    return os.path.join(vcpkgRootPath, "scripts/buildsystems/vcpkg.cmake").replace("\\", "/")


def checkBootstrapVcpkg(vcpkgRoot):
    """ Builds the vcpkg binary if it hasn't been already. """

    vcpkgExe = "vcpkg.exe" if sys.platform == "win32" else "vcpkg"
    vcpkgBin = os.path.join(vcpkgRoot, vcpkgExe)

    if not os.path.isfile(vcpkgBin):
        print("[INFO] vcpkg not built. Bootstrapping...")
        subprocess.run([os.path.join(vcpkgRoot, "bootstrap-vcpkg.bat" if sys.platform == "win32" else "bootstrap-vcpkg.sh")], cwd=vcpkgRoot)



def findVcpkgRoot():
    """
        Attempts to find the vcpkg root directory either via the VCPKG_ROOT environment variable, or via user input.
        1. VCPKG_ROOT environment variable
        2. User input (prompting for path)
    """

    vcpkgRoot = os.environ.get("VCPKG_ROOT")
    if vcpkgRoot:
        print(f"[INFO] Found VCPKG_ROOT environment variable: {vcpkgRoot}")
        if os.path.exists(getToolchainFile(vcpkgRoot)):
            return vcpkgRoot
        else:
            print(f"[WARNING] VCPKG_ROOT ('{vcpkgRoot}') does not contain vcpkg.cmake!")


    while True:
        vcpkgRoot = input("Please enter the path to your vcpkg installation (e.g., C:/vcpkg or /home/user/vcpkg). It must NOT contain spaces: ")
        
        vcpkgRoot = os.path.abspath(vcpkgRoot).replace("\\", "/")

        if " " in vcpkgRoot and sys.platform == "win32":
            print("[ERROR]: The vcpkg path on Windows should NOT contain spaces.")
            continue

        toolchainFileDir = getToolchainFile(vcpkgRoot)
        if os.path.exists(toolchainFileDir):
            print(f"[INFO] Found vcpkg toolchain at: {toolchainFileDir}")
            return vcpkgRoot
        else:
            print(f"[ERROR] vcpkg toolchain file not found at '{toolchainFileDir}'. Please ensure the path is correct.")


def runCMakeCONFIGURE(cmakeFileDir, buildDir, vcpkgRoot, buildType="Debug"):
    """ Runs the CMake configure command. """

    # Creates build directory if it doesn't exist
    os.makedirs(buildDir, exist_ok=True)

    cmakeArgs = [
        "cmake",
        "-S", cmakeFileDir,
        "-B", buildDir,
        f"-DCMAKE_TOOLCHAIN_FILE={getToolchainFile(vcpkgRoot)}",
        f"-DCMAKE_BUILD_TYPE={buildType}" # Add build type
    ]

    
    if generator := os.environ.get("CMAKE_GENERATOR"):
        print(f"[INFO] Configuration will use the generator '{generator}'.")
        cmakeArgs.extend(["-G", generator])
    # TIL there is an operator in Python called the "walrus operator". If we were to use the == operator, the code below would look like this:
    #
    # generator = os.environ.get("CMAKE_GENERATOR")
    # if generator:
    #     cmakeArgs.extend(["-G", generator])
    #
    # Pretty neat!

    print(f"\n[INFO] Running CMake configure command: {' '.join(cmakeArgs)}")

    try:
        subprocess.run(cmakeArgs, check=True)
        print("[INFO] CMake configure successful!")
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] CMake configure failed with exit code {e.returncode}")
        sys.exit(1)


def runCMakeBUILD(buildDir, buildType="Debug"):
    """ Runs the CMake build command. """

    if not os.path.exists(buildDir):
        print(f"[ERROR] Build directory '{buildDir}' not found. Please run configure first.")
        sys.exit(1)

    cmakeArgs = [
        "cmake",
        "--build", buildDir,
        "--config", buildType # NOTE: This is important for multi-config generators like Visual Studio
    ]

    print(f"\n[INFO] Running CMake build command: {' '.join(cmakeArgs)}")
    try:
        subprocess.run(cmakeArgs, check=True)
        print("[INFO] CMake build successful!")
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] CMake build failed with exit code {e.returncode}")
        sys.exit(1)


def cleanBuildDirectory(buildDir):
    """ Removes the build directory. """

    if os.path.exists(buildDir):
        print(f"[INFO] Removing build directory: {buildDir}")
        shutil.rmtree(buildDir)
        print("[INFO] Build directory cleaned.")
    else:
        print("[INFO] Build directory does not exist, nothing to clean.")


def printUsage():
    print("Usage: python Bootstrap.py [configure|build|clean] [Debug|Release]")
    print("\tconfigure: Configures the CMake project.")
    print("\tbuild: Builds the CMake project.")
    print("\tclean: Removes the build directory.")
    print("\t[Debug|Release]: Optional build type for configure/build commands (default: Debug).")


def main():
    print(f"RUNNING: {' '.join(sys.argv)}\n")

    if len(sys.argv) < 2:
        printUsage()
        sys.exit(0)


    command = sys.argv[1].lower()
    buildType = "Debug"

    cmakeFileDir = Path(__file__).parent.parent.__str__()
    buildDir = os.path.join(cmakeFileDir, "build", buildType)

    if len(sys.argv) > 2:
        argBuildType = sys.argv[2].lower()
        if argBuildType in ["debug", "release", "relwithdebinfo", "minsizerelease"]:
            buildType = argBuildType.capitalize() # Capitalizes first letter for CMake
        else:
            print(f"[WARNING] Unknown build type '{argBuildType}'. Using default '{buildType}'.")


    if command == "configure":
        vcpkgRoot = findVcpkgRoot()
        checkBootstrapVcpkg(vcpkgRoot)

        if vcpkgRoot:
            runCMakeCONFIGURE(cmakeFileDir, buildDir, vcpkgRoot, buildType)

    elif command == "build":
        runCMakeBUILD(buildDir, buildType)

    elif command == "clean":
        cleanBuildDirectory(buildDir)

    else:
        print(f"Unknown command: {command}")
        printUsage()
        sys.exit(1)


if __name__ == "__main__":
    main()