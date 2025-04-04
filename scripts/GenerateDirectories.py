"""
This Python script generates paths to headers and source files to be compiled in CMake.
"""


import os
import sys
import platform
from pathlib import Path


# Name of the script (used for loggging)
fileName = __file__.replace(str(Path(__file__).parent.parent), "").replace("\\", "/")[1:]

# Writes the file paths of all source and header files to corresponding CMake files. 
def retrieveFileDirectories(startPath, pyFilePath, targetDirs):
    def enquote(s) -> str:
        # Enquotes a string.
        return '"' + s + '"'
    
    def parseNewFile(s) -> str:
        # Parses a path/file to be written into CMake files.
        return "\n\t" + enquote(s)
    
    def pathJoin(a, b, nl=False) -> str:
        # Creates a CMake-compatible path from two sub-paths.
        return parseNewFile(os.path.join(a, b).replace("\\", "/"))
    
    def isOSIncompatible(file) -> bool:
        # Determines whether a file is not compatible with the operating system (based on its name)
        incompat = ("linux" in file.lower() and platform.system() != "Linux") or \
                ("mac" in file.lower() and platform.system() != "Darwin") or \
                ("win" in file.lower() and platform.system() != "Windows")
        
        if incompat:
            print(f"[{fileName}] WARNING: {enquote(file)} is being skipped because it seems to be OS-incompatible.")

        return incompat


    # Replaces backslash chars in the start path with forward-slash chars
    startPath = str(startPath).replace("\\", "/") + "/"

    # Opens/Creates CMake files
    headerDirs = open(os.path.join(pyFilePath, "HeaderDirs.cmake"), "w")
    sourceFiles = open(os.path.join(pyFilePath, "SourceFiles.cmake"), "w")
    headerFiles = open(os.path.join(pyFilePath, "HeaderFiles.cmake"), "w")

    # Initializes content to be written into CMake files
    headerDirsContent = f"""set(HEADER_DIRS"""
    for tgDir in targetDirs:
        headerDirsContent += parseNewFile(tgDir)

    sourceFilesContent = f"""set(SOURCE_FILES"""
    headerFilesContent = f"""set(HEADER_FILES"""

    # File extensions that are allowed to be written into CMake files
    permissibleSourceExts = ["c", "cpp"]
    permissibleHeaderExts = ["h", "hpp"]

    targetDirsFullPath = []
    for dir in targetDirs:
        targetDirsFullPath.append(os.path.join(startPath, dir))
    

    noOfSourceFiles = noOfHeaderFiles = 0
    # Traverses each targeted directory from the start path (project root)
    for root, dirs, files in os.walk(startPath):
        for target in targetDirsFullPath:
            if root.startswith(target):
                # Truncates the start path from the current root to that it becomes a relative path to the project root
                root = root.replace(startPath, "").replace("\\", "/")
                for file in files:
                    ext = file.split(".")[-1]

                    # Skip platform-specific files based on OS
                    if isOSIncompatible(file):
                        continue
                    
                    if ext in permissibleSourceExts:
                        sourceFilesContent += pathJoin(root, file, True)
                        noOfSourceFiles += 1

                    elif ext in permissibleHeaderExts:
                        headerFilesContent += pathJoin(root, file, True)
                        noOfHeaderFiles += 1


    print(f"[{fileName}] Discovered {noOfSourceFiles} source files and {noOfHeaderFiles} header files. Writing to CMake files...", flush=True)

    # Close CMake files
    headerDirsContent += "\n)"
    headerDirs.write(headerDirsContent)
    headerDirs.close()

    sourceFilesContent += "\n)"
    sourceFiles.write(sourceFilesContent)
    sourceFiles.close()

    headerFilesContent += "\n)"
    headerFiles.write(headerFilesContent)
    headerFiles.close()


def main():
    # python [FILE] [TARGET_SOURCE_AND_HEADER_ROOT_DIRS...]
    startDir = "scripts"
    rootAbsDir = Path(__file__).parent.parent
    targetDirs = sys.argv[1:]

    # If no target directories are passed in
    if not targetDirs:
        print(f"[{fileName}] Error: No target directories provided. Exiting...", flush=True)
        sys.exit(1)

    retrieveFileDirectories(rootAbsDir, startDir, targetDirs)
    sys.exit(0)


if __name__ == "__main__":
    main()