#include "Constants.h"
#include "Shader.h"
#include "LoggingUtils.h"

#include "BufferObjects.h"
#include "VertexArrayObject.h"
#include "Renderer.h"

#include "Texture.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

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

    // Load image icon
    const int numOfAppIcons = 1;
    GLFWimage icons[numOfAppIcons];
    for (int i = 1; i <= numOfAppIcons; i++) {
        int iconWidth, iconHeight, iconChannels;
        std::string filePath = FilePath::WINDOW_ICONS_PREFIX + std::to_string(i) + ".png";
        unsigned char* pixels = stbi_load(filePath.c_str(), &iconWidth, &iconHeight, &iconChannels, 0);

        icons[i-1].width = iconWidth;
        icons[i-1].height = iconHeight;
        icons[i-1].pixels = pixels;
    }
    
    glfwSetWindowIcon(window, numOfAppIcons, icons);

    // Enable debug context (Should not be enabled in production)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Ensure synchronous output and avoid race conditions
    glDebugMessageCallback(openGLDebugCallback, nullptr);


    // Array of 2D vertices
    Vertex2D vertexData[] = {
        { -0.5f, -0.5f,     0.7f, 1.0f, 0.4f,   0.0f, 0.0f},          // 0
        { 0.5f, 0.5f,       1.0f, 0.5f, 0.4f,   1.0f, 1.0f},          // 1
        { 0.5f, -0.5f,      0.9f, 0.1f, 0.4f,   1.0f, 0.0f},          // 2
        { -0.5f, 0.5f,      0.1f, 0.3f, 0.0f,   0.0f, 1.0f},          // 3
        //{-0.5f, 0.8f,       1.0f, 1.0f, 0.6f},          // 4
        //{0.5f, 0.8f,        1.0f, 1.0f, 1.0f},          // 5
        //{0.0f, 0.5f,        0.5f, 1.0f, 0.0f}           // 6
    };

    // Indices for elements in vertexData
    unsigned int vertexIndices[] = {
        0, 1, 2,     // triangle 1 (bottom-left, top-right, bottom-right)
        0, 1, 3,     // triangle 2 (bottom-left, top-right, top-left)
        //3, 4, 1,     // triangle 3 (top-left, top-left + (y: 0.3), bottom-left)
        //1, 5, 3,     // triangle 4 (bottom-left, top-right + (y: 0.3), top-left)
        //4, 5, 6      // triangle 5 (top-left + (y: 0.3), top-right + (y: 0.3), top-mid)
    };

    /*
    (-0.5, 0.5: 0.0, 1.0)		(0.5, 0.5: 1.0, 1.0)

                          (0, 0)

    (-0.5, -0.5: 0.0, 0.0)		(0.5, -0.5: 1.0, 0.0)
    */

    /*Vertex2D appLogoVertices[] = {
        {-0.7f, 0.6f,      0.0f, 0.0f, 0.0f,     0.0f, 0.0f},
        {-0.7f, 0.9f,      0.0f, 0.0f, 0.0f,     0.0f, 1.0f},
        {0.7f, 0.6f,      0.0f, 0.0f, 0.0f,     1.0f, 0.0f},
        {0.7f, 0.9f,      0.0f, 0.0f, 0.0f,     1.0f, 1.0f},
    };*/

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
    VBLayout.push<Vertex2D>(0, 2, sizeof(Vertex2D), offsetof(Vertex2D, x));
    VBLayout.push<Vertex2D>(1, 3, sizeof(Vertex2D), offsetof(Vertex2D, r));
    VBLayout.push<Vertex2D>(2, 2, sizeof(Vertex2D), offsetof(Vertex2D, texX));
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
    Texture appLogo("assets/textures/DeveloperTextureOrange512x512.png");
    appLogo.bind(0);
    shader.bind();

    unsigned int texSlot0 = glGetUniformLocation(shaderID, "tex0");
    glUniform1i(texSlot0, 0);
    
    int objectScale = glGetUniformLocation(shaderID, "scale");
    float scale = 1.0f;
    glUniform1f(objectScale, scale);

    bool completedCycle = false;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        renderer.clear();

        shader.bind();
        VBO.bind(); // VBO is already bound on initialization. However, with multiple VBOs, the VertexBuffer::bind() function is necessary to switch between them
        appLogo.bind(0);

        // Nauseating zooming effect for funsies
        /*if (scale >= 0.0f) {
            if (completedCycle) {
                if (scale == 1.0f) {
                    completedCycle = false;
                    scale -= 0.01f;
                }
                else scale += 0.01f;
            }
            else scale -= 0.01f;
        }
        else {
            completedCycle = true;
            scale += 0.01f;
        }
        glUniform1f(objectScale, scale);*/

        renderer.draw(VAO, IBO, shader);
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}