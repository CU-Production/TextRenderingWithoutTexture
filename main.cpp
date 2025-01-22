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

    sg_buffer_desc _sg_buffer_desc{};
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

const vec2 vertices[4] = {
  {0.0f, 0.0f,},
  {1.0f, 0.0f,},
  {0.0f, 1.0f,},
  {1.0f, 1.0f,},
};

const int indices[6] = { 0, 1, 2, 1, 3, 2 };

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

layout(location=0) out vec2 out_uv;
layout(location=1) out flat int out_text;
void main() {
  vec2 position = vertices[indices[gl_VertexID]];
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

// Original Font Data:
//
// http://www.fial.com/~scott/tamsyn-font/
//
// Range 0x20 to 0x7f (inclusive)
//
// Every uvec4 holds the bitmap for one 8x16 bit character.
//
const uvec4 font_data[96] = {

  { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }, // 0x20: SPACE
  { 0x00001010, 0x10101010, 0x00001010, 0x00000000 }, // 0x21: '!'
  { 0x00242424, 0x24000000, 0x00000000, 0x00000000 }, // 0x22: '\'
  { 0x00000024, 0x247E2424, 0x247E2424, 0x00000000 }, // 0x23: '#'
  { 0x00000808, 0x1E20201C, 0x02023C08, 0x08000000 }, // ...etc... ASCII code
  { 0x00000030, 0x494A3408, 0x16294906, 0x00000000 },
  { 0x00003048, 0x48483031, 0x49464639, 0x00000000 },
  { 0x00101010, 0x10000000, 0x00000000, 0x00000000 },
  { 0x00000408, 0x08101010, 0x10101008, 0x08040000 },
  { 0x00002010, 0x10080808, 0x08080810, 0x10200000 },
  { 0x00000000, 0x0024187E, 0x18240000, 0x00000000 },
  { 0x00000000, 0x0808087F, 0x08080800, 0x00000000 },
  { 0x00000000, 0x00000000, 0x00001818, 0x08081000 },
  { 0x00000000, 0x0000007E, 0x00000000, 0x00000000 },
  { 0x00000000, 0x00000000, 0x00001818, 0x00000000 },
  { 0x00000202, 0x04040808, 0x10102020, 0x40400000 },
  { 0x0000003C, 0x42464A52, 0x6242423C, 0x00000000 },
  { 0x00000008, 0x18280808, 0x0808083E, 0x00000000 },
  { 0x0000003C, 0x42020204, 0x0810207E, 0x00000000 },
  { 0x0000007E, 0x04081C02, 0x0202423C, 0x00000000 },
  { 0x00000004, 0x0C142444, 0x7E040404, 0x00000000 },
  { 0x0000007E, 0x40407C02, 0x0202423C, 0x00000000 },
  { 0x0000001C, 0x2040407C, 0x4242423C, 0x00000000 },
  { 0x0000007E, 0x02040408, 0x08101010, 0x00000000 },
  { 0x0000003C, 0x4242423C, 0x4242423C, 0x00000000 },
  { 0x0000003C, 0x4242423E, 0x02020438, 0x00000000 },
  { 0x00000000, 0x00181800, 0x00001818, 0x00000000 },
  { 0x00000000, 0x00181800, 0x00001818, 0x08081000 },
  { 0x00000004, 0x08102040, 0x20100804, 0x00000000 },
  { 0x00000000, 0x00007E00, 0x007E0000, 0x00000000 },
  { 0x00000020, 0x10080402, 0x04081020, 0x00000000 },
  { 0x00003C42, 0x02040810, 0x00001010, 0x00000000 },
  { 0x00001C22, 0x414F5151, 0x51534D40, 0x201F0000 },
  { 0x00000018, 0x24424242, 0x7E424242, 0x00000000 },
  { 0x0000007C, 0x4242427C, 0x4242427C, 0x00000000 },
  { 0x0000001E, 0x20404040, 0x4040201E, 0x00000000 },
  { 0x00000078, 0x44424242, 0x42424478, 0x00000000 },
  { 0x0000007E, 0x4040407C, 0x4040407E, 0x00000000 },
  { 0x0000007E, 0x4040407C, 0x40404040, 0x00000000 },
  { 0x0000001E, 0x20404046, 0x4242221E, 0x00000000 },
  { 0x00000042, 0x4242427E, 0x42424242, 0x00000000 },
  { 0x0000003E, 0x08080808, 0x0808083E, 0x00000000 },
  { 0x00000002, 0x02020202, 0x0242423C, 0x00000000 },
  { 0x00000042, 0x44485060, 0x50484442, 0x00000000 },
  { 0x00000040, 0x40404040, 0x4040407E, 0x00000000 },
  { 0x00000041, 0x63554949, 0x41414141, 0x00000000 },
  { 0x00000042, 0x62524A46, 0x42424242, 0x00000000 },
  { 0x0000003C, 0x42424242, 0x4242423C, 0x00000000 },
  { 0x0000007C, 0x4242427C, 0x40404040, 0x00000000 },
  { 0x0000003C, 0x42424242, 0x4242423C, 0x04020000 },
  { 0x0000007C, 0x4242427C, 0x48444242, 0x00000000 },
  { 0x0000003E, 0x40402018, 0x0402027C, 0x00000000 },
  { 0x0000007F, 0x08080808, 0x08080808, 0x00000000 },
  { 0x00000042, 0x42424242, 0x4242423C, 0x00000000 },
  { 0x00000042, 0x42424242, 0x24241818, 0x00000000 },
  { 0x00000041, 0x41414149, 0x49495563, 0x00000000 },
  { 0x00000041, 0x41221408, 0x14224141, 0x00000000 },
  { 0x00000041, 0x41221408, 0x08080808, 0x00000000 },
  { 0x0000007E, 0x04080810, 0x1020207E, 0x00000000 },
  { 0x00001E10, 0x10101010, 0x10101010, 0x101E0000 },
  { 0x00004040, 0x20201010, 0x08080404, 0x02020000 },
  { 0x00007808, 0x08080808, 0x08080808, 0x08780000 },
  { 0x00001028, 0x44000000, 0x00000000, 0x00000000 },
  { 0x00000000, 0x00000000, 0x00000000, 0x00FF0000 },
  { 0x00201008, 0x04000000, 0x00000000, 0x00000000 },
  { 0x00000000, 0x003C0202, 0x3E42423E, 0x00000000 },
  { 0x00004040, 0x407C4242, 0x4242427C, 0x00000000 },
  { 0x00000000, 0x003C4240, 0x4040423C, 0x00000000 },
  { 0x00000202, 0x023E4242, 0x4242423E, 0x00000000 },
  { 0x00000000, 0x003C4242, 0x7E40403E, 0x00000000 },
  { 0x00000E10, 0x107E1010, 0x10101010, 0x00000000 },
  { 0x00000000, 0x003E4242, 0x4242423E, 0x02023C00 },
  { 0x00004040, 0x407C4242, 0x42424242, 0x00000000 },
  { 0x00000808, 0x00380808, 0x0808083E, 0x00000000 },
  { 0x00000404, 0x001C0404, 0x04040404, 0x04043800 },
  { 0x00004040, 0x40444850, 0x70484442, 0x00000000 },
  { 0x00003808, 0x08080808, 0x0808083E, 0x00000000 },
  { 0x00000000, 0x00774949, 0x49494949, 0x00000000 },
  { 0x00000000, 0x007C4242, 0x42424242, 0x00000000 },
  { 0x00000000, 0x003C4242, 0x4242423C, 0x00000000 },
  { 0x00000000, 0x007C4242, 0x4242427C, 0x40404000 },
  { 0x00000000, 0x003E4242, 0x4242423E, 0x02020200 },
  { 0x00000000, 0x002E3020, 0x20202020, 0x00000000 },
  { 0x00000000, 0x003E4020, 0x1804027C, 0x00000000 },
  { 0x00000010, 0x107E1010, 0x1010100E, 0x00000000 },
  { 0x00000000, 0x00424242, 0x4242423E, 0x00000000 },
  { 0x00000000, 0x00424242, 0x24241818, 0x00000000 },
  { 0x00000000, 0x00414141, 0x49495563, 0x00000000 },
  { 0x00000000, 0x00412214, 0x08142241, 0x00000000 },
  { 0x00000000, 0x00424242, 0x4242423E, 0x02023C00 },
  { 0x00000000, 0x007E0408, 0x1020407E, 0x00000000 },
  { 0x000E1010, 0x101010E0, 0x10101010, 0x100E0000 },
  { 0x00080808, 0x08080808, 0x08080808, 0x08080000 },
  { 0x00700808, 0x08080807, 0x08080808, 0x08700000 },
  { 0x00003149, 0x46000000, 0x00000000, 0x00000000 },
  { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },

};

layout(location=0) in vec2 uv;
layout(location=1) in flat int text;
out vec4 frag_color;
void main() {
  int char_code = max(min(text - 32, 96), 0);

  ivec2 char_coord = ivec2(floor(uv * vec2(8, 16)));

  uint four_lines = font_data[char_code][char_coord.y/4];

  uint current_line  = (four_lines >> (8*(3-(char_coord.y)%4))) & 0xff;
  uint current_pixel = (current_line >> (7-char_coord.x)) & 0x01;

  frag_color = vec4(uv, 0.0f, 1.0f);

  frag_color = mix(frag_color, vec4(1.0f), current_pixel);
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
    _sg_pipeline_desc.shader = shd;
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
    debug_draw_text(50.0f, 50.0f, 32.0f, "Hello, World!");
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