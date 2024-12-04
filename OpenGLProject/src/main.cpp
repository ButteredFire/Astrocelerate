#include "Constants.h"
#include "Shaders.h"
#include "LoggingUtils.h"

#include "BufferObjects.h"
#include "VertexArrayObject.h"

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


    // Array of 2D vertices
    Vertex2D vertexData[] = {
        // triangle 1
        { -0.5f, -0.5f},    // 0
        { 0.5f, 0.5f},      // 1
        { 0.5f, -0.5f},     // 2
        { -0.5f, 0.5f},     // 3
        {-0.5f, 0.8f},      // 4
        {0.5f, 0.8f}        // 5
    };

    // Indices for elements in vertexData
    unsigned int vertexIndices[] = {
        0, 1, 2,     // triangle 1
        0, 1, 3,     // triangle 2
        3, 4, 1,     // triangle 3
        1, 5, 3      // triangle 4
    };

    float testData[] = {
        -0.5f, -0.5f,
        0.5f, 0.5f,
        0.5f, -0.5f,
        -0.5f, 0.5f,
    };
    unsigned int testIndices[] = {
        0, 1, 2, 3, 4, 5, // triangle 1
        0, 1, 2, 3, 6, 7  // triangle 2
    };

    // Create a vertex array object (VAO)
    VertexArray VAO;
    
    // Create a vertex buffer object (VBO)
    VertexBuffer VBO(vertexData, sizeof(vertexData));
    //VertexBuffer VBO_1(testData, sizeof(testData));

    
    // Create an index buffer object (IBO), a.k.a. element buffer object (EBO), hence the GL_ELEMENT_ARRAY_BUFFER
    IndexBuffer IBO(vertexIndices, sizeof(vertexIndices));
    //IndexBuffer IBO_1(testIndices, sizeof(testIndices));


    /* Create a vertex buffer layout and Set up a vertex attribute pointer (to define the data layout)
    * glEnableVertexAttribArray(x): Enables the vertex attribute at location = x (refer to shader file . vertex shader)
    */
    VertexBufferLayout VBLayout;
    VBLayout.push<Vertex2D>(2, sizeof(Vertex2D), offsetof(Vertex2D, x));
    VAO.addBuffer(VBO, VBLayout);
    
    /*glEnableVertexAttribArray(0);
    const void* offset = (const void*) offsetof(Vertex2D, x);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), offset);*/

    /* Unbind the VAO, VBO and IBO after uploading   their data to the GPU via glBufferData(...)
    * Reason: Doing so ensures that future buffer-binding operations don't modify the VAO data and VBO-IBO configurations.
    */
    VBO.unbind();
    IBO.unbind();
    VAO.unbind();

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

        VAO.bind();
        VBO.bind(); // VBO is already bound on initialization. However, with multiple VBOs, the VertexBuffer::bind() function is necessary to switch between them
        IBO.bind();

        glDrawElements(GL_TRIANGLES, sizeof(vertexIndices) / sizeof(float), GL_UNSIGNED_INT, nullptr);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}