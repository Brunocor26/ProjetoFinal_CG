#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>

/**
 * @brief Represents a loaded texture.
 */
struct Texture {
  unsigned int id;   /**< OpenGL Texture ID */
  std::string type;  /**< Map type (e.g., "texture_diffuse", "texture_specular") */
  std::string path;  /**< File path of the loaded texture */
};

#endif
