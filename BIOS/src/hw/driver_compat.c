/**
 * @file driver_compat.c
 * @brief Camada de compatibilidade de drivers estilo Linux
 */

#include "driver_compat.h"
#include "baios.h"
#include <string.h>

static baios_driver_t *g_drivers = NULL;
static u32             g_driver_count = 0;
static bool            g_drv_ready = false;

baios_status_t baios_driver_init(void) {
    if (g_drv_ready)
        return BAIOS_ERR_STATE;
    g_drivers = NULL;
    g_driver_count = 0;
    g_drv_ready = true;
    BAIOS_LOG_INFO("driver: compatibility layer ready");
    return BAIOS_OK;
}

void baios_driver_shutdown(void) {
    if (!g_drv_ready)
        return;

    baios_driver_t *d = g_drivers;
    while (d) {
        baios_driver_t *next = d->next;
        if (d->remove && (d->flags & BAIOS_DRV_FLAG_LOADED))
            d->remove(d, d->priv);
        d = next;
    }
    g_drivers = NULL;
    g_driver_count = 0;
    g_drv_ready = false;
    BAIOS_LOG_INFO("driver: shutdown complete");
}

baios_status_t baios_driver_register(baios_driver_t *drv) {
    if (!g_drv_ready || !drv || !drv->name)
        return BAIOS_ERR_INVAL;

    if (g_driver_count >= BAIOS_MAX_DRIVERS)
        return BAIOS_ERR_OVERFLOW;

    /* Verifica duplicata */
    for (baios_driver_t *d = g_drivers; d; d = d->next) {
        if (strcmp(d->name, drv->name) == 0)
            return BAIOS_ERR_BUSY;
    }

    drv->next = g_drivers;
    g_drivers = drv;
    drv->flags |= BAIOS_DRV_FLAG_LOADED;
    g_driver_count++;

    BAIOS_LOG_INFO("driver: registered '%s' v%s", drv->name,
                   drv->version ? drv->version : "?");

    if (drv->probe) {
        baios_status_t st = drv->probe(drv, drv->priv);
        if (st != BAIOS_OK) {
            BAIOS_LOG_WARN("driver: probe failed for '%s': %s",
                           drv->name, baios_status_str(st));
        }
    }
    return BAIOS_OK;
}

baios_status_t baios_driver_unregister(baios_driver_t *drv) {
    if (!g_drv_ready || !drv)
        return BAIOS_ERR_INVAL;

    baios_driver_t **pp = &g_drivers;
    while (*pp) {
        if (*pp == drv) {
            if (drv->remove)
                drv->remove(drv, drv->priv);
            *pp = drv->next;
            drv->flags &= ~BAIOS_DRV_FLAG_LOADED;
            g_driver_count--;
            BAIOS_LOG_INFO("driver: unregistered '%s'", drv->name);
            return BAIOS_OK;
        }
        pp = &(*pp)->next;
    }
    return BAIOS_ERR_NOTFOUND;
}

baios_driver_t *baios_driver_find(const char *name) {
    if (!g_drv_ready || !name)
        return NULL;
    for (baios_driver_t *d = g_drivers; d; d = d->next) {
        if (strcmp(d->name, name) == 0)
            return d;
    }
    return NULL;
}

void baios_driver_foreach(baios_driver_iter_fn fn, void *ctx) {
    if (!g_drv_ready || !fn)
        return;
    for (baios_driver_t *d = g_drivers; d; d = d->next)
        fn(d, ctx);
}

u32 baios_driver_count(void) {
    return g_driver_count;
}
