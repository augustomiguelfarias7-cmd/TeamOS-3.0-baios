/**
 * hw/linux_compat.c — Camada de compatibilidade com drivers estilo Linux
 */

#include "../include/drivers.h"
#include "../include/hw_kernel.h"
#include "../include/types.h"

#include <string.h>
#include <stdio.h>

static baios_linux_driver_t *g_drivers = NULL;
static bool g_compat_ready = false;

/* Dispositivos virtuais simples */
static baios_ssize_t null_read(void *file, char *buf, baios_size_t count, u64 *pos) {
    (void)file; (void)buf; (void)pos;
    return 0; /* sempre EOF */
}

static baios_ssize_t null_write(void *file, const char *buf, baios_size_t count, u64 *pos) {
    (void)file; (void)buf; (void)pos;
    return (baios_ssize_t)count; /* descarta tudo */
}

static baios_ssize_t zero_read(void *file, char *buf, baios_size_t count, u64 *pos) {
    (void)file; (void)pos;
    memset(buf, 0, count);
    return (baios_ssize_t)count;
}

static baios_ssize_t zero_write(void *file, const char *buf, baios_size_t count, u64 *pos) {
    (void)file; (void)buf; (void)pos;
    return (baios_ssize_t)count;
}

static baios_file_ops_t null_fops = {
    .read  = null_read,
    .write = null_write,
};

static baios_file_ops_t zero_fops = {
    .read  = zero_read,
    .write = zero_write,
};

baios_error_t linux_compat_init(void) {
    g_drivers = NULL;
    g_compat_ready = true;

    /* Registra dispositivos clássicos */
    driver_null_init();
    driver_zero_init();
    driver_console_init();
    driver_timer_init();
    driver_random_init();

    return BAIOS_OK;
}

baios_error_t linux_register_chrdev(u32 major, const char *name,
                                    baios_file_ops_t *fops,
                                    baios_linux_driver_t **out) {
    if (!g_compat_ready || !name || !fops) return BAIOS_ERR_INVALID_ARG;

    baios_linux_driver_t *drv = (baios_linux_driver_t *)baios_kmalloc(sizeof(*drv));
    if (!drv) return BAIOS_ERR_NO_MEMORY;

    memset(drv, 0, sizeof(*drv));
    strncpy(drv->name, name, sizeof(drv->name) - 1);
    drv->major = major;
    drv->fops  = fops;
    drv->registered = true;
    drv->next  = g_drivers;
    g_drivers  = drv;

    if (out) *out = drv;
    return BAIOS_OK;
}

baios_error_t linux_unregister_chrdev(u32 major) {
    baios_linux_driver_t **pp = &g_drivers;
    while (*pp) {
        if ((*pp)->major == major) {
            baios_linux_driver_t *tmp = *pp;
            *pp = tmp->next;
            baios_kfree(tmp);
            return BAIOS_OK;
        }
        pp = &(*pp)->next;
    }
    return BAIOS_ERR_NOT_FOUND;
}

baios_error_t driver_null_init(void) {
    return linux_register_chrdev(1, "null", &null_fops, NULL);
}

baios_error_t driver_zero_init(void) {
    return linux_register_chrdev(1, "zero", &zero_fops, NULL);
}

baios_error_t driver_console_init(void) {
    /* Placeholder — console real seria ligado ao UART/framebuffer */
    return BAIOS_OK;
}

baios_error_t driver_timer_init(void) {
    return BAIOS_OK;
}

baios_error_t driver_random_init(void) {
    return BAIOS_OK;
}

/* Tabela de drivers built-in (para probe futuro) */
const baios_builtin_driver_t baios_builtin_drivers[] = {
    { "null",    1, 3, driver_null_init    },
    { "zero",    1, 5, driver_zero_init    },
    { "console", 5, 1, driver_console_init },
    { "timer",   10, 0, driver_timer_init  },
    { "random",  1, 8, driver_random_init  },
    { NULL, 0, 0, NULL }
};
