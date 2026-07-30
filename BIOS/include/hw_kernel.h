#ifndef BAIOS_HW_KERNEL_H
#define BAIOS_HW_KERNEL_H

#include "types.h"
#include "memory.h"
#include "interrupts.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hardware Microkernel
 *
 * Responsável por:
 * - Interface direta com CPU, memória física e periféricos
 * - Gerenciamento de interrupções
 * - Controle de energia
 * - Camada de compatibilidade com drivers Linux
 * - Execução de operações solicitadas pelo Software Microkernel
 */

#define BAIOS_HW_KERNEL_ID  0

typedef struct {
    bool initialized;
    u64  boot_time_ns;
    u64  heartbeat;
    u32  irq_count;
    u32  device_count;
    baios_buddy_t *buddy;
} baios_hw_state_t;

/* Lifecycle */
baios_error_t hw_kernel_init(void);
void          hw_kernel_shutdown(void);
void          hw_kernel_main_loop(void);  /* loop cooperativo / interrupt-driven */

/* Heartbeat */
void hw_kernel_tick(void);
u64  hw_kernel_get_heartbeat(void);

/* Memory (lado hardware) */
baios_error_t hw_mem_alloc_pages(u32 order, baios_addr_t *out_phys);
baios_error_t hw_mem_free_pages(baios_addr_t phys, u32 order);
baios_error_t hw_mem_map_physical(baios_addr_t phys, baios_size_t size,
                                  u32 flags, baios_addr_t *out_virt);

/* Power management */
typedef enum {
    BAIOS_POWER_ON        = 0,
    BAIOS_POWER_IDLE      = 1,
    BAIOS_POWER_SUSPEND   = 2,
    BAIOS_POWER_HIBERNATE = 3,
    BAIOS_POWER_OFF       = 4
} baios_power_state_t;

baios_error_t hw_power_set_state(baios_power_state_t state);
baios_power_state_t hw_power_get_state(void);
baios_error_t hw_power_register_wake_source(u32 irq);

/* Device / driver interface (Linux compat) */
typedef struct baios_device {
    u32          id;
    char         name[64];
    u32          major;
    u32          minor;
    void        *private_data;
    baios_error_t (*open)(struct baios_device *dev);
    baios_error_t (*close)(struct baios_device *dev);
    baios_error_t (*read)(struct baios_device *dev, void *buf, baios_size_t len, baios_size_t *out);
    baios_error_t (*write)(struct baios_device *dev, const void *buf, baios_size_t len, baios_size_t *out);
    baios_error_t (*ioctl)(struct baios_device *dev, u32 cmd, void *arg);
    struct baios_device *next;
} baios_device_t;

baios_error_t hw_device_register(baios_device_t *dev);
baios_error_t hw_device_unregister(u32 id);
baios_device_t *hw_device_find(const char *name);
baios_device_t *hw_device_get(u32 id);

/* Entrada Assembly (bootstrap) */
void hw_entry_asm(void);  /* definido em entry.S */

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_HW_KERNEL_H */
