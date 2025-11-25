#include "../include/common.h"
#include "../include/labirintos.h"
#include "../include/shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

// Variáveis globais
Jogador jogador;
GLFWwindow *window;
Shader *shader;
unsigned int playerVAO, playerVBO, playerEBO;
float projectionMatrix[16];

// Variáveis de rede
bool jogadorPresente = true;    // Host começa com o jogador
string ipCliente = "127.0.0.1"; // Localhost por padrão (mudar para IP do PC B)
int linhaTunel = 9;             // Linha do túnel (será randomizada)

// Controle de input para evitar movimento contínuo
double ultimoMovimento = 0.0;
const double intervaloMovimento = 0.15; // 150ms entre movimentos

// Forward declaration
void enviarJogador();

// Função de callback para teclado
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);

  if (!jogadorPresente)
    return; // Só processa input se jogador estiver presente

  // Verificar intervalo de tempo entre movimentos
  double tempoAtual = glfwGetTime();
  if (tempoAtual - ultimoMovimento < intervaloMovimento) {
    return; // Ainda não passou tempo suficiente
  }

  float novoX = jogador.x;
  float novoY = jogador.y;
  bool moveu = false;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    novoY -= jogador.velocidade;
    moveu = true;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    novoY += jogador.velocidade;
    moveu = true;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    novoX -= jogador.velocidade;
    moveu = true;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    novoX += jogador.velocidade;
    moveu = true;

    // Debug: mostrar posição e coluna
    int gridY = (int)(jogador.y / TAMANHO_BLOCO);
    int gridX = (int)(novoX / TAMANHO_BLOCO);
    cout << "Movendo direita - X: " << jogador.x << " -> " << novoX
         << " | Coluna: " << gridX << " | Linha: " << gridY
         << " | Limite: " << (LARGURA - TAMANHO_BLOCO) << endl;
  }

  if (!moveu)
    return; // Nenhuma tecla de movimento foi pressionada

  int gridY = (int)(jogador.y / TAMANHO_BLOCO);

  // Verificar se está na linha do túnel e tentando sair pela direita
  if (gridY == linhaTunel && novoX > LARGURA - 2 * TAMANHO_BLOCO) {
    cout << "PRÓXIMO DO TÚNEL! X=" << novoX << endl;

    // Permitir movimento para a direita no túnel sem verificar colisão
    jogador.x = novoX;
    jogador.y = novoY;
    ultimoMovimento = tempoAtual;

    // Se ultrapassou a borda, enviar
    if (novoX >= LARGURA) {
      cout << "✓ TÚNEL DETECTADO! Jogador saindo pelo túnel..." << endl;
      enviarJogador();
    }
    return;
  }

  // Verificar se está tentando sair pela direita em outras linhas
  if (novoX > LARGURA - TAMANHO_BLOCO && gridY != linhaTunel) {
    cout << "✗ Tentando sair fora do túnel! Bloqueado na linha " << gridY
         << endl;
    return;
  }

  // Verificar limites da janela (cima, baixo, esquerda)
  if (novoX < 0 || novoY < 0 || novoY > ALTURA - TAMANHO_BLOCO) {
    return; // Fora dos limites
  }

  // Verificar colisão com paredes (só se ainda estiver dentro do mapa)
  if (novoX <= LARGURA - TAMANHO_BLOCO &&
      !verificaColisao(novoX, novoY, mapaInicio)) {
    jogador.x = novoX;
    jogador.y = novoY;
    ultimoMovimento = tempoAtual;
  } else if (novoX > LARGURA - TAMANHO_BLOCO) {
    // Já tratado acima
  } else {
    cout << "Colisão com parede!" << endl;
  }
}

// Callback para redimensionar janela
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

// Função para enviar jogador ao cliente via TCP
void enviarJogador() {
  cout << "=== Tentando enviar jogador ===" << endl;
  cout << "IP destino: " << ipCliente << ":" << PORTA << endl;

  int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket < 0) {
    cerr << "Erro ao criar socket cliente: " << strerror(errno) << endl;
    return;
  }

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORTA);

  if (inet_pton(AF_INET, ipCliente.c_str(), &serverAddress.sin_addr) <= 0) {
    cerr << "Endereço inválido: " << ipCliente << endl;
    close(clientSocket);
    return;
  }

  cout << "Conectando ao servidor..." << endl;
  if (connect(clientSocket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) < 0) {
    cerr << "Erro ao conectar ao servidor: " << strerror(errno) << endl;
    cerr << "Certifica-te que o cliente está a correr!" << endl;
    close(clientSocket);
    return;
  }

  cout << "Conectado! Enviando dados..." << endl;
  // Enviar dados do jogador
  ssize_t bytesSent = send(clientSocket, &jogador, sizeof(Jogador), 0);
  if (bytesSent < 0) {
    cerr << "Erro ao enviar dados: " << strerror(errno) << endl;
  } else {
    cout << "✓ Jogador enviado com sucesso! (" << bytesSent << " bytes)"
         << endl;
    cout << "  Posição: X=" << jogador.x << " Y=" << jogador.y << endl;
    // Remover jogador deste PC
    jogadorPresente = false;
  }

  close(clientSocket);
}

// Inicializar geometria do jogador
void initPlayerGeometry() {
  float vertices[] = {0.0f,          0.0f, TAMANHO_BLOCO, 0.0f, TAMANHO_BLOCO,
                      TAMANHO_BLOCO, 0.0f, TAMANHO_BLOCO};

  unsigned int indices[] = {0, 1, 2, 2, 3, 0};

  glGenVertexArrays(1, &playerVAO);
  glGenBuffers(1, &playerVBO);
  glGenBuffers(1, &playerEBO);

  glBindVertexArray(playerVAO);

  glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

int main(int argc, char **argv) {
  // Verificar se foi passado IP do cliente como argumento
  if (argc > 1) {
    ipCliente = argv[1];
    cout << "IP do cliente: " << ipCliente << endl;
  } else {
    cout << "Usando localhost (127.0.0.1). Para usar outro IP: ./host <IP>"
         << endl;
  }

  // Inicializar GLFW
  if (!glfwInit()) {
    cerr << "Erro ao inicializar GLFW" << endl;
    return -1;
  }

  // Configurar GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Criar janela
  window =
      glfwCreateWindow(LARGURA, ALTURA, "Labirinto - Host (PC A)", NULL, NULL);
  if (!window) {
    cerr << "Erro ao criar janela GLFW" << endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Inicializar GLEW
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    cerr << "Erro ao inicializar GLEW" << endl;
    return -1;
  }

  // Configurar viewport
  glViewport(0, 0, LARGURA, ALTURA);

  // Criar shader
  shader = new Shader("shaders/vertex.glsl", "shaders/fragment.glsl");

  // Criar matriz de projeção ortográfica
  createOrthoMatrix(projectionMatrix, 0, LARGURA, ALTURA, 0);

  // Inicializar jogador
  jogador.x = TAMANHO_BLOCO;
  jogador.y = TAMANHO_BLOCO;
  jogador.velocidade = TAMANHO_BLOCO;

  // Inicializar geometria do jogador
  initPlayerGeometry();

  // Randomizar linha do túnel
  srand(time(NULL));
  linhaTunel = rand() % 18 + 1; // 1 a 18 (evita bordas 0 e 19)
  cout << "=== TÚNEL GERADO NA LINHA " << linhaTunel << " ===" << endl;

  // Fechar túnel padrão (linha 9)
  mapaInicio[9][19] = 1;

  // Abrir novo túnel
  mapaInicio[linhaTunel][19] = 0;
  mapaInicio[linhaTunel][18] = 0; // Garantir acesso

  // Loop principal
  while (!glfwWindowShouldClose(window)) {
    // Input
    processInput(window);

    // Render
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    shader->use();
    shader->setMat4("projection", projectionMatrix);

    // Desenhar mapa
    desenhaMapa(mapaInicio, *shader);

    // Desenhar jogador apenas se estiver presente
    if (jogadorPresente) {
      shader->setVec3("color", 1.0f, 0.0f, 0.0f); // Vermelho
      shader->setVec2("offset", jogador.x, jogador.y);
      glBindVertexArray(playerVAO);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Swap buffers e poll eventos
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Limpeza
  glDeleteVertexArrays(1, &playerVAO);
  glDeleteBuffers(1, &playerVBO);
  glDeleteBuffers(1, &playerEBO);
  delete shader;

  glfwTerminate();
  return 0;
}
