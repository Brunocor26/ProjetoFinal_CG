#include "../include/Objloader.hpp"
#include <cstring>

bool loadMTL(const char *path, std::map<std::string, Material> &out_materials) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open MTL file: " << path << std::endl;
    return false;
  }

  std::string currentMaterial;
  std::string line;

  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "newmtl") {
      iss >> currentMaterial;
      out_materials[currentMaterial] = Material();
    } else if (prefix == "Ka") {
      iss >> out_materials[currentMaterial].Ka.x >>
          out_materials[currentMaterial].Ka.y >>
          out_materials[currentMaterial].Ka.z;
    } else if (prefix == "Kd") {
      iss >> out_materials[currentMaterial].Kd.x >>
          out_materials[currentMaterial].Kd.y >>
          out_materials[currentMaterial].Kd.z;
    } else if (prefix == "Ks") {
      iss >> out_materials[currentMaterial].Ks.x >>
          out_materials[currentMaterial].Ks.y >>
          out_materials[currentMaterial].Ks.z;
    } else if (prefix == "Ns") {
      iss >> out_materials[currentMaterial].Ns;
    } else if (prefix == "d") {
      iss >> out_materials[currentMaterial].d;
    }
  }

  file.close();
  return true;
}

bool loadOBJ(const char *path, std::vector<glm::vec3> &out_vertices,
             std::vector<glm::vec2> &out_uvs,
             std::vector<glm::vec3> &out_normals) {
  std::vector<glm::vec3> temp_vertices;
  std::vector<glm::vec2> temp_uvs;
  std::vector<glm::vec3> temp_normals;
  std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open OBJ file: " << path << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;

    if (prefix == "v") {
      // Vertex position
      glm::vec3 vertex;
      iss >> vertex.x >> vertex.y >> vertex.z;
      temp_vertices.push_back(vertex);
    } else if (prefix == "vt") {
      // Texture coordinate
      glm::vec2 uv;
      iss >> uv.x >> uv.y;
      temp_uvs.push_back(uv);
    } else if (prefix == "vn") {
      // Vertex normal
      glm::vec3 normal;
      iss >> normal.x >> normal.y >> normal.z;
      temp_normals.push_back(normal);
    } else if (prefix == "f") {
      // Face indices
      std::string vertex1, vertex2, vertex3;
      iss >> vertex1 >> vertex2 >> vertex3;

      // Parse face format: v/vt/vn or v//vn or v/vt or v
      unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
      bool hasUVs = false;
      bool hasNormals = false;

      for (int i = 0; i < 3; i++) {
        const char *vertexStr =
            (i == 0 ? vertex1.c_str()
                    : (i == 1 ? vertex2.c_str() : vertex3.c_str()));
        int v, vt, vn;

        // Try v/vt/vn format
        if (sscanf(vertexStr, "%d/%d/%d", &v, &vt, &vn) == 3) {
          vertexIndex[i] = v;
          uvIndex[i] = vt;
          normalIndex[i] = vn;
          hasUVs = true;
          hasNormals = true;
        }
        // Try v//vn format
        else if (sscanf(vertexStr, "%d//%d", &v, &vn) == 2) {
          vertexIndex[i] = v;
          normalIndex[i] = vn;
          hasNormals = true;
        }
        // Try v/vt format
        else if (sscanf(vertexStr, "%d/%d", &v, &vt) == 2) {
            vertexIndex[i] = v;
            uvIndex[i] = vt;
            hasUVs = true;
        }
        // Try just v
        else if (sscanf(vertexStr, "%d", &v) == 1) {
          vertexIndex[i] = v;
        }
      }

      // Store indices (OBJ is 1-indexed, convert to 0-indexed)
      vertexIndices.push_back(vertexIndex[0] - 1);
      vertexIndices.push_back(vertexIndex[1] - 1);
      vertexIndices.push_back(vertexIndex[2] - 1);

      if (hasUVs) {
          uvIndices.push_back(uvIndex[0] - 1);
          uvIndices.push_back(uvIndex[1] - 1);
          uvIndices.push_back(uvIndex[2] - 1);
      }

      if (hasNormals) {
        normalIndices.push_back(normalIndex[0] - 1);
        normalIndices.push_back(normalIndex[1] - 1);
        normalIndices.push_back(normalIndex[2] - 1);
      }
    }
  }

  file.close();

  // Build output arrays
  for (unsigned int i = 0; i < vertexIndices.size(); i++) {
    unsigned int vertexIndex = vertexIndices[i];
    glm::vec3 vertex = temp_vertices[vertexIndex];
    out_vertices.push_back(vertex);

    if (i < uvIndices.size()) {
        unsigned int uvIndex = uvIndices[i];
        if (uvIndex < temp_uvs.size()) {
            glm::vec2 uv = temp_uvs[uvIndex];
            out_uvs.push_back(uv);
        } else {
             out_uvs.push_back(glm::vec2(0.0f, 0.0f));
        }
    } else {
         out_uvs.push_back(glm::vec2(0.0f, 0.0f));
    }

    if (i < normalIndices.size()) {
      unsigned int normalIndex = normalIndices[i];
      glm::vec3 normal = temp_normals[normalIndex];
      out_normals.push_back(normal);
    } else {
      // If no normals, add placeholder
      out_normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    }
  }

  std::cout << "OBJ Loaded: " << path << " | Vertices: " << out_vertices.size()
            << " | UVs: " << out_uvs.size() 
            << " | Normals: " << out_normals.size() << std::endl;

  return true;
}
