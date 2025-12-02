#include "shader.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>

std::string loadTextFile(const char* path) {
    std::ifstream f(path, std::ios::in);
    if (!f) { std::fprintf(stderr, "[erro] não consegui abrir: %s\n", path); return {}; }
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const char* source, const char* debugName) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096]; glGetShaderInfoLog(s, sizeof log, nullptr, log);
        std::fprintf(stderr, "[shader %s] erro de compilação:\n%s\n", debugName, log);
        glDeleteShader(s); return 0;
    }
    return s;
}

GLuint linkProgramFromFiles(const char* vsPath, const char* fsPath) {
    std::string vsrc = loadTextFile(vsPath), fsrc = loadTextFile(fsPath);
    if (vsrc.empty() || fsrc.empty()) return 0;
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsrc.c_str(), vsPath); if (!vs) return 0;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsrc.c_str(), fsPath); if (!fs){ glDeleteShader(vs); return 0; }
    GLuint p = glCreateProgram(); glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok = GL_FALSE; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[4096]; glGetProgramInfoLog(p, sizeof log, nullptr, log);
        std::fprintf(stderr, "[link %s + %s] erro:\n%s\n", vsPath, fsPath, log);
        glDeleteProgram(p); return 0; }
    return p;
}