#include "../include/common.h"
#include "../include/labirintos.h"
#include "../include/shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

// Variáveis globais
Jogador jogador;
GLFWwindow *window;
Shader *shader;
unsigned int playerVAO, playerVBO, playerEBO;
float projectionMatrix[16];

// Variáveis de rede
bool jogadorPresente = false;
mutex jogadorMutex;
int serverSocket = -1;
int clientSocket = -1;

// Controle de input para evitar movimento contínuo
double ultimoMovimento = 0.0;
const double intervaloMovimento = 0.15; // 150ms entre movimentos

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

  lock_guard<mutex> lock(jogadorMutex);

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
  }

  if (!moveu)
    return; // Nenhuma tecla de movimento foi pressionada

  // Verificar limites da janela
  if (novoX >= 0 && novoX <= LARGURA - TAMANHO_BLOCO && novoY >= 0 &&
      novoY <= ALTURA - TAMANHO_BLOCO) {

    // Verificar colisão com paredes
    if (!verificaColisao(novoX, novoY, mapaFim)) {
      jogador.x = novoX;
      jogador.y = novoY;
      ultimoMovimento = tempoAtual; // Atualizar tempo do último movimento

      // Verificar se chegou na saída (valor 2)
      int gridX = (int)(novoX / TAMANHO_BLOCO);
      int gridY = (int)(novoY / TAMANHO_BLOCO);

      if (mapaFim[gridY][gridX] == 2) {
        cout << "\n\n========================================" << endl;
        cout << "=== PARABÉNS! VOCÊ CHEGOU AO FIM! ===" << endl;
        cout << "========================================\n\n" << endl;
        glfwSetWindowShouldClose(window, true);
      }
    }
  }
}

// Callback para redimensionar janela
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

// Função para gerar saída aleatória garantindo que seja alcançável
void gerarSaida(int startRow) {
  // Limpar saídas anteriores
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 20; j++) {
      if (mapaFim[i][j] == 2)
        mapaFim[i][j] = 0;
    }
  }

  vector<pair<int, int>> posicoesAlcancaveis;
  bool visitado[20][20] = {false};
  queue<pair<int, int>> q;

  // Iniciar BFS a partir da entrada
  if (mapaFim[startRow][0] == 0) {
    q.push({startRow, 0});
    visitado[startRow][0] = true;
  }

  int dx[] = {0, 0, 1, -1};
  int dy[] = {1, -1, 0, 0};

  while (!q.empty()) {
    pair<int, int> atual = q.front();
    q.pop();

    int r = atual.first;
    int c = atual.second;

    // Adicionar à lista de candidatos se estiver longe o suficiente do início
    if (c > 5) {
      posicoesAlcancaveis.push_back({r, c});
    }

    // Verificar vizinhos
    for (int i = 0; i < 4; i++) {
      int nr = r + dy[i];
      int nc = c + dx[i];

      if (nr >= 0 && nr < 20 && nc >= 0 && nc < 20 && !visitado[nr][nc] &&
          mapaFim[nr][nc] == 0) {
        visitado[nr][nc] = true;
        q.push({nr, nc});
      }
    }
  }

  if (!posicoesAlcancaveis.empty()) {
    srand(time(NULL));
    int idx = rand() % posicoesAlcancaveis.size();
    int y = posicoesAlcancaveis[idx].first;
    int x = posicoesAlcancaveis[idx].second;

    mapaFim[y][x] = 2; // Definir como saída
    cout << "=== SAÍDA GERADA (VALIDADA) EM: Coluna " << x << " Linha " << y
         << " ===" << endl;
  } else {
    cout << "ERRO: Nenhuma posição alcançável encontrada!" << endl;
  }
}

// Thread do servidor TCP - recebe o jogador
void servidorThread() {
  cout << "=== Iniciando servidor TCP ===" << endl;

  // Criar socket
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    cerr << "Erro ao criar socket servidor: " << strerror(errno) << endl;
    return;
  }

  // Permitir reutilização da porta
  int opt = 1;
  setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Configurar endereço
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORTA);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Bind
  if (bind(serverSocket, (struct sockaddr *)&serverAddress,
           sizeof(serverAddress)) < 0) {
    cerr << "Erro no bind do servidor: " << strerror(errno) << endl;
    cerr << "A porta " << PORTA << " pode estar em uso!" << endl;
    close(serverSocket);
    return;
  }

  // Listen
  if (listen(serverSocket, 1) < 0) {
    cerr << "Erro no listen do servidor: " << strerror(errno) << endl;
    close(serverSocket);
    return;
  }

  cout << "✓ Servidor aguardando conexão na porta " << PORTA << "..." << endl;

  // Loop de aceitação de conexões
  while (true) {
    cout << "  [Aguardando cliente conectar...]" << endl;
    clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
      cerr << "Erro ao aceitar conexão: " << strerror(errno) << endl;
      continue;
    }

    cout << "✓ Cliente conectado!" << endl;

    // Receber dados do jogador
    Jogador jogadorRecebido;
    ssize_t bytesRecebidos =
        recv(clientSocket, &jogadorRecebido, sizeof(Jogador), 0);

    if (bytesRecebidos == sizeof(Jogador)) {
      lock_guard<mutex> lock(jogadorMutex);

      // Posicionar jogador na entrada do túnel (esquerda)
      jogador.x = 0;
      jogador.y = jogadorRecebido.y;
      jogador.velocidade = jogadorRecebido.velocidade;
      jogadorPresente = true;

      // Abrir buraco na parede onde o jogador chegou
      int gridY = (int)(jogador.y / TAMANHO_BLOCO);
      cout << "=== ABRINDO TÚNEL NA LINHA " << gridY << " ===" << endl;

      // Fechar entrada padrão (linha 9) se for diferente
      if (gridY != 9)
        mapaFim[9][0] = 1;

      // Abrir nova entrada
      mapaFim[gridY][0] = 0;
      mapaFim[gridY][1] = 0; // Garantir acesso

      // Gerar saída alcançável a partir desta entrada
      gerarSaida(gridY);

      cout << "✓ Jogador recebido! (" << bytesRecebidos << " bytes)" << endl;
      cout << "  Posição inicial: X=0 Y=" << jogador.y << endl;
    } else {
      cerr << "Erro: Bytes recebidos incorretos (" << bytesRecebidos
           << " != " << sizeof(Jogador) << ")" << endl;
    }

    close(clientSocket);
    clientSocket = -1;
  }
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
  window = glfwCreateWindow(LARGURA, ALTURA, "Labirinto - Cliente (PC B)", NULL,
                            NULL);
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

  // Inicializar jogador (começa sem estar presente)
  jogador.x = TAMANHO_BLOCO;
  jogador.y = TAMANHO_BLOCO;
  jogador.velocidade = TAMANHO_BLOCO;
  jogadorPresente = false; // Começa sem jogador

  // Inicializar geometria do jogador
  initPlayerGeometry();

  // Iniciar thread do servidor
  thread servidorTCP(servidorThread);
  servidorTCP.detach();

  cout << "Cliente iniciado. Aguardando jogador do host..." << endl;

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
    desenhaMapa(mapaFim, *shader);

    // Desenhar jogador apenas se estiver presente
    if (jogadorPresente) {
      lock_guard<mutex> lock(jogadorMutex);
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
  if (serverSocket >= 0)
    close(serverSocket);
  if (clientSocket >= 0)
    close(clientSocket);
  glDeleteVertexArrays(1, &playerVAO);
  glDeleteBuffers(1, &playerVBO);
  glDeleteBuffers(1, &playerEBO);
  delete shader;

  glfwTerminate();
  return 0;
}
