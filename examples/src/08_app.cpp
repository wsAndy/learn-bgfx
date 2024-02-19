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
    }

    void OnTerminate() override {
      AppExtensionBase::OnTerminate();
      vertex_buffer_.Destroy();
      index_buffer_.Destroy();
      program_.Destroy();
    }

  private:
    big2::VertexBufferScopedHandle vertex_buffer_;
    big2::IndexBufferScopedHandle index_buffer_;
    big2::ProgramScopedHandle program_;
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
