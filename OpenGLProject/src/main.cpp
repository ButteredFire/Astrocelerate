#include "Constants.h"
#include "Shaders.h"
#include "LoggingUtils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <utility>
#include <vector>
#include <map>


struct Vertex2D {
    float x, y;     // Positions (x, y)
};


int main(void) {
    GLFWwindow* window;

    // Initialize GLFW
    if (!glfwInit()) {
        logError(Error::CANNOT_INIT_GLFW, "Cannot initialize library: GLFW.");
        return Error::CANNOT_INIT_GLFW;
    }

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

    // Make sure GPU supports OpenGL 4.3+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // Dark teal background
    printAppInfo();

    // Enable debug context (Should not be enabled in production)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure synchronous output and avoid race conditions
    glDebugMessageCallback(openGLDebugCallback, nullptr);

    glEnable(GL_INVALID_ENUM); // Invalid capability enum


    // Define 2 triangles as an array of vertices
    Vertex2D vertexPositions[] = {
        // triangle 1
        { -0.5f, -0.5f}, // 0
        { 0.5f, 0.5f },  // 1
        { 0.5f, -0.5f }, // 2
        { -0.5f, 0.5f }  // 3
    };
    //unsigned int numOfTriangles = (sizeof(vertexPositions) / sizeof(Vertex2D)) / 3;

    // Indices for elements in vertexPositions
    unsigned int vertexIndices[] = {
        0, 1, 2, // triangle 1
        0, 1, 3  // triangle 2
    };

    // Create a vertex buffer object (VBO)
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // Select (Bind) the created buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW); // Set up buffer data (create a new data store for the VBO)
    glEnableVertexAttribArray(0); // Enable position attribute: Location 0: (float x, y;)

    /* Set up a vertex attribute pointer (to define the data layout)
        Note: offsetof(Vertex2D, x) = 0 because the offset is set to the `x` member in the Vertex2D struct, which is at index 0
          If the vertex contains, for instance, (float r, g, b;), then you must enable color attribute with Location 1: glEnableVertexAttribArray(1)
          then create a vertex attribute pointer like so: glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*) offsetof(Vertex2D, r));
    */
    const void* offset = (const void*) offsetof(Vertex2D, x);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), offset);

    // Load shaders from file
    ShaderSources shaderSources = parseShaderFile(FilePath::BASIC_SHADERS_DIR);
    unsigned int shader = createShader(shaderSources);
    glUseProgram(shader);

    
    // Create an index buffer object (IBO)
    unsigned int IBO;
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndices), vertexIndices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        //glDrawArrays(GL_TRIANGLES, 0, sizeof(vertexPositions) / sizeof(Vertex2D));
        glDrawElements(GL_TRIANGLES, sizeof(vertexIndices), GL_UNSIGNED_INT, nullptr);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}