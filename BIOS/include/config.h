/**
 * @file config.h
 * @brief Configuração de compilação e limites do Baios
 */
#ifndef BAIOS_CONFIG_H
#define BAIOS_CONFIG_H

/* Versão */
#define BAIOS_VERSION_MAJOR  3
#define BAIOS_VERSION_MINOR  0
#define BAIOS_VERSION_PATCH  0
#define BAIOS_VERSION_STRING "3.0.0-baios"

/* ============================================================
 * Limites do sistema
 * ============================================================ */
#define BAIOS_MAX_PROCESSES          256
#define BAIOS_MAX_THREADS_PER_PROC   64
#define BAIOS_MAX_HANDLES_PER_PROC   512
#define BAIOS_MAX_OPEN_FILES         1024
#define BAIOS_MAX_DRIVERS            128
#define BAIOS_MAX_IRQ                256
#define BAIOS_MAX_IPC_CHANNELS       64

/* Memória física */
#define BAIOS_PAGE_SIZE              4096ULL
#define BAIOS_PAGE_SHIFT             12
#define BAIOS_MAX_PHYS_PAGES         (1ULL << 20)   /* até 4 GiB com páginas de 4K */
#define BAIOS_BUDDY_MAX_ORDER        10             /* 4K * 2^10 = 4 MiB */

/* Zero-Copy IPC */
#define BAIOS_IPC_RING_SIZE          (64 * 1024)    /* 64 KiB por anel */
#define BAIOS_IPC_MAX_MSG_SIZE       4096
#define BAIOS_IPC_SLOT_COUNT         32

/* Stacks */
#define BAIOS_KERNEL_STACK_SIZE      (16 * 1024)
#define BAIOS_USER_STACK_SIZE        (128 * 1024)

/* Timeouts padrão (ms) */
#define BAIOS_DEFAULT_IPC_TIMEOUT_MS 1000
#define BAIOS_DEFAULT_IO_TIMEOUT_MS  5000

/* Debug */
#ifndef BAIOS_DEBUG
#define BAIOS_DEBUG 1
#endif

#if BAIOS_DEBUG
#define BAIOS_ASSERT(cond) do { \
    if (!(cond)) { \
        /* em produção real: panic */ \
        while (1) { } \
    } \
} while (0)
#else
#define BAIOS_ASSERT(cond) ((void)0)
#endif

#endif /* BAIOS_CONFIG_H */
