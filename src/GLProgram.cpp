#include "../include/GLProgram.h"
#include "glm/ext.hpp"

GLProgram::GLProgram() {}

void GLProgram::init(const char* vertexPath, const char* fragmentPath, const char* whiteFragmentPath) {

    // initialize window system
    SDL_SetMainReady();
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Video initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

    this->window = SDL_CreateWindow("3D Surfaces",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      this->windowWidth, this->windowHeight,
      SDL_WINDOW_OPENGL);
    if (this->window == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL window initialization failed: %s\n", SDL_GetError());
      cleanup(CleanupMode::sdl_quit);
      exit(2);
    }

    this->context = SDL_GL_CreateContext(this->window);
    if (this->context == NULL)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create OpenGL context: %s\n", SDL_GetError());
      cleanup(CleanupMode::sdl_destroy_window);
      exit(3);
    }

    if (SDL_GL_MakeCurrent(window, context) < 0)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can not set SDL GL context as current context: %s\n", SDL_GetError());
      cleanup(CleanupMode::sdl_gl_delete_context);
      exit(4);
    }

    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (this->renderer == NULL)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL GL renderer initialization failed: %s", SDL_GetError());
      cleanup(CleanupMode::sdl_gl_delete_context);
      exit(5);
    }

    if (SDL_GetRendererInfo(renderer, &info) < 0)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not retrieve SDL renderer info: %s", SDL_GetError());
      cleanup(CleanupMode::sdl_destroy_renderer);
      exit(6);
    }

    SDL_GL_SetSwapInterval(1);

    GLint max_units;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_units);
    if (max_units < 1) {
      fprintf(stderr, "Your GPU does not have any vertex texture image units\n");
      cleanup(CleanupMode::sdl_destroy_renderer);
      exit(7);
    }

    TwInit(TW_OPENGL_CORE, NULL);
    TwWindowSize(this->windowWidth, this->windowHeight);
    this->myBar = TwNewBar("Settings");
    TwAddVarRW(this->myBar, "Camera eye Z", TW_TYPE_FLOAT, &this->camera.z_position,
               " label='Camera eye position Z' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Camera eye Y", TW_TYPE_FLOAT, &this->camera.y_position,
               " label='Camera eye position Y' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Camera eye X", TW_TYPE_FLOAT, &this->camera.x_position,
               " label='Camera eye position X' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Camera center Z", TW_TYPE_FLOAT, &this->camera.z_front,
               " label='Camera center position Z' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Camera center Y", TW_TYPE_FLOAT, &this->camera.y_front,
               " label='Camera center position Y' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Camera center X", TW_TYPE_FLOAT, &this->camera.x_front,
               " label='Camera center position X' min=-100.0 max=100.0 step=0.1 ");
    TwAddVarRW(this->myBar, "Plot index", TW_TYPE_INT32, &this->current_index,
               " label='Plot index' min=0 max=2 ");

    this->current_index = static_cast<int>(SurfacePlotter::PlotIndex::plot_sombrero);

    // GL calls
    glViewport(0, 0, this->windowWidth, this->windowHeight);
    glEnable(GL_DEPTH_TEST);

    // init shaders
    this->shader = Shader(vertexPath, fragmentPath);
    this->whiteShader = Shader(vertexPath, whiteFragmentPath);

    bind_indices_once.resize(2);
    for (auto &flag: bind_indices_once)
      flag = false;

    // set up VAOs and VBOs and EBOs
    initDrawingData();
}

void GLProgram::run(void) {
    int handled;
    this->quit = 0;

    // main loop
    while (!this->quit) {

        // input
        //processInput();

        glClearColor(this->clearColor.r, this->clearColor.g, this->clearColor.b, this->clearColor.alpha);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set up shader and transformation matrices
        // TODO: condense this part
        glm::mat4 viewMatrix = getViewMatrix();
        glm::mat4 projectionMatrix = getProjectionMatrix();
        int zRange = this->surfacePlotter.getZRange();
        this->shader.use();
        this->shader.setFloatUniform("zRange", (zRange == 0) ? 1.0f : zRange);
        this->shader.setFloatUniform("zMin", this->surfacePlotter.getZMin());
        this->shader.setMat4Uniform("view", viewMatrix);
        this->shader.setMat4Uniform("projection", projectionMatrix);
        this->shader.setMat4Uniform("model", getDefaultModelMatrix() * modelMatrix);
        this->whiteShader.use();
        this->whiteShader.setMat4Uniform("view", viewMatrix);
        this->whiteShader.setMat4Uniform("projection", projectionMatrix);
        this->whiteShader.setMat4Uniform("model", getDefaultModelMatrix() * modelMatrix);

        // render
        this->surfacePlotter.generateSurfacePlot(static_cast<SurfacePlotter::PlotIndex>(this->current_index), bind_indices_once[0]);
        drawSurfacePlot();
        drawCube();

        TwDraw();

        // check and call events and swap buffers
        while (SDL_PollEvent(&this->event)) {
          handled = TwEventSDL(&this->event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
          if (!handled) {
            if (this->event.type == SDL_QUIT) {
              this->quit = 1;
            }
          }
        }
        SDL_GL_SwapWindow(this->window);
    }
}

void GLProgram::initDrawingData(void) {

    // SURFACE PLOT

    // generate surface plot VAO and VBO and EBO
    this->surfacePlotVAO = generateVAO();
    this->surfacePlotVBO = generateBuffer();
    this->surfacePlotEBO = generateBuffer();

    glBindVertexArray(this->surfacePlotVAO);

    // set VBO data
    glBindBuffer(GL_ARRAY_BUFFER, this->surfacePlotVBO);

    // set EBO data
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBO);

    // vertices attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // CUBE

    // generate cube VAO and VBO and EBO
    this->cubeVAO = generateVAO();
    this->cubeVBO = generateBuffer();
    this->cubeEBO = generateBuffer();

    glBindVertexArray(this->cubeVAO);

    // set VBO data
    glBindBuffer(GL_ARRAY_BUFFER, this->cubeVBO);

    // set EBO data
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->cubeEBO);

    // vertices attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void GLProgram::drawSurfacePlot(void) {
    this->shader.use();
    glBindVertexArray(this->surfacePlotVAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->surfacePlotVBO);
    glBufferData(GL_ARRAY_BUFFER, this->surfacePlotter.getNumElements()*sizeof(float), this->surfacePlotter.getVertices(), GL_DYNAMIC_DRAW);
    if (!bind_indices_once[0]) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotter.getNumIndices()*sizeof(uint), this->surfacePlotter.getIndices(), GL_DYNAMIC_DRAW);
      bind_indices_once[0] = 1;
    }
    glDrawElements(GL_LINES, this->surfacePlotter.getNumIndices(),GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GLProgram::drawCube(void) {
    this->whiteShader.use();
    glBindVertexArray(this->cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, this->cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, 24*sizeof(float), this->surfacePlotter.getCubeVertices(), GL_STATIC_DRAW);
    if (!bind_indices_once[1]) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->cubeEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24*sizeof(uint), this->surfacePlotter.getCubeIndices(), GL_STATIC_DRAW);
      bind_indices_once[1] = 1;
    }
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GLProgram::cleanup(CleanupMode cm) {
    switch (cm) {
    case CleanupMode::delete_buffers:
      glDeleteVertexArrays(1, &(this->surfacePlotVAO));
      glDeleteBuffers(1, &(this->surfacePlotVBO));
      glDeleteBuffers(1, &this->surfacePlotEBO);

      glDeleteVertexArrays(1, &(this->cubeVAO));
      glDeleteBuffers(1, &(this->cubeVBO));
      glDeleteBuffers(1, &this->cubeEBO);
      [[fallthrough]];
    case CleanupMode::tw_terminate:
      TwTerminate();
      [[fallthrough]];
    case CleanupMode::sdl_destroy_renderer:
      SDL_DestroyRenderer(this->renderer);
      [[fallthrough]];
    case CleanupMode::sdl_gl_delete_context:
      SDL_GL_DeleteContext(this->context);
      [[fallthrough]];
    case CleanupMode::sdl_destroy_window:
      SDL_DestroyWindow(this->window);
      [[fallthrough]];
    case CleanupMode::sdl_quit:
      SDL_Quit();
      break;
    }
}

void GLProgram::setClearColor(float r, float g, float b, float alpha) {
    this->clearColor = {r, g, b, alpha};
}

uint GLProgram::generateBuffer(void) {
    uint buf;
    glGenBuffers(1, &buf);
    return buf;
}

uint GLProgram::generateVAO(void) {
    uint vao;
    glGenVertexArrays(1, &vao);
    return vao;
}

glm::mat4 GLProgram::getViewMatrix(void) {
    return camera.getViewMatrix();
}

glm::mat4 GLProgram::getProjectionMatrix(void) {
    return glm::perspective(glm::radians(camera.zoom), (float) this->windowWidth / (float) this->windowHeight, 0.1f, 99999.0f);
}

glm::mat4 GLProgram::getDefaultModelMatrix(void) {
    //return glm::mat4(1.0f); // identity
    return glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

/*
void GLProgram::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}
*/

/*
void GLProgram::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
            mousePressed = true;
        }
        else if (action == GLFW_RELEASE) {
            mousePressed = false;
        }
    }
}
*/

/*
void GLProgram::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}
*/

/*
void GLProgram::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {

    if (mousePressed) {

        // get current cursor coordinates
        double currMouseX, currMouseY;
        glfwGetCursorPos(window, &currMouseX, &currMouseY);

        // get points on arcball
        glm::vec3 va = getArcballVector(prevMouseX, prevMouseY);
        glm::vec3 vb = getArcballVector(currMouseX, currMouseY);

        float speedFactor = 0.1f;
        float angleOfRotation = speedFactor * acos(MIN(1.0f, glm::dot(va, vb)));

        // to get the axis of rotation, need to convert from camera coordinates to world coordinates
        glm::vec3 axisCamera = glm::cross(va, vb);
        glm::mat3 cameraToModel = glm::inverse(glm::mat3(camera.getViewMatrix() * modelMatrix));
        glm::vec3 axisModel = cameraToModel * axisCamera;

        // update model rotation matrix
        float tolerance = 1e-4;
        if (angleOfRotation > tolerance)
            modelMatrix = glm::rotate(modelMatrix, glm::degrees(angleOfRotation), axisModel);

        // update cursor position
        prevMouseX = currMouseX;
        prevMouseY = currMouseY;
    }
}
*/

glm::vec3 GLProgram::getArcballVector(float x, float y) {

    // get normalized vector from center of the virtual arcball to a point P on the arcball's surface
    // if (x,y) is too far away from the arcball, return the nearest point on the arcball's surface
    glm::vec3 P(x/windowWidth * 2 - 1.0, y/windowHeight * 2 - 1.0, 0.0f);
    P.y = -P.y;

    float radius = 1.0f;
    float OP_squared = P.x * P.x + P.y * P.y;

    if (OP_squared <= radius)
        P.z = sqrt(radius - OP_squared); // apply pythagorean theorem to find z
    else
        P = glm::normalize(P); // nearest point

    return P;
}

/*
void GLProgram::processInput(void) {

    // close window with 'ESC' key
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);

    // camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
}
*/
