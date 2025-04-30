/* ModelLoader.hpp - Defines a model loader.
*/

#pragma once

#include <tinyobjloader/tiny_obj_loader.h>

#include <Core/LoggingManager.hpp>

#include <Engine/Components/ModelComponents.hpp>


class ModelLoader {
public:
	enum FileType {
		T_OBJ
	};

	/* Loads a model.
		@param filePath: The path to the model file.
		@param fileType: The model file type.

		@return A mesh component.
	*/
	static Component::Mesh loadModel(const std::string& filePath, FileType fileType);
};