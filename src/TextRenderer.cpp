#include "../include/TextRenderer.h"
#include <glm/ext/matrix_clip_space.hpp> // for glm::ortho
#include <iostream>

TextRenderer::TextRenderer(unsigned int width, unsigned int height) {
  // Load and configure shader
  const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;
    uniform mat4 projection;
    void main() {
      gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
      TexCoords = vertex.zw;
    }
  )";

  const char *fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 color;
    uniform sampler2D text;
    uniform vec3 textColor;
    void main() {
      vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
      color = vec4(textColor, 1.0) * sampled;
    }
  )";

  // Compile shaders
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // Link shaders
  this->TextShader = glCreateProgram();
  glAttachShader(this->TextShader, vertexShader);
  glAttachShader(this->TextShader, fragmentShader);
  glLinkProgram(this->TextShader);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Configure VAO/VBO for texture quads
  glGenVertexArrays(1, &this->VAO);
  glGenBuffers(1, &this->VBO);
  glBindVertexArray(this->VAO);
  glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Set projection matrix (orthographic for 2D text rendering)
  glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 0.0f,
                                    static_cast<float>(height));
  glUseProgram(this->TextShader);
  glUniformMatrix4fv(glGetUniformLocation(this->TextShader, "projection"), 1,
                     GL_FALSE, &projection[0][0]);
}

void TextRenderer::Load(std::string font, unsigned int fontSize) {
  // Clear any previously loaded characters
  this->Characters.clear();

  // Initialize FreeType
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    std::cout << "ERROR::FREETYPE: Could not init FreeType Library"
              << std::endl;
    return;
  }

  // Load font as face
  FT_Face face;
  if (FT_New_Face(ft, font.c_str(), 0, &face)) {
    std::cout << "ERROR::FREETYPE: Failed to load font: " << font << std::endl;
    return;
  }

  // Set size to load glyphs as
  FT_Set_Pixel_Sizes(face, 0, fontSize);

  // Disable byte-alignment restriction
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Load first 128 characters of ASCII set
  for (unsigned char c = 0; c < 128; c++) {
    // Load character glyph
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      std::cout << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
      continue;
    }

    // Generate texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
                 face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
                 face->glyph->bitmap.buffer);

    // Set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Store character for later use
    Character character = {
        texture,
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        static_cast<unsigned int>(face->glyph->advance.x)};
    Characters.insert(std::pair<char, Character>(c, character));
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  // Destroy FreeType once we're finished
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  std::cout << "Font '" << font << "' loaded successfully!" << std::endl;
}

void TextRenderer::RenderText(std::string text, float x, float y, float scale,
                              glm::vec3 color) {
  // Activate corresponding render state
  glUseProgram(this->TextShader);
  glUniform3f(glGetUniformLocation(this->TextShader, "textColor"), color.x,
              color.y, color.z);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(this->VAO);

  // Iterate through all characters
  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) {
    Character ch = Characters[*c];

    float xpos = x + ch.Bearing.x * scale;
    float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

    float w = ch.Size.x * scale;
    float h = ch.Size.y * scale;

    // Update VBO for each character
    float vertices[6][4] = {
        {xpos, ypos + h, 0.0f, 0.0f},    {xpos, ypos, 0.0f, 1.0f},
        {xpos + w, ypos, 1.0f, 1.0f},

        {xpos, ypos + h, 0.0f, 0.0f},    {xpos + w, ypos, 1.0f, 1.0f},
        {xpos + w, ypos + h, 1.0f, 0.0f}};

    // Render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Advance cursors for next glyph
    x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels
  }
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

float TextRenderer::CalculateTextWidth(std::string text, float scale) {
  float width = 0.0f;
  std::string::const_iterator c;
  for (c = text.begin(); c != text.end(); c++) {
    Character ch = Characters[*c];
    width += (ch.Advance >> 6) * scale;
  }
  return width;
}
