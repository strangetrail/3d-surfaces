#include "../include/Camera.h"

// default constructor
Camera::Camera()
    : z_position(30.0f), y_position(0.0f), x_position(0.0f), z_front(-0.5f), y_front(0.0f), x_front(0.0f), up(1.0f, 0.0f, 0.0f),
      right(0.0f, -1.0f, 0.0f), movementSpeed(15.0f), zoom(45.0f), zoomSpeed(2.0f) {}

glm::mat4 Camera::getViewMatrix(void) {
  return glm::lookAt(glm::vec3(this->x_position, this->y_position, this->z_position),
                     glm::vec3(this->x_front, this->y_front, this->z_front), this->up);
}
