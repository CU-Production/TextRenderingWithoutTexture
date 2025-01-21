#define SOKOL_IMPL
#define SOKOL_GLCORE
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h""

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static void init(void) {
    sg_desc _sg_desc{};
    _sg_desc.environment = sglue_environment();
    _sg_desc.logger.func = slog_func;
    sg_setup(&_sg_desc);

    float vertices[] = {
        0.0f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
       -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f
   };
    sg_buffer_desc _sg_buffer_desc{};
    _sg_buffer_desc.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&_sg_buffer_desc);

    sg_shader_desc _sg_shader_desc{};
    _sg_shader_desc.vertex_func.source = R"(
#version 430 core
#extension GL_ARB_shading_language_420pack : enable
layout(location=0) in vec4 position;
layout(location=1) in vec4 color0;
out vec4 color;
void main() {
  gl_Position = position;
  color = color0;
};
)";
    _sg_shader_desc.fragment_func.source = R"(
#version 430 core
#extension GL_ARB_shading_language_420pack : enable
in vec4 color;
out vec4 frag_color;
void main() {
  frag_color = color;
}
)";

    sg_shader shd = sg_make_shader(&_sg_shader_desc);

    sg_pipeline_desc _sg_pipeline_desc{};
    _sg_pipeline_desc.shader = shd,
    _sg_pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    _sg_pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    state.pip = sg_make_pipeline(&_sg_pipeline_desc);

    state.pass_action .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.2f, 0.3f, 0.3f, 1.0f } };
}

void frame(void) {
    sg_pass _sg_pass = { .action = state.pass_action, .swapchain = sglue_swapchain() };
    sg_begin_pass(&_sg_pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_draw(0, 3, 1);
    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    sapp_desc _sapp_desc{};
    _sapp_desc.init_cb = init;
    _sapp_desc.frame_cb = frame;
    _sapp_desc.cleanup_cb = cleanup;
    _sapp_desc.width = 640;
    _sapp_desc.height = 480;
    _sapp_desc.window_title = "Triangle";
    _sapp_desc.logger.func = slog_func;
    return _sapp_desc;
}