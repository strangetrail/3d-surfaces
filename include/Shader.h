#ifndef SHADER_H
#define SHADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
  uint ID;
  Shader();
  Shader(const char *vertexPath, const char *fragmentPath);
  void use(void);
  void setIntUniform(const std::string &name, int value) const;
  void setFloatUniform(const std::string &name, float value) const;
  void setVec3Uniform(const std::string &name, glm::vec3 value) const;
  void setMat4Uniform(const std::string &name, glm::mat4 value) const;
  void cleanup(void);

private:
  void checkCompileErrors(uint shader, std::string type);
};

#endif // SHADER_H
