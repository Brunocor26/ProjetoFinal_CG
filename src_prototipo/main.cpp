#include "Mesh.hpp"
#include "objloader.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/transform.h>
#include <vector>

// Maze Map Data (copied from labirintos.h to avoid GLEW conflict)
int mapaInicio[20][20] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

const float BLOCK_SIZE = 30.0f;

// Camera setup
Camera camera(glm::vec3(300.0f, 600.0f, 600.0f), glm::vec3(0.0f, 1.0f, 0.0f),
              -90.0f, -45.0f); // Positioned to see the maze
float lastX = 900.0f / 2.0;
float lastY = 600.0f / 2.0;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Store all user input states (mouse, keyboard)
struct InputState {
  float lightRotationSpeed = 1.0f; // Speed of light orbiting
  bool lightPaused = false;        // Is light rotation paused?
  float lightAngle = 0.0f;         // Current light position angle
  bool wireframe = false;          // Show wireframe mode?
  bool blinn = false;              // Blinn-Phong lighting?

  // Store if key was pressed (prevents repeated triggers)
  bool spacePressed = false;
  bool plusPressed = false;
  bool minusPressed = false;
  bool rPressed = false;
  bool fPressed = false;
  bool bPressed = false;
};

// Transform for the model (maze root)
Transform modelTransform;

// Read keyboard and mouse input and update the InputState
void processInput(GLFWwindow *window, InputState &input) {
  // Close window if ESC is pressed
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  // Camera movement
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);

  // Pause/unpause light rotation with SPACE
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!input.spacePressed) {
      input.lightPaused = !input.lightPaused;
      std::printf("Light rotation: %s\n",
                  input.lightPaused ? "PAUSED" : "RUNNING");
      input.spacePressed = true;
    }
  } else {
    input.spacePressed = false;
  }

  // Toggle wireframe mode with F key
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    if (!input.fPressed) {
      input.wireframe = !input.wireframe;
      if (input.wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      std::printf("Wireframe: %s\n", input.wireframe ? "ON" : "OFF");
      input.fPressed = true;
    }
  } else {
    input.fPressed = false;
  }

  // Toggle Blinn-Phong with B key
  if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
    if (!input.bPressed) {
      input.blinn = !input.blinn;
      std::printf("Blinn-Phong: %s\n", input.blinn ? "ON" : "OFF");
      input.bPressed = true;
    }
  } else {
    input.bPressed = false;
  }

  // Reset everything to initial state with R key
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    if (!input.rPressed) {
      input.lightRotationSpeed = 1.0f;
      input.lightPaused = false;
      input.lightAngle = 0.0f;
      input.wireframe = false;
      input.blinn = false;
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      camera =
          Camera(glm::vec3(300.0f, 600.0f, 600.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                 -90.0f, -45.0f); // Reset camera
      std::printf("State reset\n");
      input.rPressed = true;
    }
  } else {
    input.rPressed = false;
  }
}

// Mouse callback
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

// Scroll callback
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.ProcessMouseScroll(yoffset);
}

// Create and setup OpenGL window with GLFW and GLAD
GLFWwindow *initWindow(int width, int height, const char *title) {
  if (!glfwInit()) {
    std::fprintf(stderr, "GLFW init falhou\n");
    return nullptr;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *win = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!win) {
    std::fprintf(stderr, "Falha a criar janela\n");
    glfwTerminate();
    return nullptr;
  }
  glfwMakeContextCurrent(win);
  glfwSetCursorPosCallback(win, mouse_callback);
  glfwSetScrollCallback(win, scroll_callback);

  // Tell GLFW to capture our mouse
  glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::fprintf(stderr, "Failed to initialize GLAD\n");
    return nullptr;
  }

  glfwSetFramebufferSizeCallback(
      win, [](GLFWwindow *, int w, int h) { glViewport(0, 0, w, h); });

  return win;
}

// Create a cube mesh with normals
Mesh *createCubeMesh() {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  // Helper to add face
  auto addFace = [&](glm::vec3 normal, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
                     glm::vec3 v4) {
    unsigned int startIdx = vertices.size();
    vertices.push_back({v1, normal});
    vertices.push_back({v2, normal});
    vertices.push_back({v3, normal});
    vertices.push_back({v4, normal});
    indices.push_back(startIdx);
    indices.push_back(startIdx + 1);
    indices.push_back(startIdx + 2);
    indices.push_back(startIdx + 2);
    indices.push_back(startIdx + 3);
    indices.push_back(startIdx);
  };

  // Front (Z+)
  addFace({0, 0, 1}, {-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5}, {0.5, 0.5, 0.5},
          {-0.5, 0.5, 0.5});
  // Back (Z-)
  addFace({0, 0, -1}, {0.5, -0.5, -0.5}, {-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5},
          {0.5, 0.5, -0.5});
  // Left (X-)
  addFace({-1, 0, 0}, {-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5}, {-0.5, 0.5, 0.5},
          {-0.5, 0.5, -0.5});
  // Right (X+)
  addFace({1, 0, 0}, {0.5, -0.5, 0.5}, {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5},
          {0.5, 0.5, 0.5});
  // Top (Y+)
  addFace({0, 1, 0}, {-0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {0.5, 0.5, -0.5},
          {-0.5, 0.5, -0.5});
  // Bottom (Y-)
  addFace({0, -1, 0}, {-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, -0.5, 0.5},
          {-0.5, -0.5, 0.5});

  return new Mesh(vertices, indices);
}

int main() {
  // initialize window
  GLFWwindow *win = initWindow(900, 600, "TP2 - 3D Maze");
  if (!win) // treat if error creating window
    return -1;

  // Create cube mesh for walls
  Mesh *wallMesh = createCubeMesh();

  // Compile and link Phong shader program
  Shader phongShader(FileSystem::getPath("shaders/phong.vert").c_str(),
                     FileSystem::getPath("shaders/phong.frag").c_str());

  // State for inputs
  InputState input;

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark gray background

  glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D effect

  while (!glfwWindowShouldClose(win)) {
    // Per-frame time logic
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Read user input
    processInput(win, input);

    // Clear screen for next frame
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update light position angle if not paused
    if (!input.lightPaused) {
      input.lightAngle += input.lightRotationSpeed * 0.01f;
    }

    // Setup camera and projection (handles window resize)
    int fbw, fbh;
    glfwGetFramebufferSize(win, &fbw, &fbh);
    float aspect = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;

    // Use Camera class for view matrix
    glm::mat4 view = camera.GetViewMatrix();
    // Create perspective projection (45 degree field of view)
    glm::mat4 proj =
        glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 2000.0f);

    // Activate shader
    phongShader.use();

    // Calculate light position orbiting around the center of the maze
    float lightRadius = 400.0f;
    glm::vec3 dynamicLightPos =
        glm::vec3(300.0f + lightRadius * std::cos(input.lightAngle), 300.0f,
                  300.0f + lightRadius * std::sin(input.lightAngle));

    // Send lights to shader
    phongShader.setInt("numLights", 1);

    // Convert light position to camera space
    glm::vec4 lightPosEye = view * glm::vec4(dynamicLightPos, 1.0f);

    phongShader.setVec4("lights[0].Position", lightPosEye);
    phongShader.setVec3("lights[0].La", 0.2f, 0.2f, 0.2f);
    phongShader.setVec3("lights[0].Ld", 0.8f, 0.8f, 0.8f);
    phongShader.setVec3("lights[0].Ls", 1.0f, 1.0f, 1.0f);

    // Send material values to shader (Wall material)
    phongShader.setVec3("Material.Ka", 0.2f, 0.2f, 0.2f);
    phongShader.setVec3("Material.Kd", 0.6f, 0.6f, 0.6f); // Grey walls
    phongShader.setVec3("Material.Ks", 0.1f, 0.1f, 0.1f);
    phongShader.setFloat("Material.Shininess", 16.0f);

    // Blinn-Phong toggle
    phongShader.setBool("blinn", input.blinn);

    // Render Maze
    for (int row = 0; row < 20; row++) {
      for (int col = 0; col < 20; col++) {
        if (mapaInicio[row][col] == 1) {
          // Calculate position
          float x = col * BLOCK_SIZE;
          float z = row * BLOCK_SIZE;
          float y = 0.0f;

          // Model matrix for this wall block
          glm::mat4 model = glm::mat4(1.0f);
          model = glm::translate(model, glm::vec3(x, y, z));
          model =
              glm::scale(model, glm::vec3(BLOCK_SIZE)); // Scale to block size

          // Calculate transformation matrices for shader
          glm::mat4 modelView = view * model;
          glm::mat4 MVP = proj * modelView;
          glm::mat3 normalMatrix =
              glm::mat3(glm::transpose(glm::inverse(modelView)));

          // Send matrices to shader
          phongShader.setMat4("ModelViewMatrix", modelView);
          phongShader.setMat4("MVP", MVP);
          phongShader.setMat3("NormalMatrix", normalMatrix);

          wallMesh->Draw(phongShader.ID);
        }
      }
    }

    // Display rendered image on screen
    glfwSwapBuffers(win);
    // Handle window events (close, resize, etc.)
    glfwPollEvents();
  }

  // Clean up memory
  delete wallMesh;
  // Close OpenGL window and cleanup
  glfwTerminate();
}