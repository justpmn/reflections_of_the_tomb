
#include "Model.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

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
        const glm::mat3 normalMatrix=glm::transpose(glm::inverse(glm::mat3(glm::make_mat4(matrix))));
        const auto value=glm::normalize(normalMatrix*glm::make_vec3(normal));
        std::copy(glm::value_ptr(value),glm::value_ptr(value)+3,normal);
    }

    unsigned int loadTexture(
        const cgltf_texture* sourceTexture,
        const std::filesystem::path& modelDirectory)
    {
        const cgltf_image* sourceImage=sourceTexture?sourceTexture->image:nullptr;
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
                4
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
                4
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

        channels=4; // Decode every source to RGBA; handles grayscale+alpha safely.
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
            channels==4?GL_SRGB8_ALPHA8:(channels==3?GL_SRGB8:GL_R8),
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

        if(sourceTexture->sampler) {
            const auto& sampler=*sourceTexture->sampler;
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,sampler.wrap_s);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,sampler.wrap_t);
            if(sampler.min_filter) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,sampler.min_filter);
            if(sampler.mag_filter) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,sampler.mag_filter);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(imageData);

        return textureID;
    }
}

struct Model::AnimationState {
    cgltf_data* model=nullptr;
    cgltf_data* idle=nullptr;
    cgltf_data* walk=nullptr;
    const cgltf_animation* idleClip=nullptr;
    const cgltf_animation* walkClip=nullptr;
    std::vector<cgltf_node> rest;
    struct Primitive {
        const cgltf_skin* skin=nullptr;
        std::vector<float> vertices;
        std::vector<glm::uvec4> joints;
        std::vector<glm::vec4> weights;
    };
    std::vector<Primitive> primitives;
    ~AnimationState() {cgltf_free(model);cgltf_free(idle);cgltf_free(walk);}
};

Model::~Model()
{
    cleanup();
}

bool Model::load(const std::string& filePath, bool retainSkin)
{
    cleanup();
    minBounds=glm::vec3(1e20f); maxBounds=glm::vec3(-1e20f);

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

    stbi_set_flip_vertically_on_load(false);
    if (retainSkin) {
        animation=new AnimationState;
        animation->model=data;
        animation->rest.assign(data->nodes,data->nodes+data->nodes_count);
    }

    std::unordered_map<const cgltf_texture*,GLuint> textureCache;
    // Process a mesh using the world transformation of its node.
    auto processMesh = [&](const cgltf_mesh* sourceMesh,
                           const cgltf_float worldMatrix[16], const cgltf_skin* skin)
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
            const cgltf_accessor* joints = nullptr;
            const cgltf_accessor* weights = nullptr;
            const cgltf_accessor* colors = nullptr;

            for (cgltf_size k = 0;
                 k < primitive.attributes_count; ++k)
            {
                const cgltf_attribute& attribute =
                    primitive.attributes[k];

                if (attribute.type == cgltf_attribute_type_position)
                    positions = attribute.data;

                if (attribute.type == cgltf_attribute_type_normal)
                    normals = attribute.data;

                if (attribute.type == cgltf_attribute_type_texcoord && attribute.index==0)
                    texCoords = attribute.data;
                if (attribute.type == cgltf_attribute_type_joints) joints=attribute.data;
                if (attribute.type == cgltf_attribute_type_weights) weights=attribute.data;
                if (attribute.type == cgltf_attribute_type_color && attribute.index==0) colors=attribute.data;
            }

            if (!positions)
                continue;

            std::vector<float> vertices;
            std::vector<float> vertexColors;
            std::vector<unsigned int> indices;
            AnimationState::Primitive animated;
            animated.skin=skin;

            // Read vertices and apply node transformation.
            for (cgltf_size v = 0; v < positions->count; ++v)
            {
                float position[3] = {};
                float normal[3] = {0.0f, 1.0f, 0.0f};
                float uv[2] = {};
                if(colors) {
                    float color[4]={1,1,1,1};
                    cgltf_accessor_read_float(colors,v,color,cgltf_num_components(colors->type));
                    vertexColors.insert(vertexColors.end(),color,color+4);
                }

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
                if (animation && skin && joints && weights) {
                    cgltf_uint j[4]{};float w[4]{};
                    cgltf_accessor_read_uint(joints,v,j,4);
                    cgltf_accessor_read_float(weights,v,w,4);
                    animated.joints.emplace_back(j[0],j[1],j[2],j[3]);
                    animated.weights.emplace_back(w[0],w[1],w[2],w[3]);
                    animated.vertices.insert(animated.vertices.end(),{position[0],position[1],position[2],normal[0],normal[1],normal[2],uv[0],uv[1]});
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

            if(glm::determinant(glm::make_mat4(worldMatrix))<0)
                for(size_t i=0;i+2<indices.size();i+=3) std::swap(indices[i+1],indices[i+2]);
            if (vertices.empty() || indices.empty())
                continue;
            if(!normals) {
                // glTF specifies flat face normals when NORMAL is absent. Split
                // shared vertices rather than smoothing unrelated stone faces.
                std::vector<float> flat,flatColors;std::vector<unsigned int> flatIndices;
                for(size_t i=0;i+2<indices.size();i+=3) {
                    const auto a=indices[i],b=indices[i+1],c=indices[i+2];
                    const auto cross=glm::cross(glm::make_vec3(&vertices[b*8])-glm::make_vec3(&vertices[a*8]),
                                               glm::make_vec3(&vertices[c*8])-glm::make_vec3(&vertices[a*8]));
                    const auto n=glm::length(cross)>.000001f?glm::normalize(cross):glm::vec3(0,1,0);
                    for(int k=0;k<3;++k) {
                        const auto index=indices[i+k];
                        flatIndices.push_back(static_cast<unsigned int>(flat.size()/8));
                        flat.insert(flat.end(),vertices.begin()+index*8,vertices.begin()+index*8+8);
                        std::copy(glm::value_ptr(n),glm::value_ptr(n)+3,flat.end()-5);
                        if(!vertexColors.empty()) flatColors.insert(flatColors.end(),vertexColors.begin()+index*4,vertexColors.begin()+index*4+4);
                    }
                }
                vertices=std::move(flat);indices=std::move(flatIndices);vertexColors=std::move(flatColors);
            }

            Mesh mesh;
            float largestArea=0;
            for(size_t t=0;t+2<indices.size();t+=3) {
                const auto a=glm::make_vec3(&vertices[indices[t]*8]);
                const auto b=glm::make_vec3(&vertices[indices[t+1]*8]);
                const auto c=glm::make_vec3(&vertices[indices[t+2]*8]);
                const auto cross=glm::cross(b-a,c-a);const float area=glm::length(cross);
                if(area>largestArea) {largestArea=area;mesh.normal=cross/area;}
            }
            if(mesh.normal.z<0) mesh.normal=-mesh.normal;
            mesh.name=sourceMesh->name?sourceMesh->name:"unnamed";
            for(size_t v=0;v<vertices.size();v+=8) {
                const auto p=glm::make_vec3(&vertices[v]);
                mesh.minBounds=glm::min(mesh.minBounds,p);mesh.maxBounds=glm::max(mesh.maxBounds,p);
            }
            if (primitive.material) {
                const auto* c = primitive.material->pbr_metallic_roughness.base_color_factor;
                mesh.baseColor = glm::vec4(c[0],c[1],c[2],c[3]);
                mesh.roughness=primitive.material->pbr_metallic_roughness.roughness_factor;
                mesh.metallic=primitive.material->pbr_metallic_roughness.metallic_factor;
                mesh.emissive=glm::make_vec3(primitive.material->emissive_factor);
                mesh.alphaMode=static_cast<int>(primitive.material->alpha_mode);
                mesh.alphaCutoff=primitive.material->alpha_cutoff;
                mesh.doubleSided=primitive.material->double_sided;
            }

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
            if(!vertexColors.empty()) {
                mesh.hasVertexColor=true;
                glGenBuffers(1,&mesh.colorVBO);glBindBuffer(GL_ARRAY_BUFFER,mesh.colorVBO);
                glBufferData(GL_ARRAY_BUFFER,vertexColors.size()*sizeof(float),vertexColors.data(),GL_STATIC_DRAW);
                glVertexAttribPointer(3,4,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);
                glEnableVertexAttribArray(3);
            }

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

                auto found=textureCache.find(texture);
                if(found!=textureCache.end()) mesh.textureID=found->second;
                else {mesh.textureID=loadTexture(texture,modelDirectory);textureCache[texture]=mesh.textureID;}

                mesh.hasTexture = mesh.textureID != 0;
            }

            meshes.push_back(mesh);
            if (animation) animation->primitives.push_back(std::move(animated));
        }
    };

    // Only traverse nodes in the selected glTF scene, preserving instancing and
    // parent transforms. Unreferenced library meshes are not visible scene objects.
    std::function<void(const cgltf_node*)> visit=[&](const cgltf_node* node) {
        if(node->mesh) {
            cgltf_float worldMatrix[16];cgltf_node_transform_world(node,worldMatrix);
            processMesh(node->mesh,worldMatrix,node->skin);
        }
        for(size_t i=0;i<node->children_count;++i) visit(node->children[i]);
    };
    if(data->scene) for(size_t i=0;i<data->scene->nodes_count;++i) visit(data->scene->nodes[i]);
    else for(size_t i=0;i<data->nodes_count;++i) if(!data->nodes[i].parent) visit(&data->nodes[i]);

    if (!animation) cgltf_free(data);

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

    std::vector<unsigned int> translucent;
    for(unsigned int i=0;i<meshes.size();++i) {
        if(meshes[i].alphaMode==cgltf_alpha_mode_blend) translucent.push_back(i);
        else drawMesh(i,shaderProgram);
    }
    if(!translucent.empty()) {
        glm::mat4 model(1),view(1);
        const auto m=glGetUniformLocation(shaderProgram,"model"),v=glGetUniformLocation(shaderProgram,"view");
        if(m>=0)glGetUniformfv(shaderProgram,m,glm::value_ptr(model));
        if(v>=0)glGetUniformfv(shaderProgram,v,glm::value_ptr(view));
        std::sort(translucent.begin(),translucent.end(),[&](unsigned int a,unsigned int b) {
            return (view*model*glm::vec4(meshCenter(a),1)).z<(view*model*glm::vec4(meshCenter(b),1)).z;
        });
        for(auto i:translucent) drawMesh(i,shaderProgram);
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
    glUniform3fv(glGetUniformLocation(shaderProgram,"uMaterialEmissive"),1,glm::value_ptr(mesh.emissive));
    glUniform1f(glGetUniformLocation(shaderProgram,"uRoughness"),mesh.roughness);
    glUniform1f(glGetUniformLocation(shaderProgram,"uMetallic"),mesh.metallic);
    glUniform1i(glGetUniformLocation(shaderProgram,"uAlphaMode"),mesh.alphaMode);
    glUniform1f(glGetUniformLocation(shaderProgram,"uAlphaCutoff"),mesh.alphaCutoff);
    if(mesh.doubleSided) glDisable(GL_CULL_FACE);else {glEnable(GL_CULL_FACE);glCullFace(GL_BACK);}
    if(mesh.alphaMode==cgltf_alpha_mode_blend) {
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    }
    const GLint vertexColorLocation=glGetUniformLocation(shaderProgram,"uHasVertexColor");
    if(vertexColorLocation!=-1) glUniform1i(vertexColorLocation,mesh.hasVertexColor);
    const GLint materialLocation = glGetUniformLocation(shaderProgram, "uMaterialColor");
    if (materialLocation != -1)
        glUniform4f(materialLocation,mesh.baseColor.r,mesh.baseColor.g,mesh.baseColor.b,mesh.baseColor.a);

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
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
}

// Return the number of meshes in the model.
unsigned int Model::getMeshCount() const
{
    return static_cast<unsigned int>(meshes.size());
}
glm::vec3 Model::meshCenter(unsigned int i) const {
    return (meshes.at(i).minBounds+meshes.at(i).maxBounds)*.5f;
}
glm::vec3 Model::meshSize(unsigned int i) const {
    return meshes.at(i).maxBounds-meshes.at(i).minBounds;
}

const std::string& Model::meshName(unsigned int i) const { return meshes.at(i).name; }
int Model::findMesh(const std::string& name) const {
    for(size_t i=0;i<meshes.size();++i) if(meshes[i].name==name) return static_cast<int>(i);
    return -1;
}
glm::vec3 Model::meshNormal(unsigned int i) const {return meshes.at(i).normal;}

bool Model::loadAnimations(const std::string& idleFile,const std::string& walkFile)
{
    if (!animation) return false;
    auto loadClip=[&](const std::string& path,const char* name,cgltf_data*& data,const cgltf_animation*& clip) {
        cgltf_options options{};
        if(cgltf_parse_file(&options,path.c_str(),&data)!=cgltf_result_success) return false;
        if(cgltf_load_buffers(&options,data,path.c_str())!=cgltf_result_success || cgltf_validate(data)!=cgltf_result_success) return false;
        for(size_t i=0;i<data->animations_count;++i) if(data->animations[i].name && std::strcmp(data->animations[i].name,name)==0) clip=&data->animations[i];
        if(!clip) return false;
        // All animation targets must have a matching Ranger node. Reject incompatible rigs.
        for(size_t i=0;i<clip->channels_count;++i) {
            const auto& ch=clip->channels[i];bool found=false;
            if(!ch.target_node || !ch.target_node->name) return false;
            if(ch.sampler->interpolation!=cgltf_interpolation_type_linear && ch.sampler->interpolation!=cgltf_interpolation_type_step) return false;
            for(size_t n=0;n<animation->model->nodes_count;++n)
                if(animation->model->nodes[n].name && std::strcmp(animation->model->nodes[n].name,ch.target_node->name)==0) found=true;
            if(!found) return false;
        }
        return true;
    };
    const bool ok=loadClip(idleFile,"Idle_A",animation->idle,animation->idleClip) && loadClip(walkFile,"Walking_A",animation->walk,animation->walkClip);
    std::cout << (ok?"Ranger idle/walk rigs compatible\n":"Ranger animation incompatible; static fallback\n");
    return ok;
}

bool Model::animate(float seconds,bool walking)
{
    if(!animation || !animation->idleClip || !animation->walkClip) return false;
    const auto* clip=walking?animation->walkClip:animation->idleClip;
    // Restore bind pose before each sample so switching clips never leaves stale joints.
    for(size_t n=0;n<animation->rest.size();++n) {
        auto& node=animation->model->nodes[n];const auto& rest=animation->rest[n];
        std::copy(rest.translation,rest.translation+3,node.translation);
        std::copy(rest.rotation,rest.rotation+4,node.rotation);
        std::copy(rest.scale,rest.scale+3,node.scale);
    }
    float duration=0;
    for(size_t i=0;i<clip->samplers_count;++i) {
        const auto* input=clip->samplers[i].input;
        float end=0;if(input->count) cgltf_accessor_read_float(input,input->count-1,&end,1);
        duration=std::max(duration,end);
    }
    const float t=duration>0?std::fmod(std::max(seconds,0.f),duration):0;
    for(size_t c=0;c<clip->channels_count;++c) {
        const auto& channel=clip->channels[c];const auto& sampler=*channel.sampler;
        cgltf_node* target=nullptr;
        for(size_t n=0;n<animation->model->nodes_count;++n) {
            auto& node=animation->model->nodes[n];
            if(node.name && std::strcmp(node.name,channel.target_node->name)==0) {target=&node;break;}
        }
        if(!target || !sampler.input->count) continue;
        size_t a=0;float ta=0,tb=0;
        for(size_t k=1;k<sampler.input->count;++k) {
            float time;cgltf_accessor_read_float(sampler.input,k,&time,1);
            if(time>t) break;
            a=k;
        }
        const size_t b=std::min(a+1,sampler.input->count-1);
        cgltf_accessor_read_float(sampler.input,a,&ta,1);cgltf_accessor_read_float(sampler.input,b,&tb,1);
        const float blend=sampler.interpolation==cgltf_interpolation_type_step || tb<=ta?0:std::clamp((t-ta)/(tb-ta),0.f,1.f);
        glm::vec4 va(0),vb(0);
        const int count=channel.target_path==cgltf_animation_path_type_rotation?4:3;
        cgltf_accessor_read_float(sampler.output,a,glm::value_ptr(va),count);
        cgltf_accessor_read_float(sampler.output,b,glm::value_ptr(vb),count);
        if(channel.target_path==cgltf_animation_path_type_rotation) {
            const auto q=glm::normalize(glm::slerp(glm::quat(va.w,va.x,va.y,va.z),glm::quat(vb.w,vb.x,vb.y,vb.z),blend));
            target->rotation[0]=q.x;target->rotation[1]=q.y;target->rotation[2]=q.z;target->rotation[3]=q.w;
            target->has_rotation=1;
        } else {
            const auto v=glm::mix(va,vb,blend);
            if(channel.target_path==cgltf_animation_path_type_translation) {
                std::copy(glm::value_ptr(v),glm::value_ptr(v)+3,target->translation);target->has_translation=1;
            } else if(channel.target_path==cgltf_animation_path_type_scale) {
                std::copy(glm::value_ptr(v),glm::value_ptr(v)+3,target->scale);target->has_scale=1;
            }
        }
    }
    for(size_t i=0;i<animation->primitives.size();++i) {
        const auto& primitive=animation->primitives[i];
        if(!primitive.skin || primitive.joints.empty()) continue;
        std::vector<glm::mat4> matrices;
        for(size_t j=0;j<primitive.skin->joints_count;++j) {
            float world[16];cgltf_node_transform_world(primitive.skin->joints[j],world);
            glm::mat4 inverse(1);
            if(primitive.skin->inverse_bind_matrices) cgltf_accessor_read_float(primitive.skin->inverse_bind_matrices,j,glm::value_ptr(inverse),16);
            matrices.push_back(glm::make_mat4(world)*inverse);
        }
        auto vertices=primitive.vertices;
        for(size_t v=0;v<primitive.joints.size();++v) {
            glm::mat4 skin(0);float total=0;
            for(int k=0;k<4;++k) {
                if(primitive.joints[v][k]>=matrices.size()) return false;
                skin+=matrices[primitive.joints[v][k]]*primitive.weights[v][k];total+=primitive.weights[v][k];
            }
            if(total<.001f) continue;
            skin/=total;
            const auto pos=skin*glm::vec4(vertices[v*8],vertices[v*8+1],vertices[v*8+2],1);
            const auto normal=glm::normalize(glm::transpose(glm::inverse(glm::mat3(skin)))*glm::vec3(vertices[v*8+3],vertices[v*8+4],vertices[v*8+5]));
            if(!std::isfinite(pos.x)||!std::isfinite(pos.y)||!std::isfinite(pos.z)) return false;
            vertices[v*8]=pos.x;vertices[v*8+1]=pos.y;vertices[v*8+2]=pos.z;
            vertices[v*8+3]=normal.x;vertices[v*8+4]=normal.y;vertices[v*8+5]=normal.z;
        }
        glBindBuffer(GL_ARRAY_BUFFER,meshes[i].VBO);
        glBufferSubData(GL_ARRAY_BUFFER,0,vertices.size()*sizeof(float),vertices.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER,0);
    return true;
}

void Model::cleanup()
{
    delete animation;animation=nullptr;
    std::unordered_set<unsigned int> deletedTextures;
    for (Mesh& mesh : meshes)
    {
        if(mesh.colorVBO) glDeleteBuffers(1,&mesh.colorVBO);
        if (mesh.EBO)
            glDeleteBuffers(1, &mesh.EBO);

        if (mesh.VBO)
            glDeleteBuffers(1, &mesh.VBO);

        if (mesh.VAO)
            glDeleteVertexArrays(1, &mesh.VAO);

        if (mesh.textureID && deletedTextures.insert(mesh.textureID).second)
            glDeleteTextures(1, &mesh.textureID);
    }

    meshes.clear();
    loaded = false;
}
