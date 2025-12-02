#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

// tirado do livro Learn OpenGL : cap. 20

// Estrutura para um vértice
struct Vertex {
  glm::vec3 Position; // Posição
  glm::vec3 Normal;   // Normal
  // glm::vec2 TexCoords; // Removido pois não usamos texturas
};

// Classe Mesh
class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  // Construtor
  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
    this->vertices = vertices;
    this->indices = indices;
    setupMesh(); // Configura os buffers
  }

  // Desenha a malha
  void Draw(GLuint shaderProgram) {
    glBindVertexArray(VAO);
    if (!indices.empty()) {
      // Desenha com índices se existirem
      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    } else {
      // Desenha array de vértices
      glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    }
    glBindVertexArray(0); // Desvincula VAO
  }

private:
  unsigned int VAO, VBO, EBO;

  // Configura os buffers da malha (VAO, VBO, EBO)
  void setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    if (!indices.empty()) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   indices.size() * sizeof(unsigned int), &indices[0],
                   GL_STATIC_DRAW);
    }

    // Atributo 0: Posição
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // Atributo 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));

    glBindVertexArray(0);
  }
};

#endif
