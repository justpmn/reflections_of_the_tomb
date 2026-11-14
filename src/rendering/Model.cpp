#include "Model.h"

#include <iostream>
#include <vector>

#define CGLTF_IMPLEMENTATION
#include "../third_party/cgltf.h"

Model::~Model()
{
    cleanup();
}

bool Model::load(const std::string& filePath)
{
    cleanup();

    cgltf_options options{};
    cgltf_data* data = nullptr;

    // Parse the GLB or GLTF file.
    cgltf_result result =
        cgltf_parse_file(&options, filePath.c_str(), &data);

    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to parse model: " << filePath << '\n';
        return false;
    }

    // Load external buffers, if required.
    result = cgltf_load_buffers(&options, data, filePath.c_str());

    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to load model buffers.\n";
        cgltf_free(data);
        return false;
    }

    // Validate the model.
    result = cgltf_validate(data);

    if (result != cgltf_result_success)
    {
        std::cerr << "Model validation failed.\n";
        cgltf_free(data);
        return false;
    }

    // Process every mesh in the model.
    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        const cgltf_mesh& sourceMesh = data->meshes[i];

        for (cgltf_size j = 0; j < sourceMesh.primitives_count; ++j)
        {
            const cgltf_primitive& primitive = sourceMesh.primitives[j];

            // This initial loader supports triangle geometry.
            if (primitive.type != cgltf_primitive_type_triangles)
                continue;

            // Find the position attribute.
            const cgltf_accessor* positions = nullptr;

            for (cgltf_size k = 0; k < primitive.attributes_count; ++k)
            {
                if (primitive.attributes[k].type ==
                    cgltf_attribute_type_position)
                {
                    positions = primitive.attributes[k].data;
                    break;
                }
            }

            if (!positions)
                continue;

            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            // Read vertex positions.
            for (cgltf_size v = 0; v < positions->count; ++v)
            {
                float position[3] = {};

                if (!cgltf_accessor_read_float(
                    positions, v, position, 3))
                {
                    continue;
                }

                vertices.push_back(position[0]);
                vertices.push_back(position[1]);
                vertices.push_back(position[2]);
            }

            // Read indices or generate them when absent.
            if (primitive.indices)
            {
                for (cgltf_size index = 0;
                     index < primitive.indices->count; ++index)
                {
                    indices.push_back(static_cast<unsigned int>(
                        cgltf_accessor_read_index(
                            primitive.indices, index)));
                }
            }
            else
            {
                for (unsigned int index = 0;
                     index < vertices.size() / 3; ++index)
                {
                    indices.push_back(index);
                }
            }

            if (vertices.empty() || indices.empty())
                continue;

            // Create an OpenGL mesh.
            Mesh mesh;

            glGenVertexArrays(1, &mesh.VAO);
            glGenBuffers(1, &mesh.VBO);
            glGenBuffers(1, &mesh.EBO);

            glBindVertexArray(mesh.VAO);

            glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
            glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(float),
                vertices.data(),
                GL_STATIC_DRAW
            );

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(unsigned int),
                indices.data(),
                GL_STATIC_DRAW
            );

            // Position attribute: location 0.
            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE,
                3 * sizeof(float), nullptr
            );

            glEnableVertexAttribArray(0);

            glBindVertexArray(0);

            mesh.indexCount =
                static_cast<unsigned int>(indices.size());

            meshes.push_back(mesh);
        }
    }

    cgltf_free(data);

    if (meshes.empty())
    {
        std::cerr << "No supported triangle meshes found.\n";
        return false;
    }

    loaded = true;

    std::cout << "Model loaded successfully: "
              << filePath << '\n';

    return true;
}

void Model::draw() const
{
    if (!loaded)
        return;

    for (const Mesh& mesh : meshes)
    {
        glBindVertexArray(mesh.VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mesh.indexCount),
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
}

void Model::cleanup()
{
    for (Mesh& mesh : meshes)
    {
        if (mesh.EBO)
            glDeleteBuffers(1, &mesh.EBO);

        if (mesh.VBO)
            glDeleteBuffers(1, &mesh.VBO);

        if (mesh.VAO)
            glDeleteVertexArrays(1, &mesh.VAO);
    }

    meshes.clear();
    loaded = false;
}
