/**
 * services/render.cpp — Serviço de renderização / compositor
 */

#include "../../include/sw_kernel.h"
#include "../../include/types.h"

#include <cstdio>
#include <cstring>

namespace {

struct RenderState {
    bool initialized;
    u32  width;
    u32  height;
    u32  bpp;
    u64  frames;
};

static RenderState g_render = {false, 1080, 2400, 32, 0}; /* celular moderno */

} // namespace

extern "C" {

baios_error_t service_render_init(void) {
    g_render.initialized = true;
    g_render.width  = 1080;
    g_render.height = 2400;
    g_render.bpp    = 32;
    g_render.frames = 0;
    std::printf("[SW] Serviço de render inicializado (%ux%u @%ubpp)\n",
                g_render.width, g_render.height, g_render.bpp);
    return BAIOS_OK;
}

baios_error_t service_render_present(const void *framebuffer, baios_size_t size) {
    if (!g_render.initialized) return BAIOS_ERR_INVALID_ARG;
    if (!framebuffer || size == 0) return BAIOS_ERR_INVALID_ARG;

    /* Em produção: enviaria o buffer via Zero-Copy para o HW/GPU */
    g_render.frames++;
    return BAIOS_OK;
}

baios_error_t service_render_get_info(u32 *w, u32 *h, u32 *bpp) {
    if (!g_render.initialized) return BAIOS_ERR_INVALID_ARG;
    if (w) *w = g_render.width;
    if (h) *h = g_render.height;
    if (bpp) *bpp = g_render.bpp;
    return BAIOS_OK;
}

void service_render_shutdown(void) {
    g_render.initialized = false;
}

} // extern "C"
