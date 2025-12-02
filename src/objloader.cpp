#include "objloader.hpp"
#include <cstring>
#include <cstdio>
#include <string>

// Carrega ficheiro .mtl (Material Template Library)
bool loadMTL(
    const char *path,
    std::map<std::string, Material> &out_materials)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        printf("Warning: Could not open material file (%s)\n", path);
        return false;
    }

    std::string currentMaterialName;
    Material currentMaterial = {{0.2f, 0.2f, 0.2f}, {0.8f, 0.8f, 0.8f}, {0.5f, 0.5f, 0.5f}, 32.0f, 1.0f};

    while (1)
    {
        char lineHeader[256];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;

        // Definir novo material
        if (strcmp(lineHeader, "newmtl") == 0)
        {
            if (!currentMaterialName.empty())
                out_materials[currentMaterialName] = currentMaterial;
            
            char matName[256];
            fscanf(file, "%s\n", matName);
            currentMaterialName = matName;
            currentMaterial = {{0.2f, 0.2f, 0.2f}, {0.8f, 0.8f, 0.8f}, {0.5f, 0.5f, 0.5f}, 32.0f, 1.0f};
        }
        // Ambient color
        else if (strcmp(lineHeader, "Ka") == 0)
        {
            fscanf(file, "%f %f %f\n", &currentMaterial.Ka.x, &currentMaterial.Ka.y, &currentMaterial.Ka.z);
        }
        // Diffuse color
        else if (strcmp(lineHeader, "Kd") == 0)
        {
            fscanf(file, "%f %f %f\n", &currentMaterial.Kd.x, &currentMaterial.Kd.y, &currentMaterial.Kd.z);
        }
        // Specular color
        else if (strcmp(lineHeader, "Ks") == 0)
        {
            fscanf(file, "%f %f %f\n", &currentMaterial.Ks.x, &currentMaterial.Ks.y, &currentMaterial.Ks.z);
        }
        // Shininess
        else if (strcmp(lineHeader, "Ns") == 0)
        {
            fscanf(file, "%f\n", &currentMaterial.Ns);
        }
        // Dissolve (transparency)
        else if (strcmp(lineHeader, "d") == 0)
        {
            fscanf(file, "%f\n", &currentMaterial.d);
        }
        // Ignore other fields (like texture maps)
        else
        {
            char dummy[1024];
            fgets(dummy, sizeof(dummy), file);
        }
    }

    // Add last material
    if (!currentMaterialName.empty())
        out_materials[currentMaterialName] = currentMaterial;

    fclose(file);
    return true;
}

// função que lê ficheiro .obj e devolve listas de vértices e normais
bool loadOBJ(
    const char *path,
    std::vector<glm::vec3> &out_vertices,
    std::vector<glm::vec3> &out_normals)
{
    // listas temporárias (armazenam tudo que for lido)
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    // abrir ficheiro pelo path dado
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        printf("Impossible to open the file ! (%s)\n", path);
        return false;
    }

    // ler até ao fim do ficheiro
    while (1)
    {
        char lineHeader[256]; // assumir que a palavra nao tem mais de 256 chars

        // ler a primeira palavra da linha
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break; // EOF = End Of File → sai do loop

        // analisar o que a primeira palavra significa
        // se for "v", a linha descreve um vértice (x y z)
        if (strcmp(lineHeader, "v") == 0)
        {
            glm::vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            temp_vertices.push_back(vertex);
        }

        // se for "vt", descreve coordenadas de textura (u v)
        else if (strcmp(lineHeader, "vt") == 0)
        {
            glm::vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            temp_uvs.push_back(uv);
        }

        // se for "vn", descreve uma normal (x y z)
        else if (strcmp(lineHeader, "vn") == 0)
        {
            glm::vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);
        }

        // se for "f", descreve uma face (índices dos vértices/uvs/normais)
        else if (strcmp(lineHeader, "f") == 0)
        {
            // read the rest of the line into a buffer
            char line[1024];
            if (!fgets(line, sizeof(line), file)) break;
            std::istringstream ss(line);
            std::string token;

            // collect face indices (support v, v/vt, v//vn, v/vt/vn)
            std::vector<unsigned int> face_v, face_vt, face_vn;
            while (ss >> token) {
                unsigned int vi = 0, ti = 0, ni = 0;
                if (sscanf(token.c_str(), "%u/%u/%u", &vi, &ti, &ni) == 3) {
                    face_v.push_back(vi);
                    face_vt.push_back(ti);
                    face_vn.push_back(ni);
                } else if (sscanf(token.c_str(), "%u//%u", &vi, &ni) == 2) {
                    face_v.push_back(vi);
                    face_vt.push_back(0);
                    face_vn.push_back(ni);
                } else if (sscanf(token.c_str(), "%u/%u", &vi, &ti) == 2) {
                    face_v.push_back(vi);
                    face_vt.push_back(ti);
                    face_vn.push_back(0);
                } else if (sscanf(token.c_str(), "%u", &vi) == 1) {
                    face_v.push_back(vi);
                    face_vt.push_back(0);
                    face_vn.push_back(0);
                }
            }

            if (face_v.size() < 3) {
                // malformed face
                fclose(file);
                return false;
            }

            // triangulate polygon (fan triangulation)
            for (size_t i = 1; i + 1 < face_v.size(); ++i) {
                vertexIndices.push_back(face_v[0]); vertexIndices.push_back(face_v[i]); vertexIndices.push_back(face_v[i+1]);
                uvIndices.push_back(face_vt[0]); uvIndices.push_back(face_vt[i]); uvIndices.push_back(face_vt[i+1]);
                normalIndices.push_back(face_vn[0]); normalIndices.push_back(face_vn[i]); normalIndices.push_back(face_vn[i+1]);
            }
        }
    }

    // até agora → só tínhamos listas de índices e dados separados
    // agora vamos juntar tudo numa forma que o OpenGL consiga usar diretamente

    // para cada vértice de cada triângulo (linhas "f")
    for (unsigned int i = 0; i < vertexIndices.size(); i++)
    {
        unsigned int vertexIndex = vertexIndices[i];
        glm::vec3 vertex = temp_vertices[vertexIndex - 1]; // -1 porque .obj começa em 1
        out_vertices.push_back(vertex);

        unsigned int normalIndex = normalIndices[i];
        if (normalIndex != 0 && normalIndex <= temp_normals.size()) {
            glm::vec3 normal = temp_normals[normalIndex - 1];
            out_normals.push_back(normal);
        } else {
            out_normals.push_back(glm::vec3(0, 1, 0)); // Default normal
        }
    }

    // fechar ficheiro
    fclose(file);

    // sucesso
    return true;
}
