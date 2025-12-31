#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <ft2build.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include FT_FREETYPE_H

#include <map>
#include <string>

// Character info storing texture data
struct Character {
  unsigned int TextureID; // ID handle of the glyph texture
  glm::ivec2 Size;        // Size of glyph
  glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
  unsigned int Advance;   // Offset to advance to next glyph
};

class TextRenderer {
public:
  // Holds a list of pre-compiled Characters
  std::map<char, Character> Characters;
  // Shader used for text rendering
  unsigned int TextShader;
  // Constructor
  TextRenderer(unsigned int width, unsigned int height);
  // Pre-compiles a list of characters from the given font
  void Load(std::string font, unsigned int fontSize);
  // Renders a string of text using the precompiled list of characters
  void RenderText(std::string text, float x, float y, float scale,
                  glm::vec3 color = glm::vec3(1.0f));
  // Calculates the width of a text string in pixels
  float CalculateTextWidth(std::string text, float scale);

private:
  // Render state
  unsigned int VAO, VBO;
};

#endif
