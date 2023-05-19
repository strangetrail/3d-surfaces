#include "../include/SurfacePlotter.h"

// default constructor
SurfacePlotter::SurfacePlotter() :
    xMin(-20.0f), xMax(20.0f), yMin(-20.0f), yMax(20.0f), gridInterval(0.2f), zMin(FLOAT_MAX), zMax(FLOAT_MIN),
    vertices(NULL), numElements(0), indices(NULL), numIndices(0), cubeVertices(NULL), cubeIndices(NULL) {

    setGrid(this->xMin, this->xMax, this->yMin, this->yMax, this->gridInterval);
    this->cubeIndices = new uint[24] {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };
}

void SurfacePlotter::setGrid(float xMin, float xMax, float yMin, float yMax, float interval) {
    this->xMin = xMin;
    this->xMax = xMax;
    this->yMin = yMin;
    this->yMax = yMax;
    this->gridInterval = interval;

    // empty grid points array
    this->gridPoints.clear();

    // fill grid points array
    for (float x = xMin; x <= xMax; x += interval) {
        std::vector<glm::vec2> temp;
        this->gridPoints.push_back(temp);
        for (float y = yMin; y <= yMax; y += interval) {
            this->gridPoints[this->gridPoints.size()-1].push_back(glm::vec2(x, y));
        }
    }
}

void SurfacePlotter::generateSurfacePlotIndices(PlotIndex plot_index) {
      // indices:

      // deallocte old data
      if (this->indices)
          delete[] this->indices;

      // determine number of rows in x and y axes
      int numX = this->gridPoints.size();
      int numY = this->gridPoints[0].size();

      // determine number of indices
      this->numIndices = (numX * (numY-1) + numY * (numX - 1)) * 2;

      // allocate memory for new data
      this->indices = new uint[this->numIndices];

      int i = 0;

      for (int x = 0; x < numX; ++x) {
          for (int y = 0; y < numY-1; ++y) {
              this->indices[i++] = x*numY + y;
              this->indices[i++] = x*numY + y+1;
          }
      }

      for (int y = 0; y < numY; ++y) {
          for (int x = 0; x < numX-1; ++x) {
              this->indices[i++] = x*numY + y;
              this->indices[i++] = (x+1)*numY + y;
          }
      }
}

void SurfacePlotter::generateSurfacePlotVertices(PlotIndex plot_index) {
    // reset ranges
    this->zMin = FLOAT_MAX;
    this->zMax = FLOAT_MIN;

    // empty grid
    if (this->gridPoints.empty())
        return;

    // vertices:

    // deallocate old data
    if (this->vertices)
        delete[] this->vertices;

    // determine number of rows in x and y axes
    int numX = this->gridPoints.size();
    int numY = this->gridPoints[0].size();

    // allocate memory for new data
    this->numElements = 3 * numX * numY;
    this->vertices = new float[this->numElements];

    // generate vertices
    for (int x = 0; x < numX; ++x) {
        for (int y = 0; y < numY; ++y) {

            // add vertex
            this->vertices[(x * numY + y) * 3 + 0] = this->gridPoints[x][y].x; // x
            this->vertices[(x * numY + y) * 3 + 1] = this->gridPoints[x][y].y; // y
            this->vertices[(x * numY + y) * 3 + 2] = f(this->gridPoints[x][y].x, this->gridPoints[x][y].y, plot_index);
        }
    }

    generateCube();
}

float SurfacePlotter::f(float x, float y, PlotIndex plot_index) {

    // EQUATION
    float z;
    switch (plot_index) {
    case PlotIndex::plot_sombrero:
      //float z = sin(t) * 8*sin(sqrt(pow(x, 2) + pow(y, 2))) / sqrt(pow(x, 2) + pow(y, 2)); // sombrero equation
      z = 8*sin(sqrt(pow(x, 2) + pow(y, 2))) / sqrt(pow(x, 2) + pow(y, 2)); // sombrero equation
      break;
    case PlotIndex::plot_quadsin:
      z = sin(pow(x/2.5, 2) + pow(y/2.5, 2));
      break;
    case PlotIndex::plot_paraboloid:
      z = (pow(x/1.5,2) + pow(y/1.5,2)) * 0.03; // parabaloid
      break;
    }

    // update z ranges
    if (z < this->zMin)
        this->zMin = z;
    if (z > this->zMax)
        this->zMax = z;

    return z;
}

void SurfacePlotter::generateCube(void) {

    // empty grid
    if (this->gridPoints.empty())
        return;

    // vertices:

    // deallocate old data
    if (this->cubeVertices)
        delete[] this->cubeVertices;

    this->cubeVertices = new float[24] {
        this->xMax, this->yMin, this->zMin,
        this->xMax, this->yMax, this->zMin,
        this->xMin, this->yMax, this->zMin,
        this->xMin, this->yMin, this->zMin,
        this->xMax, this->yMin, this->zMax,
        this->xMax, this->yMax, this->zMax,
        this->xMin, this->yMax, this->zMax,
        this->xMin, this->yMin, this->zMax
    };
}

float SurfacePlotter::getZMin(void) {
    return this->zMin;
}

float SurfacePlotter::getZMax(void) {
    return this->zMax;
}

float SurfacePlotter::getZRange(void) {
    return this->zMax - this->zMin;
}

float* SurfacePlotter::getVertices(void) {
    return this->vertices;
}

uint SurfacePlotter::getNumElements(void) {
    return this->numElements;
}

uint* SurfacePlotter::getIndices(void) {
    return this->indices;
}

uint SurfacePlotter::getNumIndices(void) {
    return this->numIndices;
}

float* SurfacePlotter::getCubeVertices(void) {
    return this->cubeVertices;
}

uint* SurfacePlotter::getCubeIndices(void) {
    return this->cubeIndices;
}
