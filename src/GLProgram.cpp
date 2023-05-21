#include "../include/GLProgram.h"
#include "glm/ext.hpp"

GLProgram::GLProgram() {}

GLProgram::~GLProgram() { cleanup(GLProgram::CleanupMode::delete_buffers); }

void GLProgram::init(const char *vertexPath, const char *fragmentPath, const char *whiteFragmentPath) {

  // initialize window system
  SDL_SetMainReady();
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Video initialization failed: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  this->window = SDL_CreateWindow("3D Surfaces", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, this->windowWidth, this->windowHeight,
                                  SDL_WINDOW_OPENGL);
  if (this->window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL window initialization failed: %s\n", SDL_GetError());
    cleanup(CleanupMode::sdl_quit);
    exit(2);
  }

  this->context = SDL_GL_CreateContext(this->window);
  if (this->context == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create OpenGL context: %s\n", SDL_GetError());
    cleanup(CleanupMode::sdl_destroy_window);
    exit(3);
  }

  if (SDL_GL_MakeCurrent(window, context) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Can not set SDL GL context as current context: %s\n", SDL_GetError());
    cleanup(CleanupMode::sdl_gl_delete_context);
    exit(4);
  }

  this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
  if (this->renderer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL GL renderer initialization failed: %s", SDL_GetError());
    cleanup(CleanupMode::sdl_gl_delete_context);
    exit(5);
  }

  if (SDL_GetRendererInfo(renderer, &info) < 0) {
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
             " label='Camera eye position Z' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Camera eye Y", TW_TYPE_FLOAT, &this->camera.y_position,
             " label='Camera eye position Y' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Camera eye X", TW_TYPE_FLOAT, &this->camera.x_position,
             " label='Camera eye position X' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Camera center Z", TW_TYPE_FLOAT, &this->camera.z_front,
             " label='Camera center position Z' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Camera center Y", TW_TYPE_FLOAT, &this->camera.y_front,
             " label='Camera center position Y' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Camera center X", TW_TYPE_FLOAT, &this->camera.x_front,
             " label='Camera center position X' group=Camera min=-100.0 max=100.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Plot rotation around z", TW_TYPE_FLOAT, &this->rotation,
             " label='Plot rotation around z' group=Plot min=-360.0 max=360.0 step=0.1 ");
  TwAddVarRW(this->myBar, "Plot index", TW_TYPE_INT32, &this->current_index, " label='Plot index' group=Plot min=0 max=5 ");
  TwAddVarRW(this->myBar, "Enable continuous surface", TW_TYPE_BOOLCPP, &this->continuous,
             " label='Enable continuous surface' group=Plot ");
  TwAddVarRW(this->myBar, "Function parameter a", TW_TYPE_FLOAT, &this->surfacePlotter.a,
             " label='Function parameter a' group=Function min=-100.0 max=100.0 step=0.05 ");
  TwAddVarRW(this->myBar, "Function parameter b", TW_TYPE_FLOAT, &this->surfacePlotter.b,
             " label='Function parameter b' group=Function min=-100.0 max=100.0 step=0.05 ");
  TwAddVarRW(this->myBar, "Function parameter c", TW_TYPE_FLOAT, &this->surfacePlotter.c,
             " label='Function parameter c' group=Function min=-100.0 max=100.0 step=0.05 ");

  this->current_index = static_cast<int>(SurfacePlotter::PlotIndex::plot_sombrero);
  this->continuous = true;

  // GL calls
  glViewport(0, 0, this->windowWidth, this->windowHeight);
  glEnable(GL_DEPTH_TEST);

  // init shaders
  this->shader = Shader(vertexPath, fragmentPath);
  this->whiteShader = Shader(vertexPath, whiteFragmentPath);

  rotation = 0.0f;

  this->surfacePlotter.generateSurfacePlotIndices(static_cast<SurfacePlotter::PlotIndex>(this->current_index));

  // set up VAOs and VBOs and EBOs
  initDrawingData();

  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLProgram::run(void) {
  int handled;
  this->quit = 0;

  // main loop
  while (!this->quit) {

    // input
    // processInput();

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
    this->surfacePlotter.generateSurfacePlotVertices(static_cast<SurfacePlotter::PlotIndex>(this->current_index));
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
  this->surfacePlotEBOTriangles = generateBuffer();

  glBindVertexArray(this->surfacePlotVAO);

  // set VBO data
  glBindBuffer(GL_ARRAY_BUFFER, this->surfacePlotVBO);

  // vertices attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray(0);

  // set indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotter.getNumIndices() * sizeof(uint), this->surfacePlotter.getIndices(),
               GL_STATIC_DRAW);

  // set triangles
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBOTriangles);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotter.getNumTriangles() * sizeof(uint), this->surfacePlotter.getTriangles(),
               GL_STATIC_DRAW);

  // CUBE

  // generate cube VAO and VBO and EBO
  this->cubeVAO = generateVAO();
  this->cubeVBO = generateBuffer();
  this->cubeEBO = generateBuffer();

  glBindVertexArray(this->cubeVAO);

  // set VBO data
  glBindBuffer(GL_ARRAY_BUFFER, this->cubeVBO);

  // vertices attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
  glEnableVertexAttribArray(0);

  // set indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->cubeEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(uint), this->surfacePlotter.getCubeIndices(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void GLProgram::drawSurfacePlot(void) {
  this->shader.use();

  glBindVertexArray(this->surfacePlotVAO);

  glBindBuffer(GL_ARRAY_BUFFER, this->surfacePlotVBO);
  glBufferData(GL_ARRAY_BUFFER, this->surfacePlotter.getNumElements() * sizeof(float), this->surfacePlotter.getVertices(), GL_DYNAMIC_DRAW);

  if (this->continuous) {
    this->shader.setIntUniform("switch_contrast", 0);
    glPolygonOffset(1, 0);
    glEnable(GL_POLYGON_OFFSET_FILL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBOTriangles);
    glDrawElements(GL_TRIANGLES, this->surfacePlotter.getNumTriangles(), GL_UNSIGNED_INT, 0);
  }

  this->shader.setIntUniform("switch_contrast", 1);
  glPolygonOffset(0, 0);
  glDisable(GL_POLYGON_OFFSET_FILL);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->surfacePlotEBO);
  glDrawElements(GL_LINES, this->surfacePlotter.getNumIndices(), GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

void GLProgram::drawCube(void) {
  this->whiteShader.use();
  glBindVertexArray(this->cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, this->cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), this->surfacePlotter.getCubeVertices(), GL_DYNAMIC_DRAW);
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

    this->shader.cleanup();
    this->whiteShader.cleanup();
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

void GLProgram::setClearColor(float r, float g, float b, float alpha) { this->clearColor = {r, g, b, alpha}; }

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

glm::mat4 GLProgram::getViewMatrix(void) { return camera.getViewMatrix(); }

glm::mat4 GLProgram::getProjectionMatrix(void) {
  return glm::perspective(glm::radians(camera.zoom), (float)this->windowWidth / (float)this->windowHeight, 0.1f, 99999.0f);
}

glm::mat4 GLProgram::getDefaultModelMatrix(void) {
  // return glm::mat4(1.0f); // identity
  glm::mat4 z_rotation = glm::rotate(glm::mat4(1.0f), glm::radians(this->rotation), glm::vec3(1.0f, 0.0f, 1.0f));
  return z_rotation * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}
