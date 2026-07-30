/**
 * sw/main.cpp — Ponto de entrada do Software Microkernel
 */

#include "../include/baios.h"
#include "../include/sw_kernel.h"
#include "../include/ipc.h"
#include "../include/permissions.h"

#include <cstdio>
#include <cstring>

/* Declarados nos outros arquivos */
extern "C" {
    baios_error_t fs_init(void);
    void          fs_shutdown(void);
    baios_error_t service_audio_init(void);
    void          service_audio_shutdown(void);
    baios_error_t service_network_init(void);
    void          service_network_shutdown(void);
    baios_error_t service_render_init(void);
    void          service_render_shutdown(void);
    void          sw_ipc_poll(void);
}

static baios_sw_state_t *g_sw_ptr = nullptr;

extern "C" baios_error_t sw_kernel_init(void);
extern "C" void          sw_kernel_shutdown(void);
extern "C" void          sw_scheduler_tick(void);

extern "C" void sw_kernel_main_loop(void) {
    std::printf("[SW] Entrando no loop principal...\n");

    for (int i = 0; i < 10; i++) {
        sw_scheduler_tick();
        sw_ipc_poll();
    }

    std::printf("[SW] Loop de demonstração finalizado\n");
}

/* Syscall dispatcher simplificado */
extern "C" baios_error_t sw_syscall(u32 number, u64 arg0, u64 arg1, u64 arg2,
                                    u64 arg3, u64 arg4, u64 *out_result) {
    (void)arg3; (void)arg4;
    if (out_result) *out_result = 0;

    switch (number) {
        case BAIOS_SYS_GETPID: {
            baios_process_t *cur = sw_process_current();
            if (out_result && cur) *out_result = cur->pid;
            return BAIOS_OK;
        }
        case BAIOS_SYS_YIELD:
            return sw_scheduler_yield();
        case BAIOS_SYS_EXIT:
            if (sw_process_current()) {
                return sw_process_kill(sw_process_current()->pid, 0);
            }
            return BAIOS_ERR_INVALID_ARG;
        default:
            return BAIOS_ERR_NOT_SUPPORTED;
    }
}

/* Função de conveniência para inicializar tudo do lado SW */
extern "C" baios_error_t sw_full_init(void) {
    baios_error_t err;

    err = perm_init();
    if (err != BAIOS_OK) return err;

    err = sw_kernel_init();
    if (err != BAIOS_OK) return err;

    err = fs_init();
    if (err != BAIOS_OK) return err;

    err = service_audio_init();
    if (err != BAIOS_OK) return err;

    err = service_network_init();
    if (err != BAIOS_OK) return err;

    err = service_render_init();
    if (err != BAIOS_OK) return err;

    return BAIOS_OK;
}

extern "C" void sw_full_shutdown(void) {
    service_render_shutdown();
    service_network_shutdown();
    service_audio_shutdown();
    fs_shutdown();
    sw_kernel_shutdown();
}
