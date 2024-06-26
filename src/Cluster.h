//
// Created by admin on 2024/4/29.
//

#ifndef EMPTYDEMO_CLUSTER_H
#define EMPTYDEMO_CLUSTER_H

#include <big2.h>

#if BIG2_IMGUI_ENABLED
#include <imgui.h>
#endif // BIG2_IMGUI_ENABLED

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <GLFW/glfw3.h>
#include <big2/bgfx/embedded_shader.h>
#include <generated/shaders/src/all.h>

#include <iostream>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_operation.hpp>
#include <bx/file.h>
#include <bimg/decode.h>
#include <algorithm>
#include <filesystem>

#include "Log/Log.h"


struct Mesh
{
    bgfx::VertexBufferHandle vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle indexBuffer = BGFX_INVALID_HANDLE;
    unsigned int material = 0; // index into materials vector

    //bgfx::OcclusionQueryHandle occlusionQuery = BGFX_INVALID_HANDLE;

    // bgfx vertex attributes
    // initialized by Scene
    struct PosNormalTangentTex0Vertex
    {
        float x, y, z;    // position
        uint32_t color;   // color
        float nx, ny, nz; // normal
        float tx, ty, tz; // tangent
        float u, v;       // UV coordinates

        static void init()
        {
            layout.begin()
                    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
                    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                    .end();
        }
        static bgfx::VertexLayout layout;
    };
};

bgfx::VertexLayout Mesh::PosNormalTangentTex0Vertex::layout;


Mesh loadMesh(const aiMesh* mesh)
{
    Mesh::PosNormalTangentTex0Vertex::init();

    if(mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
        throw std::runtime_error("Mesh has incompatible primitive type");

    if(mesh->mNumVertices > (std::numeric_limits<uint16_t>::max() + 1u))
        throw std::runtime_error("Mesh has too many vertices (> uint16_t::max + 1)");

    constexpr size_t coords = 0;
    bool hasTexture = mesh->mNumUVComponents[coords] == 2 && mesh->mTextureCoords[coords] != nullptr;

    // vertices

    uint32_t stride = Mesh::PosNormalTangentTex0Vertex::layout.getStride();

    const bgfx::Memory* vertexMem = bgfx::alloc(mesh->mNumVertices * stride);

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        unsigned int offset = i * stride;
        Mesh::PosNormalTangentTex0Vertex& vertex = *(Mesh::PosNormalTangentTex0Vertex*)(vertexMem->data + offset);

        aiVector3D pos = mesh->mVertices[i];

        vertex.x = pos.x / 500.0f;
        vertex.y = pos.y / 500.0f;
        vertex.z = pos.z / 500.0f;

        std::cout << pos.x << ", " << pos.y << ", "<< pos.z << std::endl;

        aiVector3D nrm = mesh->mNormals[i];
        vertex.nx = nrm.x;
        vertex.ny = nrm.y;
        vertex.nz = nrm.z;

        aiVector3D tan = mesh->mTangents[i];
        vertex.tx = tan.x;
        vertex.ty = tan.y;
        vertex.tz = tan.z;

        if(hasTexture)
        {
            aiVector3D uv = mesh->mTextureCoords[coords][i];
            vertex.u = uv.x;
            vertex.v = uv.y;
        }
        // else
        {
            vertex.color = 0xFFFFFFFF;
        }
    }

    bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(vertexMem, Mesh::PosNormalTangentTex0Vertex::layout);

    // indices (triangles)

    const bgfx::Memory* iMem = bgfx::alloc(mesh->mNumFaces * 3 * sizeof(uint16_t));
    uint16_t* indices = (uint16_t*)iMem->data;

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        assert(mesh->mFaces[i].mNumIndices == 3);
        indices[(3 * i) + 0] = (uint16_t)mesh->mFaces[i].mIndices[0];
        indices[(3 * i) + 1] = (uint16_t)mesh->mFaces[i].mIndices[1];
        indices[(3 * i) + 2] = (uint16_t)mesh->mFaces[i].mIndices[2];
    }

    bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(iMem);

    return { vbh, ibh, mesh->mMaterialIndex };
}


bool fileExists(const std::string& name) {
    return std::filesystem::exists(name);
}

std::vector<Mesh> loadMeshFromFile(const char* file) {

    std::vector<Mesh> meshes;

    if( !fileExists(file)){
        std::cout << "Error, " << file << " not exists!" <<std::endl;
        return meshes;
    }

    Assimp::Importer importer;

    // Settings for aiProcess_SortByPType
    // only take triangles or higher (polygons are triangulated during import)
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    // Settings for aiProcess_SplitLargeMeshes
    // Limit vertices to 65k (we use 16-bit indices)
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, std::numeric_limits<uint16_t>::max());

    unsigned int flags =
            aiProcessPreset_TargetRealtime_Quality |                     // some optimizations and safety checks
            aiProcess_OptimizeMeshes |                                   // minimize number of meshes
            aiProcess_PreTransformVertices |                             // apply node matrices
            aiProcess_FixInfacingNormals | aiProcess_TransformUVCoords | // apply UV transformations
            //aiProcess_FlipWindingOrder   | // we cull clock-wise, keep the default CCW winding order
            aiProcess_MakeLeftHanded | // we set GLM_FORCE_LEFT_HANDED and use left-handed bx matrix functions
            aiProcess_FlipUVs;         // bimg loads textures with flipped Y (top left is 0,0)

    const aiScene* scene = nullptr;
    try
    {
        scene = importer.ReadFile(file, flags);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }



    if (!scene) { return meshes; }

    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        meshes.push_back(loadMesh(scene->mMeshes[i]));
    }

    return meshes;
}

bgfx::UniformHandle dUniform = BGFX_INVALID_HANDLE;

void demoSetUniform(const glm::mat4& modelMat)
{
    glm::mat3 normalMat = glm::transpose(glm::adjugate(glm::mat3(modelMat)));
    bgfx::setUniform(dUniform, glm::value_ptr(normalMat));
}

static const bgfx::EmbeddedShader kEmbeddedShaders[] =
        {
                BGFX_EMBEDDED_SHADER(vs_basic),
                BGFX_EMBEDDED_SHADER(fs_basic),
                BGFX_EMBEDDED_SHADER_END()
        };

class Cluster  final : public big2::AppExtensionBase {
protected:
    void OnFrameBegin() override {
        AppExtensionBase::OnFrameBegin();
    }
    void OnRender(big2::Window &window) override {
        AppExtensionBase::OnRender(window);
        bgfx::setState(
                BGFX_STATE_WRITE_R
                | BGFX_STATE_WRITE_G
                | BGFX_STATE_WRITE_B
                | BGFX_STATE_WRITE_A
        );

        for (const Mesh& mesh : sceneMeshes)
        {
            glm::mat4 model = glm::identity<glm::mat4>();
            bgfx::setTransform(glm::value_ptr(model));
            demoSetUniform(model);
            bgfx::setVertexBuffer(0, mesh.vertexBuffer);
            bgfx::setIndexBuffer(mesh.indexBuffer);
            //const Material& mat = scene->materials[mesh.material];
            //uint64_t materialState = pbr.bindMaterial(mat);
            //bgfx::setState(state | materialState);
            bgfx::submit(window.GetView(), program_, 0, ~BGFX_DISCARD_BINDINGS);
        }

        bgfx::discard(BGFX_DISCARD_ALL);


#if BIG2_IMGUI_ENABLED
        BIG2_SCOPE_VAR(big2::ImGuiFrameScoped) {
        ImGui::ShowDemoWindow();
      }
#endif // BIG2_IMGUI_ENABLED
    }
    void OnFrameEnd() override {
        AppExtensionBase::OnFrameEnd();
    }
    void OnInitialize() override {
        AppExtensionBase::OnInitialize();

        bgfx::RendererType::Enum renderer_type = bgfx::getRendererType();
        program_ = bgfx::createProgram(
                bgfx::createEmbeddedShader(kEmbeddedShaders, renderer_type, "vs_basic"),
                bgfx::createEmbeddedShader(kEmbeddedShaders, renderer_type, "fs_basic"),
                true
        );

        sceneMeshes = loadMeshFromFile("E:\\DigitalAssetsCreateTool\\learn-bgfx\\assets\\models\\cube-1mx1m.fbx");

        dUniform = bgfx::createUniform("normMat", bgfx::UniformType::Mat3 );
    }

    void OnTerminate() override {
        AppExtensionBase::OnTerminate();
        program_.Destroy();

        for (Mesh& mesh : sceneMeshes)
        {
            bgfx::destroy(mesh.vertexBuffer);
            bgfx::destroy(mesh.indexBuffer);
            mesh.vertexBuffer = BGFX_INVALID_HANDLE;
            mesh.indexBuffer = BGFX_INVALID_HANDLE;
        }
        sceneMeshes.clear();
        bgfx::destroy(dUniform);
    }

private:
    big2::ProgramScopedHandle program_;

    std::vector<Mesh> sceneMeshes;
};



#endif //EMPTYDEMO_CLUSTER_H
