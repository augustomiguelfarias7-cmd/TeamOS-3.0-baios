/**
 * @file types.h
 * @brief Tipos fundamentais do Baios Dual Microkernel
 */
#ifndef BAIOS_TYPES_H
#define BAIOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Tipos inteiros padrão
 * ============================================================ */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

typedef uintptr_t uptr;
typedef size_t    usize;
typedef ptrdiff_t isize;

/* ============================================================
 * Resultados e erros
 * ============================================================ */
typedef enum {
    BAIOS_OK                =  0,
    BAIOS_ERR_NOMEM         = -1,
    BAIOS_ERR_INVAL         = -2,
    BAIOS_ERR_PERM          = -3,
    BAIOS_ERR_NOTFOUND      = -4,
    BAIOS_ERR_BUSY          = -5,
    BAIOS_ERR_IO            = -6,
    BAIOS_ERR_TIMEOUT       = -7,
    BAIOS_ERR_AGAIN         = -8,
    BAIOS_ERR_NOSYS         = -9,
    BAIOS_ERR_FAULT         = -10,
    BAIOS_ERR_OVERFLOW      = -11,
    BAIOS_ERR_UNDERFLOW     = -12,
    BAIOS_ERR_IPC           = -13,
    BAIOS_ERR_DRIVER        = -14,
    BAIOS_ERR_STATE         = -15,
} baios_status_t;

static inline const char *baios_status_str(baios_status_t s) {
    switch (s) {
        case BAIOS_OK:           return "OK";
        case BAIOS_ERR_NOMEM:    return "Out of memory";
        case BAIOS_ERR_INVAL:    return "Invalid argument";
        case BAIOS_ERR_PERM:     return "Permission denied";
        case BAIOS_ERR_NOTFOUND: return "Not found";
        case BAIOS_ERR_BUSY:     return "Resource busy";
        case BAIOS_ERR_IO:       return "I/O error";
        case BAIOS_ERR_TIMEOUT:  return "Timeout";
        case BAIOS_ERR_AGAIN:    return "Try again";
        case BAIOS_ERR_NOSYS:    return "Not implemented";
        case BAIOS_ERR_FAULT:    return "Bad address";
        case BAIOS_ERR_OVERFLOW: return "Overflow";
        case BAIOS_ERR_UNDERFLOW:return "Underflow";
        case BAIOS_ERR_IPC:      return "IPC error";
        case BAIOS_ERR_DRIVER:   return "Driver error";
        case BAIOS_ERR_STATE:    return "Invalid state";
        default:                 return "Unknown error";
    }
}

/* ============================================================
 * Identificadores
 * ============================================================ */
typedef u32 baios_pid_t;
typedef u32 baios_tid_t;
typedef u64 baios_handle_t;
typedef u64 baios_cap_t;          /* Capability token */

#define BAIOS_INVALID_PID     ((baios_pid_t)0)
#define BAIOS_INVALID_HANDLE  ((baios_handle_t)0)
#define BAIOS_INVALID_CAP     ((baios_cap_t)0)

/* ============================================================
 * Prioridades e flags
 * ============================================================ */
typedef enum {
    BAIOS_PRIO_IDLE       = 0,
    BAIOS_PRIO_LOW        = 1,
    BAIOS_PRIO_NORMAL     = 2,
    BAIOS_PRIO_HIGH       = 3,
    BAIOS_PRIO_REALTIME   = 4,
    BAIOS_PRIO_KERNEL     = 5,
} baios_prio_t;

/* ============================================================
 * Alinhamento e utilitários
 * ============================================================ */
#define BAIOS_ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define BAIOS_ALIGN_DOWN(x, a) ((x) & ~((a) - 1))
#define BAIOS_IS_ALIGNED(x, a) (((x) & ((a) - 1)) == 0)

#define BAIOS_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BAIOS_MAX(a, b) ((a) > (b) ? (a) : (b))

#define BAIOS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BAIOS_UNUSED(x) ((void)(x))

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_TYPES_H */
