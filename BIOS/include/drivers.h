#ifndef BAIOS_DRIVERS_H
#define BAIOS_DRIVERS_H

#include "types.h"
#include "hw_kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Camada de compatibilidade com drivers Linux
 *
 * O Hardware Microkernel não reimplementa todos os drivers.
 * Ele fornece um shim que permite carregar e chamar módulos
 * no estilo Linux (file_operations, platform_device, etc.).
 */

/* Espelho simplificado de struct file_operations do Linux */
typedef struct baios_file_ops {
    baios_error_t (*open)(void *inode, void *file);
    baios_error_t (*release)(void *inode, void *file);
    baios_ssize_t (*read)(void *file, char *buf, baios_size_t count, u64 *pos);
    baios_ssize_t (*write)(void *file, const char *buf, baios_size_t count, u64 *pos);
    baios_error_t (*ioctl)(void *file, u32 cmd, u64 arg);
    baios_error_t (*mmap)(void *file, void *vma);
} baios_file_ops_t;

typedef i64 baios_ssize_t;

typedef struct baios_linux_driver {
    char              name[64];
    u32               major;
    u32               minor_start;
    u32               minor_count;
    baios_file_ops_t *fops;
    void             *private_data;
    bool              registered;
    struct baios_linux_driver *next;
} baios_linux_driver_t;

baios_error_t linux_compat_init(void);
baios_error_t linux_register_chrdev(u32 major, const char *name,
                                    baios_file_ops_t *fops,
                                    baios_linux_driver_t **out);
baios_error_t linux_unregister_chrdev(u32 major);

/* Helpers para drivers comuns simulados */
baios_error_t driver_console_init(void);
baios_error_t driver_timer_init(void);
baios_error_t driver_null_init(void);
baios_error_t driver_zero_init(void);
baios_error_t driver_random_init(void);

/* Tabela de dispositivos conhecidos */
typedef struct {
    const char *name;
    u32         major;
    u32         minor;
    baios_error_t (*probe)(void);
} baios_builtin_driver_t;

extern const baios_builtin_driver_t baios_builtin_drivers[];

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_DRIVERS_H */
