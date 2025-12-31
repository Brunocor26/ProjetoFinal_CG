// ============================================================================
// MAZE GAME - Main Game Implementation
// ============================================================================
// This file contains the core game logic including:
// - Game initialization and cleanup
// - Input processing (keyboard, mouse)
// - Network communication (host/client mode)
// - Rendering (3D maze, UI overlays)
// - Collision detection and portal proximity
// ============================================================================

#include "../include/Game.h"
#include "../include/Network.h"
#include "../include/Shader.h"
#include "../include/TextRenderer.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "../include/Texture.h"
#include "../include/stb_image.h"
#include <iostream>
#include "../include/Objloader.hpp"
#include <sys/select.h>

// ============================================================================
// GLOBAL CONSTANTS AND VARIABLES
// ============================================================================

// Network configuration
const int PORT = 8080;          // Server port for host/client communication
const char *HOST = "127.0.0.1"; // Localhost IP for client connection

// Global rendering resources (shared across game instances)
Shader *gameShader; // Main shader program for 3D rendering
Mesh *wall_mesh;    // Mesh for maze walls
Mesh *floor_mesh;   // Mesh for maze floor

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

/**
 * Loads a texture from the specified file path
 * @param path File path to the texture image
 * @return OpenGL texture ID
 */
unsigned int loadTexture(char const *path);

// ============================================================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================================================

/**
 * Game Constructor
 * Initializes all game state variables and resources
 * @param width Window width in pixels
 * @param height Window height in pixels
 * @param gameMode Either HOST or CLIENT mode for networked gameplay
 */
Game::Game(unsigned int width, unsigned int height, GameMode gameMode)
    : Width(width), Height(height), currentMaze(nullptr), camera(nullptr),
      outdoorGroundMesh(nullptr), treeMesh(nullptr), gateMesh(nullptr),
      networkSocket(-1), connectedToPortal(false), portalPosition(0.0f),
      isPaused(false), windowPtr(nullptr), mode(gameMode),
      movementLocked(gameMode == GameMode::CLIENT), serverSocket(-1),
      clientSocket(-1), showingIntroDialog(true), textRenderer(nullptr),
      inheritedColorTint(1.0f, 1.0f, 1.0f), overlayShaderProgram(0),
      overlayVAO(0), overlayVBO(0), overlayResourcesInitialized(false) {
  // Initialize all keyboard keys to unpressed state
  for (int i = 0; i < 1024; i++)
    Keys[i] = false;
}

/**
 * Game Destructor
 * Cleans up all allocated resources including meshes, textures,
 * shaders, and network connections
 */
Game::~Game() {
  // Free game objects
  delete currentMaze;
  delete camera;
  delete gameShader;
  delete wall_mesh;
  delete floor_mesh;
  delete outdoorGroundMesh;
  delete treeMesh;
  delete gateMesh;
  delete textRenderer;

  // Clean up OpenGL overlay resources (VAO, VBO, shader)
  CleanupOverlayResources();

  // Close all network connections gracefully
  if (networkSocket >= 0) {
    Network::closeSocket(networkSocket);
  }
  if (serverSocket >= 0) {
    Network::closeSocket(serverSocket);
  }
  if (clientSocket >= 0) {
    Network::closeSocket(clientSocket);
  }
}

// ============================================================================
// GAME INITIALIZATION
// ============================================================================

/**
 * Initialize Game Resources
 * Sets up network connections, loads all assets (textures, models, shaders),
 * generates the maze, and prepares the game for rendering
 */
void Game::Init() {
  // ---------------------------------------------------------------------------
  // NETWORK SETUP - Initialize based on game mode (HOST or CLIENT)
  // ---------------------------------------------------------------------------

  if (mode == GameMode::HOST) {
    std::cout << "\n===== HOST MODE =====" << std::endl;
    // Create server socket
    serverSocket = Network::createSocket();
    if (serverSocket >= 0) {
      if (Network::bindAndListen(serverSocket, PORT)) { // if successful
        std::cout << "Server listening on port " << PORT << std::endl;
        std::cout << "Waiting for client connection..." << std::endl;
      } else {
        std::cerr << "Failed to start server" << std::endl;
      }
    }
  } else {
    std::cout << "\n===== CLIENT MODE =====" << std::endl;
    std::cout << "MOVEMENT LOCKED - Waiting for host to reach portal"
              << std::endl;
    // Create client socket
    networkSocket = Network::createSocket();
    if (networkSocket >= 0) {
      std::cout << "Connecting to host at " << HOST << ":" << PORT << "..."
                << std::endl;
      if (Network::connectToServer(networkSocket, HOST, PORT)) {
        std::cout << "Connected to host!" << std::endl;
      } else {
        std::cerr << "Failed to connect to host" << std::endl;
      }
    }
  }
  std::cout << "=====================\n" << std::endl;

  // Show intro dialog instructions
  std::cout << "\n╔════════════════════════════════════════════════╗"
            << std::endl;
  std::cout << "║        WELCOME TO THE MAZE GAME!              ║" << std::endl;
  std::cout << "╠════════════════════════════════════════════════╣"
            << std::endl;
  if (mode == GameMode::HOST) {
    std::cout << "║  MODE: HOST                                    ║"
              << std::endl;
    std::cout << "║  GOAL: Reach the portal to unlock the client  ║"
              << std::endl;
  } else {
    std::cout << "║  MODE: CLIENT                                  ║"
              << std::endl;
    std::cout << "║  GOAL: Wait for unlock, then reach portal     ║"
              << std::endl;
  }
  std::cout << "╠════════════════════════════════════════════════╣"
            << std::endl;
  std::cout << "║  CONTROLS:                                     ║"
            << std::endl;
  std::cout << "║  • WASD / Arrow Keys - Move                    ║"
            << std::endl;
  std::cout << "║  • Mouse - Look around                         ║"
            << std::endl;
  std::cout << "║  • ESC - Pause/Resume                          ║"
            << std::endl;
  std::cout << "╠════════════════════════════════════════════════╣"
            << std::endl;
  std::cout << "║  Press ENTER to start the game!               ║" << std::endl;
  std::cout << "╚════════════════════════════════════════════════╝\n"
            << std::endl;

  // 1. Camera - Start higher and looking down to ensure visibility
  camera = new Camera(glm::vec3(15.0f, 20.0f, 15.0f),
                      glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -89.0f);

  // 2. Shaders
  gameShader = new Shader("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/shaders/maze_vs.glsl", "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/shaders/maze_fs.glsl");
  std::cout << "Shader Program ID: " << gameShader->ID << std::endl;
  gameShader->use();
  gameShader->setInt("texture1", 0);

  // 3. Geometry Data
  // Cube (Walls)
  std::vector<Vertex> wallVertices = {
      // Position           // Normal         // TexCoords
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};

  // plane (Floor)
  std::vector<Vertex> floorVertices = {
      {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};

  // Mesh creation moved to after texture loading
  // wall_mesh = new Mesh(wallVertices, {});
  // floor_mesh = new Mesh(floorVertices, {});

  // Load Textures
  // Walls use Bricks101 textures
  unsigned int wallTex = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/"
                                     "Bricks101_4K-PNG_Color.png");
  unsigned int wallNormal = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/"
                                        "Bricks101_4K-PNG_NormalGL.png");
  unsigned int wallRoughness = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/"
                                           "Bricks101_4K-PNG_Roughness.png");

  // Floor uses PavingStones138 textures
  unsigned int floorTex = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/"
                                      "PavingStones138_4K-PNG_Color.png");
  unsigned int floorNormal =
      loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/"
                  "PavingStones138_4K-PNG_NormalGL.png");
  unsigned int floorRoughness =
      loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/"
                  "PavingStones138_4K-PNG_Roughness.png");

  // Create Texture structs for walls (Bricks)
  Texture wallTextureStruct;
  wallTextureStruct.id = wallTex;
  wallTextureStruct.type = "texture_diffuse";
  wallTextureStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/Bricks101_4K-PNG_Color.png";

  Texture wallNormalStruct;
  wallNormalStruct.id = wallNormal;
  wallNormalStruct.type = "texture_normal";
  wallNormalStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/Bricks101_4K-PNG_NormalGL.png";

  Texture wallRoughnessStruct;
  wallRoughnessStruct.id = wallRoughness;
  wallRoughnessStruct.type = "texture_roughness";
  wallRoughnessStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Bricks101_4K-PNG/Bricks101_4K-PNG_Roughness.png";

  // Create Texture structs for floor (PavingStones)
  Texture floorTextureStruct;
  floorTextureStruct.id = floorTex;
  floorTextureStruct.type = "texture_diffuse";
  floorTextureStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/PavingStones138_4K-PNG_Color.png";

  Texture floorNormalStruct;
  floorNormalStruct.id = floorNormal;
  floorNormalStruct.type = "texture_normal";
  floorNormalStruct.path = "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/"
                           "PavingStones138_4K-PNG_NormalGL.png";

  Texture floorRoughnessStruct;
  floorRoughnessStruct.id = floorRoughness;
  floorRoughnessStruct.type = "texture_roughness";
  floorRoughnessStruct.path = "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/PavingStones138_4K-PNG/"
                              "PavingStones138_4K-PNG_Roughness.png";

  std::vector<Texture> wallTextures;
  wallTextures.push_back(wallTextureStruct);
  wallTextures.push_back(wallNormalStruct);
  wallTextures.push_back(wallRoughnessStruct);

  std::vector<Texture> floorTextures;
  floorTextures.push_back(floorTextureStruct);
  floorTextures.push_back(floorNormalStruct);
  floorTextures.push_back(floorRoughnessStruct);

  // 4. Maze (Pass textures to Mesh)
  wall_mesh = new Mesh(wallVertices, {}, wallTextures);
  floor_mesh = new Mesh(floorVertices, {}, floorTextures);

  currentMaze = new Maze(wall_mesh, floor_mesh);
  // currentMaze->wallTexture = wallTex; // REMOVED: Managed by Mesh now
  // currentMaze->floorTexture = floorTex; // REMOVED: Managed by Mesh now

  currentMaze->Generate(15, 15);

  // Find valid start position
  bool foundStart = false;
  for (int z = 0; z < currentMaze->height; z++) {
    for (int x = 0; x < currentMaze->width; x++) {
      if (currentMaze->grid[z][x] == 1) { // 1 is path
        delete camera;
        camera = new Camera(glm::vec3(x * currentMaze->cellSize, 0.5f,
                                      z * currentMaze->cellSize),
                            glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

        std::cout << "Start Position Found at: " << x << ", " << z << std::endl;
        foundStart = true;
        goto end_loop_init; // Break out of nested loop
      }
    }
  }
end_loop_init:
  if (!foundStart) {
    std::cout << "CRITICAL: No path (1) found in maze grid!" << std::endl;
  }

  // 5. Outdoor Environment
  // Create grass texture for outdoor ground
  unsigned int grassTex = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/"
                                      "Grass005_4K-PNG_Color.png");
  unsigned int grassNormal = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/"
                                         "Grass005_4K-PNG_NormalGL.png");
  unsigned int grassRoughness = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/"
                                            "Grass005_4K-PNG_Roughness.png");

  Texture grassTextureStruct;
  grassTextureStruct.id = grassTex;
  grassTextureStruct.type = "texture_diffuse";
  grassTextureStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/Grass005_4K-PNG_Color.png";

  Texture grassNormalStruct;
  grassNormalStruct.id = grassNormal;
  grassNormalStruct.type = "texture_normal";
  grassNormalStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/Grass005_4K-PNG_NormalGL.png";

  Texture grassRoughnessStruct;
  grassRoughnessStruct.id = grassRoughness;
  grassRoughnessStruct.type = "texture_roughness";
  grassRoughnessStruct.path =
      "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/textures/Grass005_4K-PNG/Grass005_4K-PNG_Roughness.png";

  std::vector<Texture> grassTextures;
  grassTextures.push_back(grassTextureStruct);
  grassTextures.push_back(grassNormalStruct);
  grassTextures.push_back(grassRoughnessStruct);

  // Create large outdoor ground plane (extends 10 cells beyond maze)
  float mazeSize = currentMaze->width * currentMaze->cellSize;
  float groundMargin = 10.0f * currentMaze->cellSize; // 10 cells beyond maze
  float groundSize = mazeSize + 2 * groundMargin;
  float halfGroundSize = groundSize / 2.0f;
  float groundCenter = mazeSize / 2.0f;

  std::vector<Vertex> outdoorGroundVertices = {
      {{groundCenter - halfGroundSize, -0.01f, groundCenter - halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {0.0f, groundSize / 5.0f}},
      {{groundCenter + halfGroundSize, -0.01f, groundCenter - halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {groundSize / 5.0f, groundSize / 5.0f}},
      {{groundCenter + halfGroundSize, -0.01f, groundCenter + halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {groundSize / 5.0f, 0.0f}},
      {{groundCenter + halfGroundSize, -0.01f, groundCenter + halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {groundSize / 5.0f, 0.0f}},
      {{groundCenter - halfGroundSize, -0.01f, groundCenter + halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {0.0f, 0.0f}},
      {{groundCenter - halfGroundSize, -0.01f, groundCenter - halfGroundSize},
       {0.0f, 1.0f, 0.0f},
       {0.0f, groundSize / 5.0f}}};

  outdoorGroundMesh = new Mesh(outdoorGroundVertices, {}, grassTextures);

  // 5a. Improved Procedural Tree (Pine Style)
  std::cout << "Generating procedural trees..." << std::endl;
  std::vector<Vertex> treeVertices;
  std::vector<Texture> treeTextures;

  // Load tree texture (Leaves0120_35_S.png) - We keep using this texture!
  unsigned int treeTex = loadTexture("/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/models/Tree1/Leaves0120_35_S.png");
  
  Texture treeTextureStruct;
  treeTextureStruct.id = treeTex;
  treeTextureStruct.type = "texture_diffuse";
  treeTextureStruct.path = "/Users/henriquelaia/maze-cg/ProjetoFinal_CG_Maze/assets/models/Tree1/Leaves0120_35_S.png";
  treeTextures.push_back(treeTextureStruct);

  // --- TRUNK ---
  float trunkH = 2.0f;
  float trunkW = 0.4f;
  // Simple box for trunk
  // Front
  treeVertices.push_back({{-trunkW, 0.0f, -trunkW}, {0,0,-1}, {0,0}});
  treeVertices.push_back({{trunkW, 0.0f, -trunkW}, {0,0,-1}, {1,0}});
  treeVertices.push_back({{trunkW, trunkH, -trunkW}, {0,0,-1}, {1,1}});
  treeVertices.push_back({{trunkW, trunkH, -trunkW}, {0,0,-1}, {1,1}});
  treeVertices.push_back({{-trunkW, trunkH, -trunkW}, {0,0,-1}, {0,1}});
  treeVertices.push_back({{-trunkW, 0.0f, -trunkW}, {0,0,-1}, {0,0}});
  // Back
  treeVertices.push_back({{-trunkW, 0.0f, trunkW}, {0,0,1}, {0,0}});
  treeVertices.push_back({{trunkW, 0.0f, trunkW}, {0,0,1}, {1,0}});
  treeVertices.push_back({{trunkW, trunkH, trunkW}, {0,0,1}, {1,1}});
  treeVertices.push_back({{trunkW, trunkH, trunkW}, {0,0,1}, {1,1}});
  treeVertices.push_back({{-trunkW, trunkH, trunkW}, {0,0,1}, {0,1}});
  treeVertices.push_back({{-trunkW, 0.0f, trunkW}, {0,0,1}, {0,0}});
  // Left
  treeVertices.push_back({{-trunkW, 0.0f, trunkW}, {-1,0,0}, {0,0}});
  treeVertices.push_back({{-trunkW, 0.0f, -trunkW}, {-1,0,0}, {1,0}});
  treeVertices.push_back({{-trunkW, trunkH, -trunkW}, {-1,0,0}, {1,1}});
  treeVertices.push_back({{-trunkW, trunkH, -trunkW}, {-1,0,0}, {1,1}});
  treeVertices.push_back({{-trunkW, trunkH, trunkW}, {-1,0,0}, {0,1}});
  treeVertices.push_back({{-trunkW, 0.0f, trunkW}, {-1,0,0}, {0,0}});
  // Right
  treeVertices.push_back({{trunkW, 0.0f, trunkW}, {1,0,0}, {0,0}});
  treeVertices.push_back({{trunkW, 0.0f, -trunkW}, {1,0,0}, {1,0}});
  treeVertices.push_back({{trunkW, trunkH, -trunkW}, {1,0,0}, {1,1}});
  treeVertices.push_back({{trunkW, trunkH, -trunkW}, {1,0,0}, {1,1}});
  treeVertices.push_back({{trunkW, trunkH, trunkW}, {1,0,0}, {0,1}});
  treeVertices.push_back({{trunkW, 0.0f, trunkW}, {1,0,0}, {0,0}});

  // --- FOLIAGE (3 Stacked Pyramids) ---
  float startY = trunkH * 0.5f;
  float levelH = 2.5f;
  float baseScale = 1.0f;
  
  for(int i=0; i<3; i++) {
      float y = startY + (i * 1.5f);
      float w = 2.0f * baseScale;
      float h = levelH;
      
      glm::vec3 top(0.0f, y + h, 0.0f);
      glm::vec3 c1(-w, y, -w);
      glm::vec3 c2(w, y, -w);
      glm::vec3 c3(w, y, w);
      glm::vec3 c4(-w, y, w);
      
      // Face 1
      treeVertices.push_back({c1, {0,0.5,-1}, {0,0}});
      treeVertices.push_back({c2, {0,0.5,-1}, {1,0}});
      treeVertices.push_back({top, {0,0.5,-1}, {0.5,1}});
      // Face 2
      treeVertices.push_back({c2, {1,0.5,0}, {0,0}});
      treeVertices.push_back({c3, {1,0.5,0}, {1,0}});
      treeVertices.push_back({top, {1,0.5,0}, {0.5,1}});
      // Face 3
      treeVertices.push_back({c3, {0,0.5,1}, {0,0}});
      treeVertices.push_back({c4, {0,0.5,1}, {1,0}});
      treeVertices.push_back({top, {0,0.5,1}, {0.5,1}});
      // Face 4
      treeVertices.push_back({c4, {-1,0.5,0}, {0,0}});
      treeVertices.push_back({c1, {-1,0.5,0}, {1,0}});
      treeVertices.push_back({top, {-1,0.5,0}, {0.5,1}});
      
      baseScale *= 0.7f; // Get smaller as we go up
  }

  treeMesh = new Mesh(treeVertices, {}, treeTextures);

  // Position trees around the perimeter of the outdoor area
  float treeSpacing = 4.0f;

  // Trees along the outer edges
  for (float x = -groundMargin; x < mazeSize + groundMargin; x += treeSpacing) {
    // North edge
    treePositions.push_back(glm::vec3(x, 0.0f, -groundMargin));
    // South edge
    treePositions.push_back(glm::vec3(x, 0.0f, mazeSize + groundMargin));
  }

  for (float z = -groundMargin; z < mazeSize + groundMargin; z += treeSpacing) {
    // West edge
    treePositions.push_back(glm::vec3(-groundMargin, 0.0f, z));
    // East edge
    treePositions.push_back(glm::vec3(mazeSize + groundMargin, 0.0f, z));
  }

  std::cout << "Outdoor environment created with " << treePositions.size()
            << " trees" << std::endl;

  // 6. Generate Procedural Sphere for Portal (Silver Sphere)
  std::cout << "Generating procedural sphere for portal..." << std::endl;
  std::vector<Vertex> portalFinalVertices;
  
  // Sphere parameters
  float radius = 1.0f;
  int sectorCount = 36;
  int stackCount = 18;
  float PI = 3.14159265359f;
  
  for(int i = 0; i <= stackCount; ++i)
  {
      float stackAngle = PI / 2 - i * PI / stackCount;        // starting from pi/2 to -pi/2
      float xy = radius * cosf(stackAngle);             // r * cos(u)
      float z = radius * sinf(stackAngle);              // r * sin(u)
  
      for(int j = 0; j <= sectorCount; ++j)
      {
          float sectorAngle = j * 2 * PI / sectorCount;           // starting from 0 to 2pi
  
          // Vertex position
          float x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
          float y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
          glm::vec3 pos(x, y, z);
          
          // Vertex normal (normalized position for sphere)
          glm::vec3 norm = glm::normalize(pos);
          
          // Vertex texCoord
          float u = (float)j / sectorCount;
          float v = (float)i / stackCount;
          glm::vec2 uv(u, v);
          
          portalFinalVertices.push_back({pos, norm, uv});
      }
  }

  // Reuse wallTextures just to have valid material structure, though we won't use the texture image
  gateMesh = new Mesh(portalFinalVertices, {}, wallTextures);
  // Mesh class handles EBO generation automatically if indices not provided? 
  // Wait, Mesh class usually expects triangles. 
  // Our Mesh class implementation takes vertices and draws them as GL_TRIANGLES.
  // The above loop generates points. We need indices or a different generation strategy.
  // ACTUALLY: The existing Mesh class might take raw vertices if drawn as GL_TRIANGLES, but we need to order them correctly.
  
  // CORRECTION: Let's regenerate using TRIANGLE STRIP logic or generate triangle indices.
  // Since Mesh constructor takes Vertex vector and Index vector...
  // Let's generate INDICES for the sphere.
  
  std::vector<unsigned int> sphereIndices;
  for(int i = 0; i < stackCount; ++i)
  {
      int k1 = i * (sectorCount + 1);     // beginning of current stack
      int k2 = k1 + sectorCount + 1;      // beginning of next stack
  
      for(int j = 0; j < sectorCount; ++j, ++k1, ++k2)
      {
          // 2 triangles per sector excluding first and last stacks
          // k1 => k2 => k1+1
          if(i != 0)
          {
              sphereIndices.push_back(k1);
              sphereIndices.push_back(k2);
              sphereIndices.push_back(k1 + 1);
          }
  
          // k1+1 => k2 => k2+1
          if(i != (stackCount-1))
          {
              sphereIndices.push_back(k1 + 1);
              sphereIndices.push_back(k2);
              sphereIndices.push_back(k2 + 1);
          }
      }
  }
  
  // Re-create mesh WITH indices
  gateMesh = new Mesh(portalFinalVertices, sphereIndices, wallTextures);
  std::cout << "Sphere Portal Mesh initialized with " << portalFinalVertices.size() << " vertices and " << sphereIndices.size() << " indices." << std::endl;
  std::cout << "Portal Mesh initialized." << std::endl;

  // Store portal position for proximity detection
  portalPosition =
      glm::vec3(currentMaze->endParams.x * currentMaze->cellSize, 0.0f,
                currentMaze->endParams.y * currentMaze->cellSize);

  // Initialize text renderer for intro dialog
  textRenderer = new TextRenderer(Width, Height);
  // Load font (TODO: adjust path)
  textRenderer->Load("/System/Library/Fonts/Helvetica.ttc", 24);
  std::cout << "Text renderer initialized!" << std::endl;
}

/**
 * Load Texture Helper Function
 * Loads an image file and creates an OpenGL texture with mipmaps
 * @param path File path to the texture image
 * @return OpenGL texture ID
 */
unsigned int loadTexture(char const *path) {
  unsigned int textureID;
  glGenTextures(1, &textureID);

  // Flip texture vertically to match OpenGL's coordinate system
  stbi_set_flip_vertically_on_load(true);

  // Load image data
  int width, height, nrComponents;
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);

  if (data) {
    std::cout << "Texture Loaded! Path: " << path << " | W: " << width
              << " H: " << height << " Ch: " << nrComponents << std::endl;

    // Determine texture format based on number of color channels
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED; // Grayscale
    else if (nrComponents == 3)
      format = GL_RGB; // RGB
    else if (nrComponents == 4)
      format = GL_RGBA; // RGBA (with alpha)

    // Upload texture to GPU
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(
        GL_TEXTURE_2D); // Generate mipmap levels for better quality

    // Set texture wrapping and filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR); // Trilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}

/**
 * Collision Detection Helper
 * Checks if a target position collides with maze walls
 * @param targetPos The 3D position to check
 * @param maze Pointer to the maze object
 * @return true if collision detected, false otherwise
 */
bool CheckCollision(glm::vec3 targetPos, Maze *maze) {
  if (!maze)
    return false;
    
  // Improved Collision Detection: Check a radius around the player
  // This prevents the camera near-plane (0.1) from clipping through walls
  // UPDATE: Increased radius and added diagonal checks to fix corner clipping
  float playerRadius = 0.35f; 
  
  // Check center
  if (maze->IsWall(targetPos.x, targetPos.z)) return true;
  
  // Check 4 cardinal points
  if (maze->IsWall(targetPos.x + playerRadius, targetPos.z)) return true;
  if (maze->IsWall(targetPos.x - playerRadius, targetPos.z)) return true;
  if (maze->IsWall(targetPos.x, targetPos.z + playerRadius)) return true;
  if (maze->IsWall(targetPos.x, targetPos.z - playerRadius)) return true;

  // Check 4 diagonal corners (Square bounding box)
  // This is crucial for corners where cardinal checks might pass but camera corners clip
  if (maze->IsWall(targetPos.x + playerRadius, targetPos.z + playerRadius)) return true;
  if (maze->IsWall(targetPos.x + playerRadius, targetPos.z - playerRadius)) return true;
  if (maze->IsWall(targetPos.x - playerRadius, targetPos.z + playerRadius)) return true;
  if (maze->IsWall(targetPos.x - playerRadius, targetPos.z - playerRadius)) return true;
  
  return false;
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

/**
 * Process Keyboard Input
 * Handles all keyboard inputs including dialog interactions, pause/resume,
 * fullscreen toggle, and player movement
 * @param dt Delta time since last frame (for frame-independent movement)
 */
void Game::ProcessInput(float dt) {
  // ---------------------------------------------------------------------------
  // INTRO DIALOG HANDLING
  // ---------------------------------------------------------------------------

  // Track ENTER key state to detect single press (not hold)
  static bool enterPressedLastFrame = false;
  bool enterPressed = Keys[GLFW_KEY_ENTER] || Keys[GLFW_KEY_KP_ENTER];

  if (showingIntroDialog) {
    // Dismiss intro dialog when ENTER is pressed
    if (enterPressed && !enterPressedLastFrame) {
      showingIntroDialog = false;
      std::cout << "\n=== Game Started! ===" << std::endl;
    }
    enterPressedLastFrame = enterPressed;
    return; // Block all other input while dialog is shown
  }
  enterPressedLastFrame = enterPressed;

  // ---------------------------------------------------------------------------
  // PAUSE/UNPAUSE HANDLING (ESC key)
  // ---------------------------------------------------------------------------
  static bool escPressedLastFrame = false;
  bool escPressed = Keys[GLFW_KEY_ESCAPE];

  if (escPressed && !escPressedLastFrame && windowPtr) {
    // Toggle pause
    isPaused = !isPaused;

    if (isPaused) {
      // Show cursor
      glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      std::cout << "Game PAUSED. Press ESC to resume." << std::endl;
    } else {
      // Hide cursor
      glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      std::cout << "Game RESUMED." << std::endl;
    }
  }
  escPressedLastFrame = escPressed;

  // ---------------------------------------------------------------------------
  // FULLSCREEN TOGGLE (F key)
  // ---------------------------------------------------------------------------
  static bool fPressedLastFrame = false;
  bool fPressed = Keys[GLFW_KEY_F];

  if (fPressed && !fPressedLastFrame && windowPtr) {
    GLFWmonitor *monitor = glfwGetWindowMonitor(windowPtr);

    if (monitor == nullptr) {
      // Currently in windowed mode - switch to fullscreen
      GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
      const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);
      glfwSetWindowMonitor(windowPtr, primaryMonitor, 0, 0, mode->width,
                           mode->height, mode->refreshRate);
      std::cout << "Switched to FULLSCREEN mode" << std::endl;
    } else {
      // Currently in fullscreen - switch to windowed mode
      glfwSetWindowMonitor(windowPtr, nullptr, 100, 100, Width, Height, 0);
      std::cout << "Switched to WINDOWED mode" << std::endl;
    }
  }
  fPressedLastFrame = fPressed;

  // ---------------------------------------------------------------------------
  // MOVEMENT RESTRICTIONS
  // ---------------------------------------------------------------------------

  // Don't process movement if game is paused
  if (isPaused) {
    return;
  }

  // CLIENT MODE: Block movement until host reaches portal and sends unlock
  if (mode == GameMode::CLIENT && movementLocked) {
    return;
  }

  // ---------------------------------------------------------------------------
  // PLAYER MOVEMENT (WASD/Arrow Keys)
  // ---------------------------------------------------------------------------

  // Get current player position and calculate movement velocity
  glm::vec3 currentPos = camera->Position;
  glm::vec3 nextPos = currentPos;
  float velocity = camera->MovementSpeed * dt; // Frame-independent movement

  // Calculate movement directions (lock to horizontal plane - no flying!)
  glm::vec3 front = camera->Front;
  front.y = 0.0f; // Remove vertical  component
  front = glm::normalize(front);
  glm::vec3 right = camera->Right;
  right.y = 0.0f; // Remove vertical component
  right = glm::normalize(right);

  // Accumulate movement from all pressed keys
  glm::vec3 proposedMove = glm::vec3(0.0f);

  // W or UP Arrow: Move forward
  if (Keys[GLFW_KEY_W] || Keys[GLFW_KEY_UP])
    proposedMove += front * velocity;
  // S or DOWN Arrow: Move backward
  if (Keys[GLFW_KEY_S] || Keys[GLFW_KEY_DOWN])
    proposedMove -= front * velocity;
  // A or LEFT Arrow: Strafe left
  if (Keys[GLFW_KEY_A] || Keys[GLFW_KEY_LEFT])
    proposedMove -= right * velocity;
  // D or RIGHT Arrow: Strafe right
  if (Keys[GLFW_KEY_D] || Keys[GLFW_KEY_RIGHT])
    proposedMove += right * velocity;

  // Apply movement with collision detection (X and Z axes independently)
  // This allows "sliding" along walls instead of getting stuck

  // Try to move on X axis
  if (!CheckCollision(
          glm::vec3(currentPos.x + proposedMove.x, currentPos.y, currentPos.z),
          currentMaze)) {
    camera->Position.x += proposedMove.x;
  }

  // Try to move on Z axis
  if (!CheckCollision(glm::vec3(camera->Position.x, currentPos.y,
                                currentPos.z + proposedMove.z),
                      currentMaze)) {
    camera->Position.z += proposedMove.z;
  }

  // Keep player at constant height (prevent floating or sinking)
  camera->Position.y = 0.5f;
}

/**
 * Process Mouse Movement
 * Updates camera orientation based on mouse movement (look around)
 * @param xoffset Horizontal mouse movement
 * @param yoffset Vertical mouse movement
 * @param constrainPitch Prevent camera from flipping upside down
 */
void Game::ProcessMouseMovement(float xoffset, float yoffset,
                                bool constrainPitch) {
  // Ignore mouse input when game is paused
  if (isPaused) {
    return;
  }
  // Delegate to camera's built-in mouse movement handler
  camera->ProcessMouseMovement(xoffset, yoffset, constrainPitch);
}

// ============================================================================
// GAME UPDATE (Network & Game Logic)
// ============================================================================

/**
 * Update Game State
 * Handles network communication (accept connections, receive unlock messages)
 * and checks portal proximity for win conditions
 * @param dt Delta time since last frame
 */
void Game::Update(float dt) {
  // ---------------------------------------------------------------------------
  // HOST MODE: Accept incoming client connections (non-blocking)
  // ---------------------------------------------------------------------------
  if (mode == GameMode::HOST && serverSocket >= 0 && clientSocket < 0) {
    // Non-blocking accept check
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(serverSocket, &readfds);
    struct timeval tv = {0, 0}; // No wait

    if (select(serverSocket + 1, &readfds, NULL, NULL, &tv) > 0) {
      clientSocket = Network::acceptConnection(serverSocket);
      if (clientSocket >= 0) {
        std::cout << "✓ Client connected!" << std::endl;
      }
    }
  }

  // CLIENT MODE: Check for unlock message
  if (mode == GameMode::CLIENT && movementLocked && networkSocket >= 0) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(networkSocket, &readfds);
    struct timeval tv = {0, 0}; // Non-blocking

    if (select(networkSocket + 1, &readfds, NULL, NULL, &tv) > 0) {
      char buffer[256];
      ssize_t bytesRead =
          Network::receiveData(networkSocket, buffer, sizeof(buffer) - 1);
      if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string message(buffer);

        if (message.find("UNLOCK") != std::string::npos) {
          movementLocked = false;

          // Parse color tint from message (format: "UNLOCK r g b")
          float r, g, b;
          if (sscanf(buffer, "UNLOCK %f %f %f", &r, &g, &b) == 3) {
            inheritedColorTint = glm::vec3(r, g, b);
            std::cout << "\n*** UNLOCKED with inherited color tint! ***"
                      << std::endl;
          } else {
            std::cout << "\n*** UNLOCKED! You can now move! ***" << std::endl;
          }
          std::cout << "Navigate to the portal to win!\n" << std::endl;
        }
      }
    }
  }

  // Check if player is near portal
  CheckPortalProximity();
}

void Game::Render() {
  gameShader->use();
  gameShader->setBool("isPortal", false); // Default to standard rendering

  // Configure flashlight (follows camera)
  gameShader->setVec3("light.position", camera->Position.x, camera->Position.y,
                      camera->Position.z);
  gameShader->setVec3("light.direction", camera->Front.x, camera->Front.y,
                      camera->Front.z);
  gameShader->setVec3("viewPos", camera->Position.x, camera->Position.y,
                      camera->Position.z);

  // Configure spotlight cone angles (cosine of angle)
  gameShader->setFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
  gameShader->setFloat("light.outerCutOff", glm::cos(glm::radians(17.5f)));

  // Light colors
  gameShader->setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
  gameShader->setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
  gameShader->setVec3("light.specular", 1.0f, 1.0f, 1.0f);

  // Attenuation (values for ~50 meters coverage)
  gameShader->setFloat("light.constant", 1.0f);
  gameShader->setFloat("light.linear", 0.09f);
  gameShader->setFloat("light.quadratic", 0.032f);

  // View/Projection matrices
  glm::mat4 projection = glm::perspective(
      glm::radians(camera->Zoom), (float)Width / (float)Height, 0.1f, 100.0f);
  glm::mat4 view = camera->GetViewMatrix();

  gameShader->setMat4("projection", glm::value_ptr(projection));
  gameShader->setMat4("view", glm::value_ptr(view));

  // Calculate and set environment tint based on portal proximity
  glm::vec3 envTint = GetEnvironmentTint();
  gameShader->setVec3("environmentTint", envTint.x, envTint.y, envTint.z);

  // Render outdoor ground first (underneath everything)
  if (outdoorGroundMesh) {
    glm::mat4 groundModel = glm::mat4(1.0f);
    gameShader->setMat4("model", glm::value_ptr(groundModel));
    gameShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
    gameShader->setBool("useTexture", true);
    outdoorGroundMesh->Draw(gameShader->ID);
  }

  // Render maze
  if (currentMaze) {
    currentMaze->Draw(*gameShader);
  }

  // Render trees around the perimeter
  if (treeMesh && treePositions.size() > 0) {
    gameShader->setBool("useTexture", true);

    for (const glm::vec3 &treePos : treePositions) {
      glm::mat4 treeModel = glm::mat4(1.0f);
      treeModel = glm::translate(treeModel, treePos);
      
      gameShader->setMat4("model", glm::value_ptr(treeModel));

      // Use white color to allow texture to show properly
      gameShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
      treeMesh->Draw(gameShader->ID);
    }
  }

  // Render portal environment at maze end
  // Only render if close enough (e.g., < 50 units) - almost always visible
  bool renderPortal = false;
  if (gateMesh && currentMaze) {
      glm::vec3 gatePos(currentMaze->endParams.x * currentMaze->cellSize, 0.0f,
                        currentMaze->endParams.y * currentMaze->cellSize);
      float distToPortal = glm::distance(camera->Position, gatePos);
      
      if (distToPortal < 50.0f) { // Always visible when in corridor
          renderPortal = true;
          
          gameShader->setBool("useTexture", false);
          
          // Enable Special "Liquid Silver" Shader Effect
          gameShader->setBool("isPortal", true); // Use custom shader logic
          gameShader->setFloat("time", (float)glfwGetTime());
          
          // Force environment tint to WHITE for the portal so it looks silver, not purple
          gameShader->setVec3("environmentTint", 1.0f, 1.0f, 1.0f);
          
          // Animation Variables
          float time = (float)glfwGetTime();
          float rotationSpeed = 2.0f; // Fast spin
          
          // 1. Position (Centered in cell, floating lightly or grounded)
          // Sphere radius 0.4 * 0.5 scale = 0.2 actual radius.
          // Center at 0.2 height makes it sit EXACTLY on the floor
          glm::vec3 animatedGatePos = gatePos;
          animatedGatePos.y = 0.2f; 
          
          glm::mat4 gateModel = glm::mat4(1.0f);
          gateModel = glm::translate(gateModel, animatedGatePos);
          
          // 2. Rotation (Spinning Silver Sphere)
          gateModel = glm::rotate(gateModel, time * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
          
          // 3. Scale (Radius 0.2 - Compact)
          gateModel = glm::scale(gateModel, glm::vec3(0.2f, 0.2f, 0.2f)); 
        
          gameShader->setMat4("model", glm::value_ptr(gateModel));
          
          // Shader handles color mixing, but we pass white base just in case
          gameShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f); 
          
          gateMesh->Draw(gameShader->ID);
          
          // Reset flags & environment settings
          gameShader->setBool("isPortal", false);
          gameShader->setVec3("environmentTint", envTint.x, envTint.y, envTint.z);
      }
  }

  // Render intro dialog overlay if needed
  if (showingIntroDialog) {
    RenderIntroDialog();
  } else if (isPaused) {
      RenderPauseOverlay();
  }
}

void Game::RenderPauseOverlay() {
  if (!textRenderer) return;

  // Initialize overlay resources if needed
  if (!overlayResourcesInitialized) {
    InitializeOverlayResources();
  }

  // Save OpenGL state
  GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
  GLboolean blendEnabled = glIsEnabled(GL_BLEND);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use shader and render background quad
  glUseProgram(overlayShaderProgram);
  glBindVertexArray(overlayVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Render centered "PAUSED" text
  std::string pausedText = "PAUSED";
  float scale = 2.0f;
  float textWidth = textRenderer->CalculateTextWidth(pausedText, scale);
  
  float centerX = (Width - textWidth) / 2.0f;
  float centerY = Height / 2.0f;

  textRenderer->RenderText(pausedText, centerX, centerY, scale, glm::vec3(1.0f, 1.0f, 1.0f));

  // Instructions
  std::string subText = "Press ESC to Resume";
  float subScale = 1.0f;
  float subWidth = textRenderer->CalculateTextWidth(subText, subScale);
  
  textRenderer->RenderText(subText, (Width - subWidth) / 2.0f, centerY - 50.0f, subScale, glm::vec3(0.8f, 0.8f, 0.8f));

  // Restore state
  if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
  if (!blendEnabled) glDisable(GL_BLEND);
}

void Game::CheckPortalProximity() {
  if (!camera || connectedToPortal) {
    return; // Already connected or no camera
  }

  // Calculate distance to portal
  float distance = glm::distance(camera->Position, portalPosition);
  float proximityThreshold = 2.0f; // Trigger when within 2 units

  if (distance < proximityThreshold) {
    if (mode == GameMode::HOST) {
      // HOST MODE: Send unlock message to client
      std::cout << "\n=== PORTAL REACHED ===" << std::endl;
      std::cout << "You have reached the portal!" << std::endl;

      // Pause game and show cursor
      isPaused = true;
      if (windowPtr) {
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }

      std::cout << "Game PAUSED at portal." << std::endl;
      std::cout << "Press ESC to resume and continue exploring." << std::endl;

      // Send unlock message to client with current color tint
      if (clientSocket >= 0) {
        glm::vec3 currentTint = GetEnvironmentTint();
        char unlockMsg[128];
        snprintf(unlockMsg, sizeof(unlockMsg), "UNLOCK %.3f %.3f %.3f",
                 currentTint.x, currentTint.y, currentTint.z);
        Network::sendData(clientSocket, unlockMsg, strlen(unlockMsg));
        std::cout << "Sent UNLOCK signal with color tint to client!"
                  << std::endl;
      } else {
        std::cout << "No client connected to unlock." << std::endl;
      }

      connectedToPortal = true;
    } else {
      // CLIENT MODE: Victory message
      std::cout << "\n=== YOU WIN! ===" << std::endl;
      std::cout << "Congratulations! You reached the portal!" << std::endl;

      // Pause game
      isPaused = true;
      if (windowPtr) {
        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }

      std::cout << "Press ESC to resume or close the window." << std::endl;
      connectedToPortal = true;
    }
  }
}

glm::vec3 Game::GetEnvironmentTint() {
  // Calculate distance from camera to portal
  float distance = glm::length(camera->Position - portalPosition);

  // Define maximum distance (full maze width)
  float maxDistance = currentMaze->width * currentMaze->cellSize;

  // Calculate normalized distance (0 = at portal, 1 = far from portal)
  float t = glm::clamp(1.0f - (distance / maxDistance), 0.0f, 1.0f);

  // Apply smoothstep for smooth interpolation
  t = t * t * (3.0f - 2.0f * t);

  // Define colors
  glm::vec3 normalColor(1.0f, 1.0f, 1.0f); // White/neutral
  glm::vec3 portalColor(0.4f, 0.2f,
                        1.0f); // Intense purple/mystical (more dramatic)

  // For client mode, start from inherited color instead of white
  if (mode == GameMode::CLIENT) {
    normalColor = inheritedColorTint;
  }

  // Interpolate between colors
  return glm::mix(normalColor, portalColor, t);
}

void Game::RenderIntroDialog() {
  if (!textRenderer) {
    return; // Safety check
  }

  // Initialize overlay resources once
  if (!overlayResourcesInitialized) {
    InitializeOverlayResources();
  }

  // Save current OpenGL state
  GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
  GLboolean blendEnabled = glIsEnabled(GL_BLEND);

  // Disable depth test and enable blending for 2D overlay
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Use shader and render the overlay quad
  glUseProgram(overlayShaderProgram);
  glBindVertexArray(overlayVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Now render text on top of the overlay
  float centerX = Width / 2.0f;

  // Title
  std::string title = "Maze: Escape from yourself";
  float titleScale = 1.5f;
  float titleWidth = textRenderer->CalculateTextWidth(title, titleScale);
  textRenderer->RenderText(title, (Width - titleWidth) / 2.0f,
                           Height - 100.0f, titleScale, glm::vec3(1.0f, 1.0f, 1.0f));

  // Mode
  std::string modeText =
      (mode == GameMode::HOST) ? "MODE: HOST" : "MODE: CLIENT";
  float modeScale = 1.0f;
  float modeWidth = textRenderer->CalculateTextWidth(modeText, modeScale);
  textRenderer->RenderText(modeText, (Width - modeWidth) / 2.0f, Height - 150.0f, modeScale,
                           glm::vec3(0.8f, 0.8f, 1.0f));

  // Objective
  std::string objectiveText;
  if (mode == GameMode::HOST) {
    objectiveText = "'No man ever steps in the same river twice, for it's not "
                    "the same river and he's not the same man.' - Heraclitus";
  } else {
    objectiveText = "Do you still feel like you are the same man? Is this "
                    "still the same river?";
  }
  float objScale = 0.6f; // Smaller to fit
  float objWidth = textRenderer->CalculateTextWidth(objectiveText, objScale);
  textRenderer->RenderText(objectiveText, (Width - objWidth) / 2.0f, Height - 200.0f,
                           objScale, glm::vec3(0.9f, 0.9f, 0.9f));

  // Controls
  std::string controlsTitle = "CONTROLS:";
  float controlsTitleWidth = textRenderer->CalculateTextWidth(controlsTitle, 1.0f);
  textRenderer->RenderText(controlsTitle, (Width - controlsTitleWidth) / 2.0f, Height - 280.0f, 1.0f,
                           glm::vec3(1.0f, 0.9f, 0.5f));

  std::string c1 = "WASD / Arrow Keys - Move";
  float c1Width = textRenderer->CalculateTextWidth(c1, 0.7f);
  textRenderer->RenderText(c1, (Width - c1Width) / 2.0f,
                           Height - 320.0f, 0.7f, glm::vec3(0.9f, 0.9f, 0.9f));

  std::string c2 = "Mouse - Look Around";
  float c2Width = textRenderer->CalculateTextWidth(c2, 0.7f);
  textRenderer->RenderText(c2, (Width - c2Width) / 2.0f,
                           Height - 350.0f, 0.7f, glm::vec3(0.9f, 0.9f, 0.9f));

  std::string c3 = "ESC - Pause/Resume";
  float c3Width = textRenderer->CalculateTextWidth(c3, 0.7f);
  textRenderer->RenderText(c3, (Width - c3Width) / 2.0f,
                           Height - 380.0f, 0.7f, glm::vec3(0.9f, 0.9f, 0.9f));

  // Instruction to start
  std::string startText = "Press ENTER to Start";
  float startWidth = textRenderer->CalculateTextWidth(startText, 1.0f);
  textRenderer->RenderText(startText, (Width - startWidth) / 2.0f, 100.0f,
                           1.0f, glm::vec3(0.5f, 1.0f, 0.5f));

  // Restore previous OpenGL state
  if (depthTestEnabled)
    glEnable(GL_DEPTH_TEST);
  if (!blendEnabled)
    glDisable(GL_BLEND);
}

void Game::InitializeOverlayResources() {
  // Simple 2D quad vertices (fullscreen overlay in NDC)
  float overlayVertices[] = {// positions (NDC: -1 to 1)
                             -1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,
                             1.0f,  1.0f,  0.0f, 1.0f,  1.0f,  0.0f,
                             -1.0f, 1.0f,  0.0f, -1.0f, -1.0f, 0.0f};

  // Create VAO, VBO for the overlay (these will be reused)
  glGenVertexArrays(1, &overlayVAO);
  glGenBuffers(1, &overlayVBO);

  glBindVertexArray(overlayVAO);
  glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVertices), overlayVertices,
               GL_STATIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Unbind
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Create simple shader for 2D rendering
  // Vertex shader - pass through NDC coordinates
  const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
      gl_Position = vec4(aPos, 1.0);
    }
  )";

  // Fragment shader - semi-transparent dark overlay
  const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
      FragColor = vec4(0.0, 0.0, 0.0, 0.8); // Dark semi-transparent
    }
  )";

  // Compile vertex shader
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // Check for shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cerr << "ERROR: Overlay Vertex Shader Compilation Failed\n"
              << infoLog << std::endl;
  }

  // Compile fragment shader
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cerr << "ERROR: Overlay Fragment Shader Compilation Failed\n"
              << infoLog << std::endl;
  }

  // Link shaders into program
  overlayShaderProgram = glCreateProgram();
  glAttachShader(overlayShaderProgram, vertexShader);
  glAttachShader(overlayShaderProgram, fragmentShader);
  glLinkProgram(overlayShaderProgram);

  glGetProgramiv(overlayShaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(overlayShaderProgram, 512, NULL, infoLog);
    std::cerr << "ERROR: Overlay Shader Program Linking Failed\n"
              << infoLog << std::endl;
  }

  // Clean up shaders (they're linked into the program now)
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  overlayResourcesInitialized = true;
  std::cout << "Overlay resources initialized successfully!" << std::endl;
}

void Game::CleanupOverlayResources() {
  if (overlayResourcesInitialized) {
    if (overlayVAO != 0) {
      glDeleteVertexArrays(1, &overlayVAO);
      overlayVAO = 0;
    }
    if (overlayVBO != 0) {
      glDeleteBuffers(1, &overlayVBO);
      overlayVBO = 0;
    }
    if (overlayShaderProgram != 0) {
      glDeleteProgram(overlayShaderProgram);
      overlayShaderProgram = 0;
    }
    overlayResourcesInitialized = false;
  }
}
