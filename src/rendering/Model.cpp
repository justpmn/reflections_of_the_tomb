
#include "Model.h"

#include <iostream>
#include <vector>
#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include "../third_party/cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

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

    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        const cgltf_mesh& sourceMesh = data->meshes[i];

        for (cgltf_size j = 0;
             j < sourceMesh.primitives_count; ++j)
        {
            const cgltf_primitive& primitive =
                sourceMesh.primitives[j];

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

            // Interleaved vertex data:
            // Position (3), Normal (3), Texture coordinates (2).
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

            // Position: location 0.
            glVertexAttribPointer(
                0, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)0
            );
            glEnableVertexAttribArray(0);

            // Normal: location 1.
            glVertexAttribPointer(
                1, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)(3 * sizeof(float))
            );
            glEnableVertexAttribArray(1);

            // Texture coordinates: location 2.
            glVertexAttribPointer(
                2, 2, GL_FLOAT, GL_FALSE,
                stride, (void*)(6 * sizeof(float))
            );
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);

            mesh.indexCount =
                static_cast<unsigned int>(indices.size());

            // Load the base colour texture, if available.
            if (primitive.material &&
                primitive.material->pbr_metallic_roughness
                    .base_color_texture.texture)
            {
                const cgltf_texture* texture =
                    primitive.material->pbr_metallic_roughness
                        .base_color_texture.texture;

                if (texture->image && texture->image->uri)
                {
                    const std::filesystem::path texturePath =
                        modelDirectory / texture->image->uri;

                    int imageWidth = 0;
                    int imageHeight = 0;
                    int channels = 0;

                    unsigned char* image = stbi_load(
                        texturePath.string().c_str(),
                        &imageWidth,
                        &imageHeight,
                        &channels,
                        0
                    );

                    if (image)
                    {
                        GLenum format = GL_RGB;

                        if (channels == 1)
                            format = GL_RED;
                        else if (channels == 3)
                            format = GL_RGB;
                        else if (channels == 4)
                            format = GL_RGBA;

                        glGenTextures(1, &mesh.textureID);
                        glBindTexture(
                            GL_TEXTURE_2D,
                            mesh.textureID
                        );

                        glTexImage2D(
                            GL_TEXTURE_2D,
                            0,
                            format,
                            imageWidth,
                            imageHeight,
                            0,
                            format,
                            GL_UNSIGNED_BYTE,
                            image
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

                        mesh.hasTexture = true;

                        stbi_image_free(image);

                        std::cout
                            << "Texture loaded: "
                            << texturePath.string()
                            << '\n';
                    }
                    else
                    {
                        std::cerr
                            << "Failed to load texture: "
                            << texturePath.string()
                            << " - "
                            << stbi_failure_reason()
                            << '\n';
                    }
                }
            }

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

void Model::draw(unsigned int shaderProgram) const
{
    if (!loaded)
        return;

    const GLint textureLocation =
        glGetUniformLocation(shaderProgram, "uTexture");

    const GLint hasTextureLocation =
        glGetUniformLocation(shaderProgram, "uHasTexture");

    for (const Mesh& mesh : meshes)
    {
        if (mesh.hasTexture)
        {
            glActiveTexture(GL_TEXTURE0);

            glBindTexture(
                GL_TEXTURE_2D,
                mesh.textureID
            );
        }

        if (textureLocation != -1)
            glUniform1i(textureLocation, 0);

        if (hasTextureLocation != -1)
            glUniform1i(
                hasTextureLocation,
                mesh.hasTexture ? 1 : 0
            );

        glBindVertexArray(mesh.VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(mesh.indexCount),
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
