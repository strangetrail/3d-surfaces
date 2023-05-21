#ifndef SURFACEPLOTTER_H
#define SURFACEPLOTTER_H

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define PI 3.14159265
#define e 2.71828
#define FLOAT_MIN -2147483648
#define FLOAT_MAX 2147483648

class SurfacePlotter {
private:
  // xy grid
  std::vector<std::vector<glm::vec2>> gridPoints; // 2D array of grid x, y coordinates
  float xMin;
  float xMax;
  float yMin;
  float yMax;
  float gridInterval;
  float zMin;
  float zMax;

  // surface plot data
  float *vertices;
  uint numElements;
  uint *indices; // lines
  uint numIndices;
  uint *triangles; // triangles
  uint numTriangles;

  // cube data
  float *cubeVertices;
  uint *cubeIndices;

public:
  enum class PlotIndex : std::int32_t {
    plot_sombrero = 0,
    plot_quadsin = 1,
    plot_paraboloid = 2,
    plot_004 = 3,
    plot_005 = 4,
    plot_006 = 5
  };

  float a, b, c;

  SurfacePlotter();
  ~SurfacePlotter();

  void setGrid(float xMin, float xMax, float yMin, float yMax, float interval);
  void generateSurfacePlotIndices(PlotIndex plot_index);
  void generateSurfacePlotVertices(PlotIndex plot_index);
  float f(float x, float y,
          PlotIndex plot_index); // mathematical multi-variable function,
                                 // returns z value

  void generateCube(void);

  float getZMin(void);
  float getZMax(void);
  float getZRange(void);

  float *getVertices(void);
  uint getNumElements(void);
  uint *getIndices(void);
  uint getNumIndices(void);
  uint *getTriangles(void);
  uint getNumTriangles(void);

  float *getCubeVertices(void);
  uint *getCubeIndices(void);

  void cleanup();
};

#endif // SURFACEPLOTTER_H
