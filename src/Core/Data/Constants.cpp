#include "Constants.h"

#include <Utils/FilePathUtils.hpp>

const std::filesystem::path EXEC_DIR = FilePathUtils::GetExecDir();
const std::string ROOT_DIR = EXEC_DIR.parent_path().string();


namespace ShaderConsts {
	const std::string VERTEX		= FilePathUtils::JoinPaths(ROOT_DIR, std::string("bin/Shaders/VertexShader.spv"));
	const std::string FRAGMENT		= FilePathUtils::JoinPaths(ROOT_DIR, std::string("bin/Shaders/FragmentShader.spv"));
}


namespace ConfigConsts {
	const std::string IMGUI_DEFAULT_CONFIG = FilePathUtils::JoinPaths(ROOT_DIR, std::string("configs/DefaultImGuiConfig.ini"));
}


namespace FontConsts {
	NotoSansFonts NotoSans = {
		.BOLD				= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-Bold.ttf"),
		.BOLD_ITALIC		= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-BoldItalic.ttf"),
		.ITALIC				= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-Italic.ttf"),
		.LIGHT				= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-Light.ttf"),
		.LIGHT_ITALIC		= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-LightItalic.ttf"),
		.REGULAR			= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSans-Regular.ttf"),
		.REGULAR_MATH		= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSansMath-Regular.ttf"),
		.REGULAR_MONO		= FilePathUtils::JoinPaths(ROOT_DIR, "assets/Fonts/NotoSans/NotoSansMono-Regular.ttf")
	};
}


namespace SimulationConsts {
	// How many frames should be processed concurrently
	
	int MAX_FRAMES_IN_FLIGHT = 3;
}