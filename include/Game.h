/**
 * @file Game.h
 * @brief Declaração da classe Game - núcleo do jogo de labirinto 3D
 * @author Projeto CG - Maze Game
 * @date 2025
 */

#ifndef GAME_H
#define GAME_H

#include "../include/learnopengl/camera.h"
#include "Maze.h"

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

/// Forward declaration da janela GLFW
struct GLFWwindow;

// ============================================================================
// ENUMS
// ============================================================================

/**
 * @brief Modos de jogo disponíveis
 *
 * HOST: Servidor que desbloqueia o cliente ao chegar ao portal
 * CLIENT: Cliente que fica bloqueado até o HOST alcançar o portal
 */
enum class GameMode {
  HOST,  ///< Modo servidor/anfitrião
  CLIENT ///< Modo cliente
};

// ============================================================================
// CLASSE GAME
// ============================================================================

/**
 * @brief Classe principal que gere todo o jogo de labirinto
 *
 * Esta classe é responsável por:
 * - Gestão de recursos (shaders, texturas, meshes)
 * - Processamento de input (teclado, rato)
 * - Lógica de jogo (colisões, proximidade ao portal)
 * - Comunicação de rede (modo HOST/CLIENT)
 * - Renderização (labirinto, ambiente exterior, UI)
 *
 * O jogo suporta dois modos:
 * - HOST: Cria servidor e desbloqueia cliente ao chegar ao portal
 * - CLIENT: Conecta ao servidor e aguarda desbloqueio
 */
class Game {
public:
  // ========================================================================
  // ESTADO DO JOGO
  // ========================================================================

  /// Array de estado das teclas (true = pressionada)
  bool Keys[1024];

  /// Largura da janela em pixels
  unsigned int Width;

  /// Altura da janela em pixels
  unsigned int Height;

  // ========================================================================
  // NETWORKING
  // ========================================================================

  /// Modo de jogo atual (HOST ou CLIENT)
  GameMode mode;

  /// Indica se movimento está bloqueado (apenas CLIENT)
  bool movementLocked;

  /// Socket do servidor (apenas HOST - para aceitar conexões)
  int serverSocket;

  /// Socket do cliente conectado (apenas HOST)
  int clientSocket;

  /// Socket de rede (apenas CLIENT - conexão ao servidor)
  int networkSocket;

  /// Indica se já alcançou o portal
  bool connectedToPortal;

  /// Posição 3D do portal no labirinto
  glm::vec3 portalPosition;

  /// Tint de cor herdado do HOST (apenas CLIENT)
  glm::vec3 inheritedColorTint;

  // ========================================================================
  // RECURSOS DO JOGO
  // ========================================================================

  /// Ponteiro para o labirinto atual
  Maze *currentMaze;

  /// Câmara do jogador (primeira pessoa)
  Camera *camera;

  /// Mesh do chão exterior (relva)
  Mesh *outdoorGroundMesh;

  /// Mesh das árvores
  Mesh *treeMesh;

  /// Posições de todas as árvores no cenário
  std::vector<glm::vec3> treePositions;

  /// Mesh do portal no final do labirinto
  Mesh *gateMesh;

  /// Sistema de renderização de texto
  class TextRenderer *textRenderer;

  // ========================================================================
  // ESTADO DA UI
  // ========================================================================

  /// Indica se está a mostrar o diálogo inicial
  bool showingIntroDialog;

  /// Indica se o jogo está pausado
  bool isPaused;

  /// Ponteiro para a janela (para controlo do cursor)
  GLFWwindow *windowPtr;

  // ========================================================================
  // CONSTRUTOR E DESTRUTOR
  // ========================================================================

  /**
   * @brief Construtor do jogo
   * @param width Largura da janela em pixels
   * @param height Altura da janela em pixels
   * @param gameMode Modo de jogo (HOST ou CLIENT), padrão é HOST
   */
  Game(unsigned int width, unsigned int height,
       GameMode gameMode = GameMode::HOST);

  /**
   * @brief Destrutor - liberta todos os recursos alocados
   */
  ~Game();

  // ========================================================================
  // CICLO DE VIDA DO JOGO
  // ========================================================================

  /**
   * @brief Inicializa todos os recursos do jogo
   *
   * Carrega shaders, modelos 3D, texturas, gera o labirinto,
   * inicializa a rede e prepara o jogo para renderização.
   */
  void Init();

  /**
   * @brief Processa input do teclado
   *
   * Trata teclas para movimento (WASD/setas), pausa (ESC),
   * fullscreen (F) e interação com diálogos (ENTER).
   *
   * @param dt Delta time para movimento independente de framerate
   */
  void ProcessInput(float dt);

  /**
   * @brief Processa movimento do rato
   *
   * Atualiza a orientação da câmara baseado no movimento do rato.
   *
   * @param xoffset Deslocamento horizontal do rato
   * @param yoffset Deslocamento vertical do rato
   * @param constrainPitch Limita rotação vertical (evita flip)
   */
  void ProcessMouseMovement(float xoffset, float yoffset,
                            bool constrainPitch = true);

  /**
   * @brief Atualiza lógica do jogo
   *
   * Trata comunicação de rede (aceitar conexões, receber mensagens)
   * e verifica proximidade ao portal.
   *
   * @param dt Delta time desde o último frame
   */
  void Update(float dt);

  /**
   * @brief Renderiza toda a cena
   *
   * Desenha o labirinto, ambiente exterior, portal e UI overlays.
   */
  void Render();

  // ========================================================================
  // MÉTODOS AUXILIARES
  // ========================================================================

  /**
   * @brief Verifica se o jogador está perto do portal
   *
   * Quando o HOST chega ao portal, envia mensagem de desbloqueio
   * ao CLIENT e mostra diálogo de vitória.
   */
  void CheckPortalProximity();

  /**
   * @brief Calcula tint de cor baseado na proximidade ao portal
   *
   * Quanto mais perto do portal, mais a cor ambiente muda para
   * tons de azul/ciano, criando efeito visual progressivo.
   *
   * @return Vetor RGB com o tint calculado
   */
  glm::vec3 GetEnvironmentTint();

  /**
   * @brief Renderiza o diálogo de introdução
   *
   * Mostra overlay semi-transparente com informações de jogo,
   * controlos e botão para iniciar.
   */
  void RenderIntroDialog();
  void RenderPauseOverlay();

private:
  // ========================================================================
  // RECURSOS PRIVADOS (OVERLAY)
  // ========================================================================

  /// Shader program para overlay 2D do diálogo
  unsigned int overlayShaderProgram;

  /// VAO (Vertex Array Object) do overlay
  unsigned int overlayVAO;

  /// VBO (Vertex Buffer Object) do overlay
  unsigned int overlayVBO;

  /// Flag indicando se recursos do overlay foram inicializados
  bool overlayResourcesInitialized;

  /**
   * @brief Inicializa recursos OpenGL para o overlay
   *
   * Cria shader, VAO e VBO para renderização do diálogo.
   * Chamado automaticamente na primeira renderização.
   */
  void InitializeOverlayResources();

  /**
   * @brief Liberta recursos OpenGL do overlay
   *
   * Deleta shader, VAO e VBO. Chamado no destrutor.
   */
  void CleanupOverlayResources();
  
  // ========================================================================
  // MINIMAP RESOURCES
  // ========================================================================
  
  /**
   * @brief VAO (Vertex Array Object) para o minimapa
   */
  unsigned int minimapVAO;

  /**
   * @brief VBO (Vertex Buffer Object) para o minimapa
   */
  unsigned int minimapVBO;

  /**
   * @brief Shader usado para renderização 2D de cor sólida
   * 
   * Usado pelo minimapa para desenhar paredes, jogador e fundo
   * sem texturas ou iluminação complexa.
   */
  class Shader *simpleShader; 
  
  /**
   * @brief Renderiza o Minimapa 2D
   *
   * Desenha uma representação top-down do labirinto no canto
   * superior direito do ecrã, mostrando:
   * - Paredes (Preto)
   * - Jogador (Vermelho)
   * - Fundo (Cinzento escuro)
   *
   * @note Usa projeção ortográfica 2D.
   */
  void RenderMinimap();
};

#endif // GAME_H