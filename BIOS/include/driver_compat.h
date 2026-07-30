/**
 * @file driver_compat.h
 * @brief Camada de compatibilidade com drivers estilo Linux
 *
 * Permite reaproveitar módulos de driver Linux através de um thin
 * wrapper. Não é um kernel Linux completo — apenas a interface
 * mínima necessária para o Microkernel de Hardware.
 */
#ifndef BAIOS_DRIVER_COMPAT_H
#define BAIOS_DRIVER_COMPAT_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Descritor de driver
 * ============================================================ */
typedef struct baios_driver {
    const char *name;
    const char *version;
    u32         major;
    u32         minor;

    baios_status_t (*probe)(struct baios_driver *drv, void *device);
    baios_status_t (*remove)(struct baios_driver *drv, void *device);
    baios_status_t (*suspend)(struct baios_driver *drv, void *device);
    baios_status_t (*resume)(struct baios_driver *drv, void *device);

    void *priv;
    u32   flags;
    struct baios_driver *next;
} baios_driver_t;

#define BAIOS_DRV_FLAG_LOADED   (1u << 0)
#define BAIOS_DRV_FLAG_BUSY     (1u << 1)
#define BAIOS_DRV_FLAG_HOTPLUG  (1u << 2)

/* ============================================================
 * Registro de drivers
 * ============================================================ */
baios_status_t baios_driver_init(void);
void           baios_driver_shutdown(void);

baios_status_t baios_driver_register(baios_driver_t *drv);
baios_status_t baios_driver_unregister(baios_driver_t *drv);

baios_driver_t *baios_driver_find(const char *name);

/** Itera sobre todos os drivers registrados */
typedef void (*baios_driver_iter_fn)(baios_driver_t *drv, void *ctx);
void baios_driver_foreach(baios_driver_iter_fn fn, void *ctx);

u32 baios_driver_count(void);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_DRIVER_COMPAT_H */
