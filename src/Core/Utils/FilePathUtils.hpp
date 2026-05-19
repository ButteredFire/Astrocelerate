/* FilePathUtils.hpp - Utilities pertaining to files and file-paths.
*/

#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <concepts>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <libgen.h> // For dirname
#include <limits.h> // For PATH_MAX
#include <unistd.h> // For readlink
#elif __APPLE__
#include <mach-o/dyld.h> // For _NSGetExecutablePath
#include <limits.h> // For PATH_MAX
#endif

#ifdef CreateFile  // Undefine Windows' CreateFile macro to prevent conflict with FilePathUtils::CreateFile
#undef CreateFile
#endif

#include <Core/Application/IO/LoggingManager.hpp>


namespace FilePathUtils {
	// Concept: Is std::string or STL character buffer
	template<typename T>
	concept FileContent = requires(const T &content) {
		{ content.data() } -> std::convertible_to<const char*>;
		{ content.size() } -> std::convertible_to<std::size_t>;
	};



	/* 	Gets the directory of the application executable.
		This function is platform-specific and uses different methods to retrieve the executable path based on the operating system.
		@return The directory of the executable as a std::filesystem::path.
	*/
	inline std::filesystem::path GetExecDir() {
		char path_buffer[FILENAME_MAX];
		std::string executablePath;

#ifdef _WIN32
		// Windows: GetModuleFileNameW for wide characters, then convert to narrow
		DWORD length = GetModuleFileNameA(NULL, path_buffer, sizeof(path_buffer));
		if (length == 0 || length == sizeof(path_buffer)) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to get executable path on Windows.");
		}
		executablePath = path_buffer;
#elif __linux__
		// Linux: readlink /proc/self/exe
		ssize_t length = readlink("/proc/self/exe", path_buffer, sizeof(path_buffer) - 1);
		if (length == -1) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to get executable path on Linux (readlink /proc/self/exe failed).");
		}
		path_buffer[length] = '\0'; // Null-terminate the string
		executablePath = path_buffer;
#elif __APPLE__
		// macOS: _NSGetExecutablePath
		uint32_t buffer_size = sizeof(path_buffer);
		if (_NSGetExecutablePath(path_buffer, &buffer_size) != 0) {
			// If the buffer was too small, we should try again with a larger buffer, but we'll just throw for now for the sake of simplicity
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "Failed to get executable path on macOS (buffer too small or other error).");
		}
		executablePath = path_buffer;
#else
#error "Unsupported operating system."
#endif

		// Extract the directory part
		size_t lastSlashPos = executablePath.find_last_of("/\\");
		if (lastSlashPos == std::string::npos) {
			// If no slash is found, it means the executable is in the current directory
			return std::filesystem::current_path();
		}
		return std::filesystem::path(executablePath.substr(0, lastSlashPos));
	}


	/* Joins multiple paths.
		@param root: The root path.
		@param paths...: The paths to be joined to the root path.

		@return The full path as a string.
	*/
	template<typename... Paths>
		requires(std::convertible_to<Paths, std::string> && ...)
	inline std::string JoinPaths(const std::string& root, const Paths&... paths) {
		std::filesystem::path fullPath = root;
		(fullPath /= ... /= paths);
		return fullPath.string();
	}


	/* Gets the parent directory of an absolute file path.
		@param filePath: The absolute file path.

		@return The parent directory of the path.
	*/
	inline std::string GetParentDirectory(const std::string& filePath) {
		if (filePath.empty()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "File path is empty!");
		}
		std::filesystem::path path(filePath);
		if (!std::filesystem::exists(path)) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "File does not exist: " + enquote(filePath));
		}
		return path.parent_path().string();
	}


	/* Gets the file name from a path.
		@param filePath: The file path (relative or absolute).
		@param includeExtension (Default: True): Should the file extension be included in the name?

		@return The file name as a string.
	*/
	inline std::string GetFileName(const std::string &filePath, bool includeExtension = true) {
		if (filePath.empty()) {
			throw Log::RuntimeException(__FUNCTION__, __LINE__, "File path is empty!");
		}
		std::filesystem::path path(filePath);
		if (includeExtension) {
			return path.filename().string();
		}
		else {
			return path.stem().string();
		}
	}


	/* Gets the file extension from its path.
		@param filePath: The file path (relative or absolute).

		@return The file extension as a string.
	*/
	inline std::string GetFileExtension(const std::string &filePath) {
		std::filesystem::path path = filePath;
		return path.extension().string();
	}


	/* Reads a file in binary mode.
		@param filePath: The path to the file to be read. If file path is relative, you must specify the working directory.
		@param workingDirectory (optional): The path to the file. By default, it is set to the binary directory. It is optional, but must be specified if the provided file path is relative.

		@return The file contents.
	*/
	inline std::vector<char> ReadFile(const std::string& filePath, std::string workingDirectory = "") {
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


	/* Gets the lines of a file as a vector of strings (lines).
		@param fileContent: The content of the file as a vector of characters (a byte vector).
		@return A vector of strings, each string being a line in the file.
	*/
	inline std::vector<std::string> GetFileLines(const std::vector<char> &fileContent) {
		std::string contentStr(fileContent.begin(), fileContent.end());

		std::stringstream ss(contentStr);
		std::vector<std::string> lines;

		std::string line;
		while (std::getline(ss, line))
			lines.push_back(line);

		return lines;
	}


	/* Does a path (file/directory) exist on disk?
		@param path: The specified path.

		@return True if the path exists, False otherwise.
	*/
	inline bool PathExists(const std::string &path) {
		return std::filesystem::exists(std::filesystem::path(path));
	}


	/* Creates a directory.
		@param path: The specified directory path.
	*/
	inline void CreatePath(const std::string &path) {
		try {
			std::filesystem::create_directories(path);
		}
		catch (const std::filesystem::filesystem_error &e) {
			Log::Print(Log::T_ERROR, __FUNCTION__, "Failed to create path " + enquote(path) + "!\n\n" + "Reason: " + e.what());
			return;
		}
	}


	/* Creates a file.
		@param filePath: The specified path to the file.
	*/
	inline void CreateFile(const std::string &filePath) {
		std::filesystem::path path(filePath);

		if (path.has_parent_path()) {
			std::filesystem::create_directories(path.parent_path());
		}

		std::ofstream file(filePath);
		file.close();
	}


	/* Writes to a file.
		@note The file will be automatically created if it does not exist.

		@param filePath: The path to the file to be read.
		@param fileContent: The content to be written to the file as a vector of characters (a byte vector).
		@param openMode (default: std::ios::binary): The open mode(s) for the file.
	*/
	template<FileContent Content>
	inline void WriteToFile(const std::string &filePath, const Content &fileContent, std::ios::openmode openMode = std::ios::binary) {
		if (!PathExists(filePath))
			CreateFile(filePath);

		// Write to file
		std::ofstream file(filePath, std::ios::binary);
		LOG_ASSERT(file.is_open(),
			"Failed to open file " + enquote(filePath) + " for writing!"
		);

		file.write(fileContent.data(), fileContent.size());
		LOG_ASSERT(file,
			"Failed to write to file " + enquote(filePath) + "!",
			Log::T_FATAL
		);

		file.close();
	}
}
