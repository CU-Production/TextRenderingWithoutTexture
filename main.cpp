#define SOKOL_IMPL
#define SOKOL_GLCORE
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_log.h"
#include "sokol/sokol_glue.h"
#include <string>


struct s_text {
    int vs_text;
    int vs_offset;
};
static s_text sb_texts[128]; // max 128 chars

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
} state;

static struct {
    float start_pos_x;
    float start_pos_y;
    float screen_resolution_w;
    float screen_resolution_h;
    float font_size;
} vs_params;

void init() {
    sg_desc _sg_desc{};
    _sg_desc.environment = sglue_environment();
    _sg_desc.logger.func = slog_func;
    sg_setup(&_sg_desc);

    if (!sg_query_features().storage_buffer) {
        assert(0, "Not support storage_buffer");
    }

    float vertices[] = {
        0.0f,  0.0f,
        1.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f,
   };
    sg_buffer_desc _sg_buffer_desc{};
    _sg_buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    _sg_buffer_desc.data = SG_RANGE(vertices);
    state.bind.vertex_buffers[0] = sg_make_buffer(&_sg_buffer_desc);

    uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
    _sg_buffer_desc = {};
    _sg_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    _sg_buffer_desc.data = SG_RANGE(indices);
    state.bind.index_buffer = sg_make_buffer(&_sg_buffer_desc);

    _sg_buffer_desc = {};
    _sg_buffer_desc.type = SG_BUFFERTYPE_STORAGEBUFFER;
    // _sg_buffer_desc.data = SG_RANGE(sb_texts);
    _sg_buffer_desc.usage = SG_USAGE_STREAM;
    _sg_buffer_desc.size = sizeof(sb_texts);
    _sg_buffer_desc.label = "ssbo_texts";
    state.bind.storage_buffers[0] = sg_make_buffer(&_sg_buffer_desc);

    _sg_buffer_desc = {};
    _sg_buffer_desc.type = SG_BUFFERTYPE_STORAGEBUFFER;
    // _sg_buffer_desc.data = SG_RANGE(vs_params);
    _sg_buffer_desc.usage = SG_USAGE_STREAM;
    _sg_buffer_desc.size = sizeof(vs_params);
    _sg_buffer_desc.label = "ssbo_pos_with_screen_res";
    state.bind.storage_buffers[1] = sg_make_buffer(&_sg_buffer_desc);

    sg_shader_desc _sg_shader_desc{};
    _sg_shader_desc.vertex_func.source = R"(
#version 430 core
#extension GL_ARB_shading_language_420pack : enable

struct sb_text {
  int text;
  int offset;
};

layout(binding=0) readonly buffer ro_texts {
  sb_text texts[];
};

layout(binding=1) readonly buffer vs_params {
  vec2 start_pos;
  vec2 screen_resolution;
  float font_size;
};

layout(location=0) in vec2 position;
layout(location=0) out vec2 out_uv;
layout(location=1) out int out_text;
void main() {
  sb_text v_text = texts[gl_InstanceID];
  vec2 font_wh = vec2(font_size, 2.0f * font_size);
  vec2 vpos = start_pos + position * font_wh;
  vpos.x += float(v_text.offset) * font_size;

  vec2 normalized_vpos = vpos / screen_resolution;

  vec2 ndc_vpos;
  ndc_vpos.x = 2.0f * normalized_vpos.x - 1.0f;
  ndc_vpos.y = -2.0f * normalized_vpos.y + 1.0f;

  gl_Position = vec4(ndc_vpos, 0.0f, 1.0f);
  out_uv = position;
  out_text = v_text.text;
};
)";
    _sg_shader_desc.fragment_func.source = R"(
#version 430 core
#extension GL_ARB_shading_language_420pack : enable
layout(location=0) in vec2 uv;
out vec4 frag_color;
void main() {
  frag_color = vec4(uv, 0.0f, 1.0f);
}
)";
    _sg_shader_desc.storage_buffers[0] = {
        .stage = SG_SHADERSTAGE_VERTEX,
        .readonly = true,
        .glsl_binding_n = 0,
    };

    _sg_shader_desc.storage_buffers[1] = {
        .stage = SG_SHADERSTAGE_VERTEX,
        .readonly = true,
        .glsl_binding_n = 1,
    };

    sg_shader shd = sg_make_shader(&_sg_shader_desc);

    sg_pipeline_desc _sg_pipeline_desc{};
    _sg_pipeline_desc.shader = shd,
    _sg_pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    _sg_pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
    state.pip = sg_make_pipeline(&_sg_pipeline_desc);

    state.pass_action .colors[0] = { .load_action=SG_LOADACTION_CLEAR, .clear_value={0.2f, 0.3f, 0.3f, 1.0f } };
}

void debug_draw_text(float start_pos_x, float start_pos_y, float font_size, std::string text) {
    if (text.size() <= 0 || text.size() > 127) return;

    for (int i = 0; i < text.size(); i++) {
        sb_texts[i].vs_text = text[i];
        sb_texts[i].vs_offset = i;
    }

    sg_range vs_ssbo_range = { .ptr = sb_texts, .size = sizeof(sb_texts) };
    sg_update_buffer(state.bind.storage_buffers[0], vs_ssbo_range);

    float screen_width = (float)sapp_width();
    float screen_height = (float)sapp_height();

    vs_params.start_pos_x = start_pos_x;
    vs_params.start_pos_y = start_pos_y;
    vs_params.screen_resolution_w = screen_width;
    vs_params.screen_resolution_h = screen_height;
    vs_params.font_size = font_size;

    sg_range vs_params_range = { .ptr = &vs_params, .size = sizeof(vs_params) };
    sg_update_buffer(state.bind.storage_buffers[1], &vs_params_range);

    sg_draw(0, 6, text.size());
}

void frame() {
    sg_pass _sg_pass = { .action = state.pass_action, .swapchain = sglue_swapchain() };
    sg_begin_pass(&_sg_pass);
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    // sg_draw(0, 6, 1);
    debug_draw_text(50.0f, 50.0f, 32.0f, "Hello World!");
    sg_end_pass();
    sg_commit();
}

void cleanup() {
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
    _sapp_desc.window_title = "Text Renderer without Texture";
    _sapp_desc.logger.func = slog_func;
    return _sapp_desc;
}