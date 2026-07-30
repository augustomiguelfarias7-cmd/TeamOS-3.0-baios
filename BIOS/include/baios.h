/**
 * @file baios.h
 * @brief Header principal público do Dual Microkernel Baios
 */
#ifndef BAIOS_H
#define BAIOS_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Inicialização global
 * ============================================================ */

/**
 * Inicializa todo o dual microkernel.
 * Deve ser chamado uma única vez no boot.
 */
baios_status_t baios_init(void);

/**
 * Encerra o kernel de forma controlada (shutdown).
 */
baios_status_t baios_shutdown(void);

/**
 * Retorna string de versão.
 */
const char *baios_version(void);

/* ============================================================
 * Estado do kernel
 * ============================================================ */
typedef enum {
    BAIOS_STATE_UNINITIALIZED = 0,
    BAIOS_STATE_HW_INIT,
    BAIOS_STATE_SW_INIT,
    BAIOS_STATE_RUNNING,
    BAIOS_STATE_PANIC,
    BAIOS_STATE_SHUTDOWN,
} baios_state_t;

baios_state_t baios_get_state(void);

/* ============================================================
 * Logging mínimo (stub para ambiente host)
 * ============================================================ */
typedef enum {
    BAIOS_LOG_TRACE = 0,
    BAIOS_LOG_DEBUG,
    BAIOS_LOG_INFO,
    BAIOS_LOG_WARN,
    BAIOS_LOG_ERROR,
    BAIOS_LOG_FATAL,
} baios_log_level_t;

void baios_log(baios_log_level_t level, const char *fmt, ...);

#define BAIOS_LOG_INFO(...)  baios_log(BAIOS_LOG_INFO,  __VA_ARGS__)
#define BAIOS_LOG_WARN(...)  baios_log(BAIOS_LOG_WARN,  __VA_ARGS__)
#define BAIOS_LOG_ERROR(...) baios_log(BAIOS_LOG_ERROR, __VA_ARGS__)
#define BAIOS_LOG_DEBUG(...) baios_log(BAIOS_LOG_DEBUG, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_H */
