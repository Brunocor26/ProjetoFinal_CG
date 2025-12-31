#ifndef MESH_H
#define MESH_H

#include "Texture.h"
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

// tirado do livro Learn OpenGL : cap. 20

// Estrutura para um vértice
/**
 * @brief Estrutura que representa um vértice 3D
 */
struct Vertex {
  /// Posição do vértice no espaço 3D (x, y, z)
  glm::vec3 Position;
  
  /// Vetor normal do vértice (para iluminação)
  glm::vec3 Normal;
  
  /// Coordenadas de textura (u, v)
  glm::vec2 TexCoords;
};

/**
 * @brief Classe que encapsula uma malha 3D (Mesh)
 *
 * Gere os dados geométricos (vértices, índices) e materiais (texturas)
 * de um objeto 3D. Responsável por configurar os buffers OpenGL (VAO, VBO, EBO)
 * e realizar o desenho (draw call).
 */
class Mesh {
public:
  /// Lista de vértices da malha
  std::vector<Vertex> vertices;
  
  /// Lista de índices para desenho otimizado (EBO)
  std::vector<unsigned int> indices;
  
  /// Lista de texturas associadas à malha
  std::vector<Texture> textures;

  /**
   * @brief Construtor da Mesh
   * 
   * @param vertices Vetor de vértices
   * @param indices Vetor de índices
   * @param textures Vetor de texturas
   */
  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices,
       std::vector<Texture> textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;
    setupMesh(); // Configura os buffers
  }

  // Desenha a malha
  void Draw(GLuint shaderProgram) {
    // bind appropriate textures
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    unsigned int roughnessNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++) {
      glActiveTexture(GL_TEXTURE0 +
                      i); // active proper texture unit before binding
      // retrieve texture number (the N in diffuse_textureN)
      std::string number;
      std::string name = textures[i].type;
      if (name == "texture_diffuse")
        number = std::to_string(diffuseNr++);
      else if (name == "texture_specular")
        number =
            std::to_string(specularNr++); // transfer unsigned int to stream
      else if (name == "texture_normal")
        number = std::to_string(normalNr++); // transfer unsigned int to stream
      else if (name == "texture_height")
        number = std::to_string(heightNr++); // transfer unsigned int to stream
      else if (name == "texture_roughness")
        number =
            std::to_string(roughnessNr++); // transfer unsigned int to stream

      // now set the sampler to the correct texture unit
      glUniform1i(glGetUniformLocation(shaderProgram, (name + number).c_str()),
                  i);
      // and finally bind the texture
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // For this specific simple shader that uses "texture1", we might want a
    // fallback or ensure we name it "texture_diffuse1" in shader? User's shader
    // uses "texture1". Let's assume texture_diffuse1 maps to texture1 logic if
    // we change shader, OR just manually ensure we set "texture1" if we only
    // have one texture. Hack for compatibility with current shader "texture1"
    // which we hardcoded to 0 in Game.cpp: If we have textures, the loop binds
    // GL_TEXTURE0. The shader expects texture1 at unit 0. So if names don't
    // match, at least the binding is at 0. But `glUniform1i` above tries to set
    // "texture_diffuse1" to 0. If shader has "texture1", "texture_diffuse1"
    // won't be found. We should probably update the shader to
    // "texture_diffuse1" or force "texture1" here. Let's stick to LearnOpenGL
    // loop but also try to set "texture1" uniform for backward compat if it
    // behaves as diffuse.
    if (textures.size() > 0) {
      glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    }

    glBindVertexArray(VAO);
    if (!indices.empty()) {
      // Desenha com índices se existirem
      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    } else {
      // Desenha array de vértices
      glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    }
    glBindVertexArray(0); // Desvincula VAO

    // always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
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
    // Attribute 2: TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
  }
};

#endif
