/**
 * services/audio.cpp — Serviço de áudio de alto nível
 */

#include "../../include/sw_kernel.h"
#include "../../include/types.h"
#include "../../include/permissions.h"

#include <cstdio>
#include <cstring>

namespace {

struct AudioState {
    bool initialized;
    bool playing;
    u32  sample_rate;
    u32  channels;
    u64  frames_played;
};

static AudioState g_audio = {false, false, 48000, 2, 0};

} // namespace

extern "C" {

baios_error_t service_audio_init(void) {
    g_audio.initialized = true;
    g_audio.playing = false;
    g_audio.sample_rate = 48000;
    g_audio.channels = 2;
    g_audio.frames_played = 0;
    std::printf("[SW] Serviço de áudio inicializado (48kHz stereo)\n");
    return BAIOS_OK;
}

baios_error_t service_audio_play(baios_pid_t caller, const void *pcm, baios_size_t bytes) {
    if (!g_audio.initialized) return BAIOS_ERR_INVALID_ARG;
    if (!perm_check(caller, BAIOS_CAP_NONE)) {
        /* áudio básico não exige capability especial neste esqueleto */
    }
    if (!pcm || bytes == 0) return BAIOS_ERR_INVALID_ARG;

    g_audio.playing = true;
    g_audio.frames_played += bytes / (g_audio.channels * 2); /* 16-bit */
    return BAIOS_OK;
}

baios_error_t service_audio_stop(baios_pid_t caller) {
    (void)caller;
    g_audio.playing = false;
    return BAIOS_OK;
}

baios_error_t service_audio_set_format(u32 sample_rate, u32 channels) {
    if (sample_rate < 8000 || sample_rate > 192000) return BAIOS_ERR_INVALID_ARG;
    if (channels < 1 || channels > 8) return BAIOS_ERR_INVALID_ARG;
    g_audio.sample_rate = sample_rate;
    g_audio.channels = channels;
    return BAIOS_OK;
}

void service_audio_shutdown(void) {
    g_audio.initialized = false;
    g_audio.playing = false;
}

} // extern "C"
