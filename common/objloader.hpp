#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <glm/glm.hpp>

struct Material {
    glm::vec3 Ka;        // Ambient color
    glm::vec3 Kd;        // Diffuse color
    glm::vec3 Ks;        // Specular color
    float Ns;            // Shininess exponent
    float d;             // Dissolve (transparency)
};

bool loadMTL(
    const char * path,
    std::map < std::string, Material > & out_materials
);

bool loadOBJ(
    const char * path,
    std::vector < glm::vec3 > & out_vertices,
    std::vector < glm::vec3 > & out_normals
);

/*
We want loadOBJ to read the file “path”, write the data in out_vertices/out_uvs/out_normals, and return false if something went wrong.
 std::vector is the C++ way to declare an array of glm::vec3 which size can be modified at will: it has nothing to do with a mathematical vector. 
Just an array, really. And finally, the & means that function will be able to modify the std::vectors.
*/