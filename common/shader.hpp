#pragma once
#include <string>
#include <GL/glew.h>

// Lê um ficheiro de texto (usado para carregar shaders)
std::string loadTextFile(const char* path);

// Compila um shader (GL_VERTEX_SHADER ou GL_FRAGMENT_SHADER)
GLuint compileShader(GLenum type, const char* source, const char* debugName);

// Liga um programa a partir de ficheiros VS/FS (compila + linka)
GLuint linkProgramFromFiles(const char* vertexPath, const char* fragmentPath);

// Helper para obter a localização de um uniform (com aviso caso não exista)
inline GLint getUniform(GLuint program, const char* name) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc < 0) std::fprintf(stderr, "[warn] uniform '%s' não encontrado\n", name);
    return loc;
}