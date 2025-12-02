#include "Mesh.hpp"
#include "objloader.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <learnopengl/camera.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/transform.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// #include "../include/common.h"
#define PORTA 8080

// Struct for network data
struct Jogador {
  float x;
  float y;
  float velocidade;
};

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

// Camera setup (First Person)
Camera camera(glm::vec3(45.0f, 15.0f, 45.0f)); // Start inside the maze (1,1)
float lastX = 900.0f / 2.0;
float lastY = 600.0f / 2.0;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Network variables
bool jogadorPresente = true;
std::string ipCliente = "127.0.0.1";
int linhaTunel = 9;

// Flashlight state
bool flashlightOn = false;
bool fPressed = false;

// Forward declaration
void enviarJogador();

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

// Check if a specific point is inside a wall
bool checkWall(float x, float z) {
  int gridX = (int)(x / BLOCK_SIZE);
  int gridZ = (int)(z / BLOCK_SIZE);

  if (gridX < 0 || gridX >= 20 || gridZ < 0 || gridZ >= 20)
    return true; // Out of bounds

  return mapaInicio[gridZ][gridX] == 1;
}

// Check collision with maze walls using a radius
bool checkCollision(glm::vec3 pos) {
  float radius = 5.0f; // Player radius buffer

  if (checkWall(pos.x + radius, pos.z))
    return true;
  if (checkWall(pos.x - radius, pos.z))
    return true;
  if (checkWall(pos.x, pos.z + radius))
    return true;
  if (checkWall(pos.x, pos.z - radius))
    return true;

  return false;
}

// Process input with collision detection
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (!jogadorPresente)
    return;

  float velocity = camera.MovementSpeed * deltaTime;
  glm::vec3 nextPos = camera.Position;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    nextPos += camera.Front * velocity;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    nextPos -= camera.Front * velocity;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    nextPos -= camera.Right * velocity;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    nextPos += camera.Right * velocity;

  // Simple collision check (treat player as a point)
  // Check X and Z separately for sliding along walls
  if (!checkCollision(
          glm::vec3(nextPos.x, camera.Position.y, camera.Position.z))) {
    camera.Position.x = nextPos.x;
  }
  if (!checkCollision(
          glm::vec3(camera.Position.x, camera.Position.y, nextPos.z))) {
    camera.Position.z = nextPos.z;
  }

  // Check for tunnel exit
  int gridX = (int)(camera.Position.x / BLOCK_SIZE);
  int gridZ = (int)(camera.Position.z / BLOCK_SIZE);

  if (gridZ == linhaTunel && gridX >= 19) {
    std::cout << "Tunnel reached! Sending player..." << std::endl;
    enviarJogador();
  }

  // Toggle flashlight with F
  if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
    if (!fPressed) {
      flashlightOn = !flashlightOn;
      std::cout << "Flashlight: " << (flashlightOn ? "ON" : "OFF") << std::endl;
      fPressed = true;
    }
  } else {
    fPressed = false;
  }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

void enviarJogador() {
  std::cout << "=== Tentando enviar jogador ===" << std::endl;
  std::cout << "IP destino: " << ipCliente << ":" << PORTA << std::endl;

  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0) {
    std::cerr << "Erro ao criar socket cliente: " << strerror(errno)
              << std::endl;
    return;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORTA);

  if (inet_pton(AF_INET, ipCliente.c_str(), &serverAddress.sin_addr) <= 0) {
    std::cerr << "Endereço inválido: " << ipCliente << std::endl;
    close(clientSocket);
    return;
  }

  if (connect(clientSocket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) < 0) {
    std::cerr << "Erro ao conectar ao servidor: " << strerror(errno)
              << std::endl;
    close(clientSocket);
    return;
  }

  // Send dummy player data (position not strictly needed if client resets pos)
  Jogador j;
  j.x = camera.Position.x;
  j.y = camera.Position.z; // Send Z as Y for 2D client compatibility
  j.velocidade = 0;

  ssize_t bytesSent = send(clientSocket, &j, sizeof(Jogador), 0);
  if (bytesSent < 0) {
    std::cerr << "Erro ao enviar dados: " << strerror(errno) << std::endl;
  } else {
    std::cout << "✓ Jogador enviado com sucesso!" << std::endl;
    jogadorPresente = false;
  }

  close(clientSocket);
}

GLFWwindow *initWindow(int width, int height, const char *title) {
  if (!glfwInit())
    return nullptr;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *win = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!win) {
    glfwTerminate();
    return nullptr;
  }
  glfwMakeContextCurrent(win);
  glfwSetCursorPosCallback(win, mouse_callback);
  glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return nullptr;

  glfwSetFramebufferSizeCallback(
      win, [](GLFWwindow *, int w, int h) { glViewport(0, 0, w, h); });

  return win;
}

int main(int argc, char **argv) {
  if (argc > 1)
    ipCliente = argv[1];

  // Randomize tunnel
  srand(time(NULL));
  linhaTunel = rand() % 18 + 1;
  mapaInicio[9][19] = 1;          // Close default
  mapaInicio[linhaTunel][19] = 0; // Open new
  mapaInicio[linhaTunel][18] = 0;

  GLFWwindow *win = initWindow(900, 600, "Host - 3D First Person");
  if (!win)
    return -1;

  Mesh *wallMesh = createCubeMesh();
  Shader phongShader(FileSystem::getPath("shaders/phong.vert").c_str(),
                     FileSystem::getPath("shaders/phong.frag").c_str());

  glEnable(GL_DEPTH_TEST);

  // Set faster movement speed
  camera.MovementSpeed = 50.0f;

  while (!glfwWindowShouldClose(win)) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(win);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (jogadorPresente) {
      int fbw, fbh;
      glfwGetFramebufferSize(win, &fbw, &fbh);
      float aspect = (fbh == 0) ? 1.0f : (float)fbw / (float)fbh;

      glm::mat4 view = camera.GetViewMatrix();
      glm::mat4 proj =
          glm::perspective(glm::radians(45.0f), aspect, 0.1f, 2000.0f);

      phongShader.use();

      // Lighting
      phongShader.setInt("numLights", 1);
      glm::vec4 lightPosEye =
          view * glm::vec4(300.0f, 500.0f, 300.0f, 1.0f); // Overhead light
      phongShader.setVec4("lights[0].Position", lightPosEye);
      phongShader.setVec3("lights[0].La", 0.3f, 0.3f, 0.3f);
      phongShader.setVec3("lights[0].Ld", 0.8f, 0.8f, 0.8f);
      phongShader.setVec3("lights[0].Ls", 1.0f, 1.0f, 1.0f);

      // Material
      phongShader.setVec3("Material.Ka", 0.2f, 0.2f, 0.2f);
      phongShader.setVec3("Material.Kd", 0.6f, 0.6f, 0.6f);
      phongShader.setVec3("Material.Ks", 0.1f, 0.1f, 0.1f);
      phongShader.setFloat("Material.Shininess", 32.0f);

      // Flashlight uniforms
      phongShader.setBool("flashlightOn", flashlightOn);
      // Position and Direction in View Space (Camera is at 0,0,0 looking down
      // -Z)
      phongShader.setVec3("flashlight.position", 0.0f, 0.0f, 0.0f);
      phongShader.setVec3("flashlight.direction", 0.0f, 0.0f, -1.0f);
      phongShader.setFloat("flashlight.cutOff", glm::cos(glm::radians(12.5f)));
      phongShader.setFloat("flashlight.outerCutOff",
                           glm::cos(glm::radians(17.5f)));
      phongShader.setVec3("flashlight.La", 0.0f, 0.0f,
                          0.0f); // No ambient for flashlight
      phongShader.setVec3("flashlight.Ld", 1.0f, 1.0f, 1.0f); // Bright white
      phongShader.setVec3("flashlight.Ls", 1.0f, 1.0f, 1.0f);
      phongShader.setFloat("flashlight.constant", 1.0f);
      phongShader.setFloat("flashlight.linear", 0.007f);
      phongShader.setFloat("flashlight.quadratic", 0.0002f);

      // Draw Maze
      for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 20; col++) {
          if (mapaInicio[row][col] == 1) {
            float x = col * BLOCK_SIZE;
            float z = row * BLOCK_SIZE;

            // Offset by half block size to align with grid cells [0, 30]
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x + BLOCK_SIZE / 2.0f, 0.0f,
                                                    z + BLOCK_SIZE / 2.0f));
            model = glm::scale(model, glm::vec3(BLOCK_SIZE));

            glm::mat4 modelView = view * model;
            glm::mat4 MVP = proj * modelView;
            glm::mat3 normalMatrix =
                glm::mat3(glm::transpose(glm::inverse(modelView)));

            phongShader.setMat4("ModelViewMatrix", modelView);
            phongShader.setMat4("MVP", MVP);
            phongShader.setMat3("NormalMatrix", normalMatrix);

            wallMesh->Draw(phongShader.ID);
          }
        }
      }
    } else {
      // Player sent, maybe show a "Waiting..." screen or just black
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }

    // =================================================================================================
    // MINIMAP RENDER PASS
    // =================================================================================================
    if (jogadorPresente) {
      // 1. Clear Depth Buffer so minimap draws on top
      glClear(GL_DEPTH_BUFFER_BIT);

      // 2. Set Viewport to Top-Right corner
      int fbw, fbh;
      glfwGetFramebufferSize(win, &fbw, &fbh);
      int mapSize = fbh / 3; // Minimap size is 1/3 of window height
      glViewport(fbw - mapSize - 10, fbh - mapSize - 10, mapSize,
                 mapSize); // 10px padding

      // 3. Set Orthographic Projection (Top-Down) covering the whole maze
      // (20x20 blocks)
      float mazeWidth = 20.0f * BLOCK_SIZE;
      float mazeHeight = 20.0f * BLOCK_SIZE;
      glm::mat4 projMap = glm::ortho(-50.0f, mazeWidth + 50.0f,
                                     mazeHeight + 50.0f, -50.0f, 0.1f, 1000.0f);

      // 4. Set View Matrix (Looking down from Y axis)
      glm::mat4 viewMap = glm::lookAt(
          glm::vec3(mazeWidth / 2, 500.0f, mazeHeight / 2),
          glm::vec3(mazeWidth / 2, 0.0f, mazeHeight / 2),
          glm::vec3(0.0f, 0.0f,
                    -1.0f)); // Z is up in 2D mapping logic usually, but here we
                             // look down Y, so Z is "down" on screen?
                             // Actually, standard LookAt: Eye, Center, Up.
      // Eye: Center of maze, high up. Center: Center of maze, y=0. Up: -Z (so Z
      // axis points down on screen, X points right)

      phongShader.use();
      // Disable flashlight for minimap (optional, but cleaner)
      phongShader.setBool("flashlightOn", false);

      // Lighting for minimap (Global overhead)
      phongShader.setInt("numLights", 1);
      glm::vec4 lightPosMap =
          viewMap * glm::vec4(mazeWidth / 2, 600.0f, mazeHeight / 2, 1.0f);
      phongShader.setVec4("lights[0].Position", lightPosMap);
      phongShader.setVec3("lights[0].La", 0.5f, 0.5f, 0.5f); // Brighter ambient
      phongShader.setVec3("lights[0].Ld", 0.8f, 0.8f, 0.8f);
      phongShader.setVec3("lights[0].Ls", 0.0f, 0.0f, 0.0f); // No specular

      // Draw Maze Walls
      phongShader.setVec3("Material.Ka", 0.2f, 0.2f, 0.2f);
      phongShader.setVec3("Material.Kd", 0.6f, 0.6f, 0.6f); // Grey walls

      for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 20; col++) {
          if (mapaInicio[row][col] == 1) {
            float x = col * BLOCK_SIZE;
            float z = row * BLOCK_SIZE;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(x + BLOCK_SIZE / 2.0f, 0.0f,
                                                    z + BLOCK_SIZE / 2.0f));
            model = glm::scale(model, glm::vec3(BLOCK_SIZE));

            glm::mat4 modelView = viewMap * model;
            glm::mat4 MVP = projMap * modelView;
            glm::mat3 normalMatrix =
                glm::mat3(glm::transpose(glm::inverse(modelView)));

            phongShader.setMat4("ModelViewMatrix", modelView);
            phongShader.setMat4("MVP", MVP);
            phongShader.setMat3("NormalMatrix", normalMatrix);

            wallMesh->Draw(phongShader.ID);
          }
        }
      }

      // Draw Player Marker (Red Cube)
      phongShader.setVec3("Material.Kd", 1.0f, 0.0f, 0.0f); // Red

      glm::mat4 modelPlayer = glm::mat4(1.0f);
      modelPlayer = glm::translate(
          modelPlayer, glm::vec3(camera.Position.x, 0.0f, camera.Position.z));
      modelPlayer = glm::scale(
          modelPlayer,
          glm::vec3(BLOCK_SIZE * 0.8f)); // Slightly smaller than a block

      glm::mat4 modelViewPlayer = viewMap * modelPlayer;
      glm::mat4 MVPPlayer = projMap * modelViewPlayer;
      glm::mat3 normalMatrixPlayer =
          glm::mat3(glm::transpose(glm::inverse(modelViewPlayer)));

      phongShader.setMat4("ModelViewMatrix", modelViewPlayer);
      phongShader.setMat4("MVP", MVPPlayer);
      phongShader.setMat3("NormalMatrix", normalMatrixPlayer);

      wallMesh->Draw(phongShader.ID);

      // Restore Viewport
      glViewport(0, 0, fbw, fbh);
    }

    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  delete wallMesh;
  glfwTerminate();
  return 0;
}
