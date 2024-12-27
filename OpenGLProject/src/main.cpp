#define GLM_ENABLE_EXPERIMENTAL

#include "Constants.h"
#include "utils/LoggingUtils.h"

#include "buffers/BufferObjects.h"
#include "buffers/VertexArrayObject.h"

#include "rendering/Shader.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"

#include "objects/Camera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <utility>
#include <vector>
#include <map>


int main(void) {
    GLFWwindow* window;

    int windowWidth = Window::DEFAULT_WINDOW_WIDTH;
    int windowHeight = Window::DEFAULT_WINDOW_HEIGHT;

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
    window = glfwCreateWindow(windowWidth, windowHeight, Window::WINDOW_NAME, NULL, NULL);
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

    // Load image icon
    const int numOfAppIcons = 1;
    GLFWimage icons[numOfAppIcons];
    for (int i = 1; i <= numOfAppIcons; i++) {
        int iconWidth, iconHeight, iconChannels;
        std::string filePath = FilePath::WINDOW_ICONS_PREFIX + std::to_string(i) + ".png";
        unsigned char* pixels = stbi_load(filePath.c_str(), &iconWidth, &iconHeight, &iconChannels, 0);

        icons[i - 1].width = iconWidth;
        icons[i - 1].height = iconHeight;
        icons[i - 1].pixels = pixels;
    }

    glfwSetWindowIcon(window, numOfAppIcons, icons);

    // Enable debug context (Should not be enabled in production)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure synchronous output and avoid race conditions
    glDebugMessageCallback(openGLDebugCallback, nullptr);


    // Array of 3D vertices
    Vertex3D vertexData[] = {
        { -0.5f, 0.0f, -0.5f,     0.7f, 1.0f, 0.4f,   0.0f, 1.0f},          // 0: back-left-bottom
        { 0.5f, 0.0f, -0.5f,      1.0f, 0.5f, 0.4f,   1.0f, 1.0f},          // 1: back-right-bottom
        { -0.5f, 0.0f, 0.5f,       0.9f, 0.1f, 0.4f,   0.0f, 0.0f},          // 2: front-left-bottom
        { 0.5f, 0.0f, 0.5f,      0.1f, 0.3f, 0.0f,   1.0f, 0.0f},          // 3: front-right-bottom
        { 0.0f, 0.8f, 0.0f,       0.1f, 0.3f, 0.0f,   0.5f, 0.5f},          // 4: apex
        //{-0.5f, 0.8f,       1.0f, 1.0f, 0.6f},          // 4
        //{0.5f, 0.8f,        1.0f, 1.0f, 1.0f},          // 5
        //{0.0f, 0.5f,        0.5f, 1.0f, 0.0f}           // 6
    };

    // Indices for elements in vertexData (pyramid)
    unsigned int vertexIndices[] = {
        // Pyramid base
        0, 1, 2,
        3, 1, 2,

        // Pyramid sides
        0, 1, 4,
        1, 3, 4,
        3, 2, 4,
        2, 0, 4
    };

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a vertex array object (VAO)
    VertexArray VAO;

    // Create a vertex buffer object (VBO)
    VertexBuffer VBO(vertexData, sizeof(vertexData));
    //VertexBuffer VBO_1(testData, sizeof(testData));


    // Create an index buffer object (IBO), a.k.a. element buffer object (EBO), hence the GL_ELEMENT_ARRAY_BUFFER
    IndexBuffer IBO(vertexIndices, sizeof(vertexIndices), GL_UNSIGNED_INT);
    //IndexBuffer IBO_1(testIndices, sizeof(testIndices));


    // Create a vertex buffer layout and Set up a vertex attribute pointer (to define the data layout)
    VertexBufferLayout VBLayout;
    VBLayout.push<Vertex3D>(0, 3, sizeof(Vertex3D), offsetof(Vertex3D, x));
    VBLayout.push<Vertex3D>(1, 3, sizeof(Vertex3D), offsetof(Vertex3D, r));
    VBLayout.push<Vertex3D>(2, 2, sizeof(Vertex3D), offsetof(Vertex3D, texX));
    VAO.addBuffer(VBO, VBLayout);

    /* Unbind the VAO, VBO and IBO after uploading   their data to the GPU via glBufferData(...)
       Reason: Doing so ensures that future buffer-binding operations don't modify the VAO data and VBO-IBO configurations.
    */
    VBO.unbind();
    IBO.unbind();
    VAO.unbind();

    // Load shaders from file
    Shader shader(FilePath::BASIC_SHADERS_DIR);
    unsigned int shaderID = shader.getShaderID();

    // Create a renderer
    Renderer renderer;

    // Load 2D texture of application logo
    Texture appLogo("assets/textures/developer/dev_orange.png");
    appLogo.bind(0);
    shader.bind();

    unsigned int texSlot0 = glGetUniformLocation(shaderID, "tex0");
    glUniform1i(texSlot0, 0);

    // Enable depth testing (which tells OpenGL what triangles to render and what triangles to hide based on visibility of each one
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, windowWidth, windowHeight);

    // Create camera object
    Camera camera(windowWidth, windowHeight, glm::vec3(0.0f, 0.0f, -3.0f), shader);
    camera.configurePerspective(60.0f, 0.1f, 1000.0f, "u_Perspective");
    camera.configureProjection("u_Projection");

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        renderer.clear(GL_COLOR_BUFFER_BIT);
        renderer.clear(GL_DEPTH_BUFFER_BIT);

        shader.bind();
        VBO.bind(); // VBO is already bound on initialization. However, with multiple VBOs, the VertexBuffer::bind() function is necessary to switch between them
        appLogo.bind(0);

        camera.updateInputs(window);
        camera.updateMatrices();

        //glDrawElements(GL_TRIANGLES, sizeof(vertexIndices) / sizeof(unsigned int), GL_UNSIGNED_INT, nullptr);
        renderer.draw(VAO, IBO, shader);
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}