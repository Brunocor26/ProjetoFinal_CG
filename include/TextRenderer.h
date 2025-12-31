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

/**
 * @brief Renders text using FreeType and OpenGL.
 */
class TextRenderer {
public:
  /** @brief Holds a list of pre-compiled Characters */
  std::map<char, Character> Characters;
  /** @brief Shader used for text rendering */
  unsigned int TextShader;
  
  /**
   * @brief Construct a new Text Renderer object.
   * 
   * @param width Screen width.
   * @param height Screen height.
   */
  TextRenderer(unsigned int width, unsigned int height);
  
  /**
   * @brief Pre-compiles a list of characters from the given font.
   * 
   * @param font Path to the font file.
   * @param fontSize Size of the font to load.
   */
  void Load(std::string font, unsigned int fontSize);
  
  /**
   * @brief Renders a string of text using the precompiled list of characters.
   * 
   * @param text The text string to render.
   * @param x Screen X position.
   * @param y Screen Y position.
   * @param scale Scaling factor.
   * @param color Text color (RGB).
   */
  void RenderText(std::string text, float x, float y, float scale,
                  glm::vec3 color = glm::vec3(1.0f));
                  
  /**
   * @brief Calculates the width of a text string in pixels.
   * 
   * @param text The text to measure.
   * @param scale The scale factor to apply.
   * @return float The total width in pixels.
   */
  float CalculateTextWidth(std::string text, float scale);

private:
  // Render state
  unsigned int VAO, VBO;
};

#endif
