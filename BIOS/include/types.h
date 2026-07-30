#ifndef BAIOS_TYPES_H
#define BAIOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Tipos básicos do Baios */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef uintptr_t baios_addr_t;
typedef size_t    baios_size_t;
typedef int32_t   baios_error_t;
typedef uint32_t  baios_pid_t;
typedef uint32_t  baios_uid_t;
typedef uint64_t  baios_handle_t;

/* Códigos de erro padrão */
#define BAIOS_OK                 0
#define BAIOS_ERR_INVALID_ARG   -1
#define BAIOS_ERR_NO_MEMORY     -2
#define BAIOS_ERR_PERMISSION    -3
#define BAIOS_ERR_NOT_FOUND     -4
#define BAIOS_ERR_BUSY          -5
#define BAIOS_ERR_TIMEOUT       -6
#define BAIOS_ERR_IO            -7
#define BAIOS_ERR_NOT_SUPPORTED -8
#define BAIOS_ERR_IPC           -9
#define BAIOS_ERR_ALREADY_EXISTS -10
#define BAIOS_ERR_OVERFLOW      -11
#define BAIOS_ERR_UNDERFLOW     -12
#define BAIOS_ERR_CORRUPT       -13

/* Alinhamento e páginas */
#define BAIOS_PAGE_SIZE         4096ULL
#define BAIOS_PAGE_SHIFT        12
#define BAIOS_PAGE_MASK         (~(BAIOS_PAGE_SIZE - 1))

#define BAIOS_ALIGN_UP(x, a)    (((x) + ((a) - 1)) & ~((a) - 1))
#define BAIOS_ALIGN_DOWN(x, a)  ((x) & ~((a) - 1))
#define BAIOS_IS_ALIGNED(x, a)  (((x) & ((a) - 1)) == 0)

/* Flags de memória */
#define BAIOS_MEM_READ          (1u << 0)
#define BAIOS_MEM_WRITE         (1u << 1)
#define BAIOS_MEM_EXEC          (1u << 2)
#define BAIOS_MEM_SHARED        (1u << 3)
#define BAIOS_MEM_USER          (1u << 4)
#define BAIOS_MEM_KERNEL        (1u << 5)
#define BAIOS_MEM_DEVICE        (1u << 6)
#define BAIOS_MEM_ZERO_COPY     (1u << 7)

/* Prioridades de processo */
typedef enum {
    BAIOS_PRIO_IDLE       = 0,
    BAIOS_PRIO_LOW        = 1,
    BAIOS_PRIO_NORMAL     = 2,
    BAIOS_PRIO_HIGH       = 3,
    BAIOS_PRIO_REALTIME   = 4,
    BAIOS_PRIO_KERNEL     = 5
} baios_priority_t;

/* Estados de processo */
typedef enum {
    BAIOS_PROC_CREATED    = 0,
    BAIOS_PROC_READY      = 1,
    BAIOS_PROC_RUNNING    = 2,
    BAIOS_PROC_BLOCKED    = 3,
    BAIOS_PROC_SLEEPING   = 4,
    BAIOS_PROC_ZOMBIE     = 5,
    BAIOS_PROC_TERMINATED = 6
} baios_proc_state_t;

/* Tipos de mensagem IPC */
typedef enum {
    BAIOS_IPC_REQUEST     = 0,
    BAIOS_IPC_RESPONSE    = 1,
    BAIOS_IPC_NOTIFY      = 2,
    BAIOS_IPC_SIGNAL      = 3,
    BAIOS_IPC_DMA         = 4,
    BAIOS_IPC_CONTROL     = 5
} baios_ipc_type_t;

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_TYPES_H */
