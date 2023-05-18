#ifndef GLPROGRAM_H
#define GLPROGRAM_H

#include <iostream>
#include "Shader.h"
#include "SurfacePlotter.h"
#include "Camera.h"

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <AntTweakBar.h>

#define MIN(a, b) ((a) < (b)) ? (a) : (b)

class GLProgram {
    private:
        SDL_Window *window;
        SDL_GLContext context;
        SDL_Renderer *renderer;
        SDL_RendererInfo info;

        TwBar *myBar;

        int quit;

        struct color {
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            float alpha = 1.0f;
        } clearColor;

        Shader shader, whiteShader;
        SurfacePlotter surfacePlotter;
        uint surfacePlotVAO, surfacePlotVBO, surfacePlotEBO;
        uint cubeVAO, cubeVBO, cubeEBO;

        void initDrawingData(void);
        static glm::vec3 getArcballVector(float x, float y); // helper to cursor callback, (x,y) are raw mouse coordinates

    public:
        enum class CleanupMode {sdl_quit, sdl_destroy_window,
          sdl_gl_delete_context, sdl_destroy_renderer, tw_terminate, delete_buffers};

        SDL_Event event;

        static int windowWidth, windowHeight;
        static Camera camera;
        static bool mousePressed;
        static double prevMouseX, prevMouseY;
        static glm::mat4 modelMatrix;

        GLProgram();

        void init(const char* vertexPath, const char* fragmentPath, const char* whiteFragmentPath);
        void run(void);
        void cleanup(CleanupMode cm);

        void setClearColor(float r, float g, float b, float alpha);

        uint generateBuffer(void);
        uint generateVAO(void);

        void drawSurfacePlot(void);
        void drawCube(void);

        // transformation matrices
        glm::mat4 getViewMatrix(void);
        glm::mat4 getProjectionMatrix(void);
        glm::mat4 getDefaultModelMatrix(void);

        // event callback functions
        /*
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        */

        // input
        //void processInput(void);
};

#endif //GLPROGRAM_H
