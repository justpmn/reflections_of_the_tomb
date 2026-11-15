
#include "Model.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <algorithm>

#define CGLTF_IMPLEMENTATION
#include "../third_party/cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

namespace
{
    // Transform a position using a column-major glTF matrix.
    void transformPosition(
        const cgltf_float matrix[16],
        float position[3])
    {
        const float x = position[0];
        const float y = position[1];
        const float z = position[2];

        position[0] =
            matrix[0] * x + matrix[4] * y +
            matrix[8] * z + matrix[12];

        position[1] =
            matrix[1] * x + matrix[5] * y +
            matrix[9] * z + matrix[13];

        position[2] =
            matrix[2] * x + matrix[6] * y +
            matrix[10] * z + matrix[14];
    }

    // Transform and normalize a normal.
    void transformNormal(
        const cgltf_float matrix[16],
        float normal[3])
    {
        const float x = normal[0];
        const float y = normal[1];
        const float z = normal[2];

        normal[0] =
            matrix[0] * x + matrix[4] * y + matrix[8] * z;

        normal[1] =
            matrix[1] * x + matrix[5] * y + matrix[9] * z;

        normal[2] =
            matrix[2] * x + matrix[6] * y + matrix[10] * z;

        const float length = std::sqrt(
            normal[0] * normal[0] +
            normal[1] * normal[1] +
            normal[2] * normal[2]
        );

        if (length > 0.00001f)
        {
            normal[0] /= length;
            normal[1] /= length;
            normal[2] /= length;
        }
    }

    unsigned int loadTexture(
        const cgltf_image* sourceImage,
        const std::filesystem::path& modelDirectory)
    {
        if (!sourceImage)
            return 0;

        int width = 0;
        int height = 0;
        int channels = 0;

        unsigned char* imageData = nullptr;

        // External image file.
        if (sourceImage->uri)
        {
            const std::filesystem::path texturePath =
                modelDirectory / sourceImage->uri;

            imageData = stbi_load(
                texturePath.string().c_str(),
                &width,
                &height,
                &channels,
                0
            );

            if (!imageData)
            {
                std::cerr << "Failed to load texture: "
                          << texturePath.string()
                          << " - " << stbi_failure_reason()
                          << '\n';
                return 0;
            }

            std::cout << "Texture loaded: "
                      << texturePath.string() << '\n';
        }
        // Embedded image inside a GLB buffer.
        else if (sourceImage->buffer_view)
        {
            const cgltf_buffer_view* view =
                sourceImage->buffer_view;

            const unsigned char* embeddedData =
                static_cast<const unsigned char*>(
                    cgltf_buffer_view_data(view)
                );

            if (!embeddedData || view->size == 0)
            {
                std::cerr << "Invalid embedded texture data.\n";
                return 0;
            }

            imageData = stbi_load_from_memory(
                embeddedData,
                static_cast<int>(view->size),
                &width,
                &height,
                &channels,
                0
            );

            if (!imageData)
            {
                std::cerr << "Failed to decode embedded texture: "
                          << stbi_failure_reason() << '\n';
                return 0;
            }

            std::cout << "Embedded GLB texture loaded successfully!\n";
        }
        else
        {
            std::cerr << "Image has no supported data source.\n";
            return 0;
        }

        GLenum format = GL_RGB;

        if (channels == 1)
            format = GL_RED;
        else if (channels == 3)
            format = GL_RGB;
        else if (channels == 4)
            format = GL_RGBA;

        unsigned int textureID = 0;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            imageData
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_REPEAT
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_REPEAT
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR_MIPMAP_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(imageData);

        return textureID;
    }
}

Model::~Model()
{
    cleanup();
}

bool Model::load(const std::string& filePath)
{
    cleanup();

    cgltf_options options{};
    cgltf_data* data = nullptr;

    cgltf_result result =
        cgltf_parse_file(&options, filePath.c_str(), &data);

    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to parse model: "
                  << filePath << '\n';
        return false;
    }

    result = cgltf_load_buffers(&options, data, filePath.c_str());

    if (result != cgltf_result_success)
    {
        std::cerr << "Failed to load model buffers.\n";
        cgltf_free(data);
        return false;
    }

    result = cgltf_validate(data);

    if (result != cgltf_result_success)
    {
        std::cerr << "Model validation failed.\n";
        cgltf_free(data);
        return false;
    }

    const std::filesystem::path modelDirectory =
        std::filesystem::path(filePath).parent_path();

    stbi_set_flip_vertically_on_load(true);

    // Process a mesh using the world transformation of its node.
    auto processMesh = [&](const cgltf_mesh* sourceMesh,
                           const cgltf_float worldMatrix[16])
    {
        if (!sourceMesh)
            return;

        for (cgltf_size j = 0;
             j < sourceMesh->primitives_count; ++j)
        {
            const cgltf_primitive& primitive =
                sourceMesh->primitives[j];

            if (primitive.type != cgltf_primitive_type_triangles)
                continue;

            const cgltf_accessor* positions = nullptr;
            const cgltf_accessor* normals = nullptr;
            const cgltf_accessor* texCoords = nullptr;

            for (cgltf_size k = 0;
                 k < primitive.attributes_count; ++k)
            {
                const cgltf_attribute& attribute =
                    primitive.attributes[k];

                if (attribute.type == cgltf_attribute_type_position)
                    positions = attribute.data;

                if (attribute.type == cgltf_attribute_type_normal)
                    normals = attribute.data;

                if (attribute.type == cgltf_attribute_type_texcoord)
                    texCoords = attribute.data;
            }

            if (!positions)
                continue;

            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            // Read vertices and apply node transformation.
            for (cgltf_size v = 0; v < positions->count; ++v)
            {
                float position[3] = {};
                float normal[3] = {0.0f, 1.0f, 0.0f};
                float uv[2] = {};

                if (!cgltf_accessor_read_float(
                    positions, v, position, 3))
                {
                    continue;
                }

                if (normals)
                {
                    cgltf_accessor_read_float(
                        normals, v, normal, 3);
                }

                if (texCoords)
                {
                    cgltf_accessor_read_float(
                        texCoords, v, uv, 2);
                }

                transformPosition(worldMatrix, position);
                transformNormal(worldMatrix, normal);
                minBounds = glm::min(minBounds, glm::vec3(position[0],position[1],position[2]));
                maxBounds = glm::max(maxBounds, glm::vec3(position[0],position[1],position[2]));

                vertices.insert(
                    vertices.end(),
                    {
                        position[0], position[1], position[2],
                        normal[0], normal[1], normal[2],
                        uv[0], uv[1]
                    }
                );
            }

            if (primitive.indices)
            {
                for (cgltf_size index = 0;
                     index < primitive.indices->count; ++index)
                {
                    indices.push_back(
                        static_cast<unsigned int>(
                            cgltf_accessor_read_index(
                                primitive.indices, index
                            )
                        )
                    );
                }
            }
            else
            {
                for (unsigned int index = 0;
                     index < vertices.size() / 8; ++index)
                {
                    indices.push_back(index);
                }
            }

            if (vertices.empty() || indices.empty())
                continue;

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

            const GLsizei stride = 8 * sizeof(float);

            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)0
            );
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)(3 * sizeof(float))
            );
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(
                2, 2, GL_FLOAT, GL_FALSE,
                stride, (void*)(6 * sizeof(float))
            );
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);

            mesh.indexCount =
                static_cast<unsigned int>(indices.size());

            // Load material base-colour texture.
            if (primitive.material &&
                primitive.material->pbr_metallic_roughness
                    .base_color_texture.texture)
            {
                const cgltf_texture* texture =
                    primitive.material->pbr_metallic_roughness
                        .base_color_texture.texture;

                mesh.textureID = loadTexture(
                    texture->image,
                    modelDirectory
                );

                mesh.hasTexture = mesh.textureID != 0;
            }

            meshes.push_back(mesh);
        }
    };

    // Track meshes that are referenced by scene nodes.
    std::vector<bool> meshReferenced(data->meshes_count, false);

    // Process every node and its complete world transformation.
    for (cgltf_size i = 0; i < data->nodes_count; ++i)
    {
        const cgltf_node* node = &data->nodes[i];

        if (!node->mesh)
            continue;

        cgltf_float worldMatrix[16];

        cgltf_node_transform_world(node, worldMatrix);

        const std::ptrdiff_t meshIndex =
            node->mesh - data->meshes;

        if (meshIndex >= 0 &&
            static_cast<cgltf_size>(meshIndex) < data->meshes_count)
        {
            meshReferenced[meshIndex] = true;
        }

        processMesh(node->mesh, worldMatrix);
    }

    // Support models containing meshes without scene nodes.
    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        if (meshReferenced[i])
            continue;

        cgltf_float identity[16] =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        processMesh(&data->meshes[i], identity);
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

    std::cout << "Mesh count: "
              << meshes.size() << '\n';

    return true;
}

// Draw every mesh in the model.
void Model::draw(unsigned int shaderProgram) const
{
    if (!loaded)
        return;

    for (unsigned int i = 0; i < meshes.size(); ++i)
    {
        drawMesh(i, shaderProgram);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Draw only one selected mesh.
void Model::drawMesh(
    unsigned int meshIndex,
    unsigned int shaderProgram) const
{
    if (!loaded || meshIndex >= meshes.size())
        return;

    const Mesh& mesh = meshes[meshIndex];

    const GLint textureLocation =
        glGetUniformLocation(shaderProgram, "uTexture");

    const GLint hasTextureLocation =
        glGetUniformLocation(shaderProgram, "uHasTexture");

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(
        GL_TEXTURE_2D,
        mesh.hasTexture ? mesh.textureID : 0
    );

    if (textureLocation != -1)
        glUniform1i(textureLocation, 0);

    if (hasTextureLocation != -1)
    {
        glUniform1i(
            hasTextureLocation,
            mesh.hasTexture ? 1 : 0
        );
    }

    glBindVertexArray(mesh.VAO);

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(mesh.indexCount),
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Return the number of meshes in the model.
unsigned int Model::getMeshCount() const
{
    return static_cast<unsigned int>(meshes.size());
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

        if (mesh.textureID)
            glDeleteTextures(1, &mesh.textureID);
    }

    meshes.clear();
    loaded = false;
}
