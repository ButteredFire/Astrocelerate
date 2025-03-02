import os
import sys
from pathlib import Path

def retrieveFileDirectories(startPath, relStartPath, cmakeHeaderDir, cmakeSourceDir, targetDirs):
    def parseNewFile(s):
        return "\n\t" + ('"' + s + '"')
    
    def pathJoin(a, b, nl=False):
        string = '"' + (a + "/" + b).replace("\\", "/") + '"'
        if nl:
            string = "\n\t" + string
        return string


    startPath = str(startPath).replace("\\", "/") + "/"
    #print(f"start path: {startPath}")
    headerDirs = open(cmakeHeaderDir, "w")
    sourceFiles = open(cmakeSourceDir, "w")

    imguiRoot = "external/imgui"
    #imguiBackends = "external/imgui/backends"

    # Fill up with Dear Imgui files in ./external/imgui
    headerDirsContent = f"""set(HEADER_DIRS"""
    headerDirsContent += parseNewFile(imguiRoot)
    for tgDir in targetDirs:
        headerDirsContent += parseNewFile(tgDir)

    # {pathJoin(imguiRoot, "imconfig.h")}
    # {pathJoin(imguiRoot, "imgui.h")}
    # {pathJoin(imguiRoot, "imgui_internal.h")}
    # {pathJoin(imguiRoot, "imstb_rectpack.h")}
    # {pathJoin(imguiRoot, "imstb_textedit.h")}
    # {pathJoin(imguiRoot, "imstb_truetype.h")}
    # {pathJoin(imguiRoot, "imgui_impl_vulkan.h")}
    # {pathJoin(imguiRoot, "imgui_impl_glfw.h")}

    sourceFilesContent = f"""set(SOURCE_FILES
    {pathJoin(imguiRoot, "imgui.cpp")}
    {pathJoin(imguiRoot, "imgui_demo.cpp")}
    {pathJoin(imguiRoot, "imgui_draw.cpp")}
    {pathJoin(imguiRoot, "imgui_tables.cpp")}
    {pathJoin(imguiRoot, "imgui_widgets.cpp")}
    {pathJoin(imguiRoot, "imgui_impl_vulkan.cpp")}
    {pathJoin(imguiRoot, "imgui_impl_glfw.cpp")}
    """


    permissibleSourceExts = ["c", "cpp"]
    permissibleHeaderExts = ["h", "hpp"]

    targetDirsFullPath = []
    for dir in targetDirs:
        targetDirsFullPath.append(os.path.join(startPath, dir))
    

    for root, dirs, files in os.walk(startPath):
        for target in targetDirsFullPath:
            if root.startswith(target):
                root = root.replace(startPath, "").replace("\\", "/")
                # Special case: Dear Imgui files in ./external/imgui
                if root.startswith(imguiRoot):
                    #print("why no activate")
                    continue

                else:
                    #print(root)
                    for file in files:
                        ext = file.split(".")[-1]
                        if ext in permissibleSourceExts:
                            sourceFilesContent += pathJoin(root, file, True)



    headerDirsContent += "\n)"
    headerDirs.write(headerDirsContent)
    headerDirs.close()

    sourceFilesContent += "\n)"
    sourceFiles.write(sourceFilesContent)
    sourceFiles.close()


def main():
    # python [FILE] [headerDirs.cmake] [SourceFiles.cmake] [TARGET_SOURCE_AND_HEADER_ROOT_DIRS...]
    startDir = "scripts"
    rootDir = ""
    rootAbsDir = Path(__file__).parent.parent
    cmakeHeaderDir = os.path.join(startDir, sys.argv[1])
    cmakeSourceDir = os.path.join(startDir, sys.argv[2])
    targetDirs = sys.argv[3:]

    retrieveFileDirectories(rootAbsDir, rootDir, cmakeHeaderDir, cmakeSourceDir, targetDirs)

if __name__ == "__main__":
    main()