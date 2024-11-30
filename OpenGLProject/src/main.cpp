#include "Constants.h"
#include "Shaders.h"
#include "LoggingUtils.h"

#include "BufferObjects.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <utility>
#include <vector>
#include <map>


struct Vertex2D {
    float x, y;        // Positions (x, y)
    float r, g, b, a;  // Color values (RGBA)
};


int main(void) {
    GLFWwindow* window;

    // Initialize GLFW
    if (!glfwInit()) {
        logError(Error::CANNOT_INIT_GLFW, "Cannot initialize library: GLFW.");
        return Error::CANNOT_INIT_GLFW;
    }

    // Force OpenGL version 4.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // Set OpenGL profile
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(Window::WINDOW_WIDTH, Window::WINDOW_HEIGHT, Window::WINDOW_NAME, NULL, NULL);
    if (!window) {
        logError(Error::CANNOT_INIT_WINDOW, "Cannot initialize window.");
        glfwTerminate();
        return Error::CANNOT_INIT_WINDOW;
    }

    // Make the window's context current and initialize GLEW (can only be initialized with a valid OpenGL rendering context)
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        logError(Error::CANNOT_INIT_GLEW, "Cannot initialize library: GLEW.");
        return Error::CANNOT_INIT_GLEW;
    }

    glfwSwapInterval(1);


    glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Dark teal background
    printAppInfo();

    // TODO: Load image icon

    // Enable debug context (Should not be enabled in production)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure synchronous output and avoid race conditions
    glDebugMessageCallback(openGLDebugCallback, nullptr);


    // Define 2 triangles as an array of vertices
    Vertex2D vertexData[] = {
        // triangle 1
        { -0.5f, -0.5f}, // 0
        { 0.5f, 0.5f },  // 1
        { 0.5f, -0.5f }, // 2
        { -0.5f, 0.5f }  // 3
    };

    // Indices for elements in vertexData
    unsigned int vertexIndices[] = {
        0, 1, 2, // triangle 1
        0, 1, 3  // triangle 2
    };

    // Create a vertex array object (VAO)
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Create a vertex buffer object (VBO)
        /* Note: Use unique pointer for automatic memory freeing AND make_unique for better memory safety than "new" (keyword).
        Starting from C++14, std::make_unique is the recommended way to create std::unique_ptr instances. */
    std::unique_ptr<VertexBuffer> VBO = std::make_unique<VertexBuffer>(vertexData, sizeof(vertexData));
    
    // Create an index buffer object (IBO), a.k.a. element buffer object (EBO), hence the GL_ELEMENT_ARRAY_BUFFER
    std::unique_ptr<IndexBuffer> IBO = std::make_unique<IndexBuffer>(vertexIndices, sizeof(vertexIndices));
    

    /* Set up a vertex attribute pointer (to define the data layout)
    * glEnableVertexAttribArray(x): Enables the vertex attribute at location = 0 (refer to shader file -> vertex shader)
    */
    glEnableVertexAttribArray(0);
    const void* offset = (const void*) offsetof(Vertex2D, x);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), offset);

    /* Unbind the VAO, VBO and IBO after uploading their data to the GPU via glBufferData(...)
    * Reason: Doing so ensures that future buffer-binding operations don't modify the VAO data and VBO-IBO configurations.
    */
    glBindVertexArray(0); // VAO
    VBO->unbind();
    IBO->unbind();

    // Load shaders from file
    ShaderSources shaderSources = parseShaderFile(FilePath::BASIC_SHADERS_DIR);
    unsigned int shader = createShader(shaderSources);
    glUseProgram(shader);

    // Set up a uniform
    int uniformLocation = glGetUniformLocation(shader, "u_Color");
    _ASSERT(uniformLocation != -1);
    glUniform4f(uniformLocation, 0.0f, 1.0f, 1.0f, 1.0f);
    

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        VBO->bind(); // VBO is already bound on initialization. However, with multiple VBOs, the VertexBuffer::bind() function is necessary to switch between them
        IBO->bind();

        glDrawElements(GL_TRIANGLES, sizeof(vertexIndices), GL_UNSIGNED_INT, nullptr);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}