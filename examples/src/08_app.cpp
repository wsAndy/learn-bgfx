//
// Copyright (c) 2023 Paper Cranes Ltd.
// All rights reserved.
//

#include <big2.h>

#if BIG2_IMGUI_ENABLED
#include <imgui.h>
#endif // BIG2_IMGUI_ENABLED

#include <bgfx/bgfx.h>
#include <bx/bx.h>

#include <GLFW/glfw3.h>
#include <big2/bgfx/embedded_shader.h>
#include <generated/shaders/examples/all.h>

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
        float nx, ny, nz; // normal
        float tx, ty, tz; // tangent
        float u, v;       // UV coordinates

        static void init()
        {
            layout.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
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
        vertex.x = pos.x;
        vertex.y = pos.y;
        vertex.z = pos.z;

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

std::vector<Mesh> loadMeshFromFile(const char* file) {

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
    
    std::vector<Mesh> meshes;


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
    // usually the normal matrix is based on the model view matrix
    // but shading is done in world space (not eye space) so it's just the model matrix
    //glm::mat4 modelViewMat = viewMat * modelMat;

    // if we don't do non-uniform scaling, the normal matrix is the same as the model-view matrix
    // (only the magnitude of the normal is changed, but we normalize either way)
    //glm::mat3 normalMat = glm::mat3(modelMat);

    // use adjugate instead of inverse
    // see https://github.com/graphitemaster/normals_revisited#the-details-of-transforming-normals
    // cofactor is the transpose of the adjugate
    glm::mat3 normalMat = glm::transpose(glm::adjugate(glm::mat3(modelMat)));
    bgfx::setUniform(dUniform, glm::value_ptr(normalMat));
}

static const bgfx::EmbeddedShader kEmbeddedShaders[] =
{
  BGFX_EMBEDDED_SHADER(vs_basic),
  BGFX_EMBEDDED_SHADER(fs_basic),
  BGFX_EMBEDDED_SHADER_END()
};

struct NormalColorVertex {
  glm::vec2 position;
  uint32_t color;
};

NormalColorVertex kTriangleVertices[] =
{
  {{-0.5f, -0.5f}, 0x339933FF},
  {{0.5f, -0.5f}, 0x993333FF},
  {{0.0f, 0.5f}, 0x333399FF},
};

const uint16_t kTriangleIndices[] =
{
  0, 1, 2,
};

class TriangleRenderAppExtension final : public big2::AppExtensionBase {
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

      bgfx::setVertexBuffer(0, vertex_buffer_);
      bgfx::setIndexBuffer(index_buffer_);
      bgfx::submit(window.GetView(), program_);


      //for (const Mesh& mesh : sceneMeshes)
      //{
      //    glm::mat4 model = glm::identity<glm::mat4>();
      //    bgfx::setTransform(glm::value_ptr(model));
      //    demoSetUniform(model);
      //    bgfx::setVertexBuffer(0, mesh.vertexBuffer);
      //    bgfx::setIndexBuffer(mesh.indexBuffer);
      //     //const Material& mat = scene->materials[mesh.material];
      //     //uint64_t materialState = pbr.bindMaterial(mat);
      //     //bgfx::setState(state | materialState);
      //    bgfx::submit(window.GetView(), program_, 0, ~BGFX_DISCARD_BINDINGS);
      //}

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

      bgfx::VertexLayout color_vertex_layout;
      color_vertex_layout.begin()
          .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
          .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
          .end();

      vertex_buffer_ = bgfx::createVertexBuffer(bgfx::makeRef(kTriangleVertices, sizeof(kTriangleVertices)), color_vertex_layout);
      index_buffer_ = bgfx::createIndexBuffer(bgfx::makeRef(kTriangleIndices, sizeof(kTriangleIndices)));

      bgfx::RendererType::Enum renderer_type = bgfx::getRendererType();
      program_ = bgfx::createProgram(
        bgfx::createEmbeddedShader(kEmbeddedShaders, renderer_type, "vs_basic"),
        bgfx::createEmbeddedShader(kEmbeddedShaders, renderer_type, "fs_basic"),
        true
      );

      // TODO:
      sceneMeshes = loadMeshFromFile("D:/assets/suzanne.fbx");

      dUniform = bgfx::createUniform("normMat", bgfx::UniformType::Mat3 );
    }

    void OnTerminate() override {
      AppExtensionBase::OnTerminate();
      vertex_buffer_.Destroy();
      index_buffer_.Destroy();
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
    big2::VertexBufferScopedHandle vertex_buffer_;
    big2::IndexBufferScopedHandle index_buffer_;
    big2::ProgramScopedHandle program_;

    std::vector<Mesh> sceneMeshes;
};

int main(std::int32_t, gsl::zstring []) {
  big2::App app;

  app.AddExtension<big2::DefaultQuitConditionAppExtension>();
#if BIG2_IMGUI_ENABLED
  app.AddExtension<big2::ImGuiAppExtension>();
#endif // BIG2_IMGUI_ENABLED
  app.AddExtension<TriangleRenderAppExtension>();

  app.CreateWindow("My App", {800, 600});
  app.Run();

  return 0;
}
