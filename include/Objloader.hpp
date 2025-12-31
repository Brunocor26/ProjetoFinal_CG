#ifndef OBJLOADER_HPP
#define OBJLOADER_HPP

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Structure representing a material property.
 * 
 * Contains ambient, diffuse, and specular color components,
 * as well as shininess and transparency.
 */
struct Material {
  glm::vec3 Ka; /**< Ambient color reflection */
  glm::vec3 Kd; /**< Diffuse color reflection */
  glm::vec3 Ks; /**< Specular color reflection */
  float Ns;     /**< Shininess exponent */
  float d;      /**< Dissolve/Transparency (1.0 = opaque, 0.0 = fully transparent) */
};

/**
 * @brief Loads material properties from an MTL file.
 * 
 * @param path Path to the .mtl file.
 * @param out_materials Map to store the loaded materials, keyed by material name.
 * @return true If the file was successfully parsed.
 * @return false If the file could not be opened or parsed.
 */
bool loadMTL(const char *path, std::map<std::string, Material> &out_materials);

/**
 * @brief Loads a 3D model from an OBJ file.
 * 
 * Reads vertex positions, UV coordinates, and normals from the specified file.
 * 
 * @param path Path to the .obj file.
 * @param out_vertices Vector to store the loaded vertex positions.
 * @param out_uvs Vector to store the loaded texture coordinates.
 * @param out_normals Vector to store the loaded normal vectors.
 * @return true If the model was successfully loaded.
 * @return false If the file could not be opened or simplified OBJ parsing failed.
 */
bool loadOBJ(const char *path, std::vector<glm::vec3> &out_vertices,
             std::vector<glm::vec2> &out_uvs,
             std::vector<glm::vec3> &out_normals);

#endif