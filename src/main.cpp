#include <cstdio>
#include <windows.h>

#include "utils/utils.hpp"
#include "generator.hpp"

#define WIDTH 10u
#define HEIGHT 10u

int main() {
  // Assuming the executable is launching from its own directory
  SetCurrentDirectory("../../../src");

  // GLFW init
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Window init
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "", NULL, NULL);
  if (!window) {
    printf("Failed to create GFLW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);

  // GLAD init
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return EXIT_FAILURE;
  }

  glViewport(0, 0, WIDTH, HEIGHT);

  // generator::genMask("pre_mask0.png", "pre_mask1.png");
  generator::genDistanceField("mask0.png", "mask1.png", GL_R8UI);

  GLenum glEror = glGetError();
  if (glEror != GL_NO_ERROR)
    error("GL error detected, type 0x%x");

  glfwTerminate();
  puts("Done");

  return 0;
}

