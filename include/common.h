#ifndef COMMON_H
#define COMMON_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Constantes do jogo
#define LARGURA 600
#define ALTURA 600
#define TAMANHO_BLOCO 30.0f
#define PORTA 8080

// Estrutura do jogador
struct Jogador {
  float x;
  float y;
  float velocidade;
};

// Estrutura para matriz de projeção ortográfica
inline void createOrthoMatrix(float *matrix, float left, float right,
                              float bottom, float top) {
  // Inicializar matriz identidade
  for (int i = 0; i < 16; i++)
    matrix[i] = 0.0f;

  matrix[0] = 2.0f / (right - left);
  matrix[5] = 2.0f / (top - bottom);
  matrix[10] = -1.0f;
  matrix[12] = -(right + left) / (right - left);
  matrix[13] = -(top + bottom) / (top - bottom);
  matrix[15] = 1.0f;
}

#endif
