/* FilePathUtils.hpp - Utilities pertaining to files and file-paths.
*/

#pragma once

#include <string>
#include <concepts>
#include <filesystem>

#include <Core/Application/LoggingManager.hpp>


namespace FilePathUtils {
	/* Joins multiple paths.
		@param root: The root path.
		@param paths...: The paths to be joined to the root path.

		@return The full path as a string.
	*/
	template<typename... Paths>
		requires(std::convertible_to<Paths, std::string> && ...) // Ensures all paths are convertible to std::string
	inline std::string joinPaths(const std::string& root, const Paths&... paths) {
		std::filesystem::path fullPath = root;
		(fullPath /= ... /= paths);
		return fullPath.string();
	}


	/* Reads a file in binary mode.
		@param filePath: The path to the file to be read. If file path is relative, you must specify the working directory.
		@param workingDirectory (optional): The path to the file. By default, it is set to the binary directory. It is optional, but must be specified if the provided file path is relative.

		@return A byte array (vector of type char) containing the file's content.
	*/
	inline std::vector<char> readFile(const std::string& filePath, std::string workingDirectory = "") {
		if (filePath.empty()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "File path is empty!");
		}

		// Joins the working directory and the file
		std::filesystem::path workingDir = workingDirectory;
		std::filesystem::path pathToFile = filePath;

		std::filesystem::path absoluteFilePath;
		if (workingDirectory.empty()) {
			absoluteFilePath = pathToFile;
		}
		else {
			// Treats filePath as relative
			pathToFile = pathToFile.relative_path();
			absoluteFilePath = workingDir / pathToFile;
		}


		// Reads the file to an ifstream
		// Flag std::ios::ate means "Start reading from the end of the file". Doing so allows us to use the read position to determine the file size and allocate a buffer.
		// Flag std::ios::binary means "Read the file as a binary file (to avoid text transformations)"
		std::ifstream file(absoluteFilePath, std::ios::ate | std::ios::binary);

		// If file cannot open
		if (!file.is_open()) {
			std::string relativePathErrMsg = " The file may not be in the directory " + enquote(workingDirectory) + '.'
				+ "\n" + "To change the working directory, please specify the full path to the file.";

			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to open file " + enquote(absoluteFilePath.string()) + "!"
				+ ((!workingDirectory.empty()) ? relativePathErrMsg : "")
			);
		}


		// Allocates a buffer using the current reading position
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);


		// Seeks back to the beginning of the file and reads all of the bytes at once
		file.seekg(0);
		file.read(buffer.data(), fileSize);


		// If file is incomplete/not read successfully
		LOG_ASSERT(file, "Failed to read file " + enquote(filePath) + "!");

		file.close();


		return buffer;
	}
}
