#ifndef BAIOS_INTERRUPTS_H
#define BAIOS_INTERRUPTS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAIOS_MAX_IRQ           256
#define BAIOS_IRQ_TIMER         0
#define BAIOS_IRQ_KEYBOARD      1
#define BAIOS_IRQ_MOUSE         12
#define BAIOS_IRQ_DISK          14
#define BAIOS_IRQ_NETWORK       16
#define BAIOS_IRQ_AUDIO         17
#define BAIOS_IRQ_GPU           18
#define BAIOS_IRQ_POWER         19
#define BAIOS_IRQ_IPC           32   /* softirq interno entre kernels */

typedef void (*baios_irq_handler_t)(u32 irq, void *context);

typedef struct {
    u32                 irq;
    baios_irq_handler_t handler;
    void               *context;
    u32                 flags;
    u64                 count;
    bool                enabled;
    char                name[32];
} baios_irq_desc_t;

baios_error_t irq_init(void);
baios_error_t irq_register(u32 irq, baios_irq_handler_t handler,
                           void *context, const char *name);
baios_error_t irq_unregister(u32 irq);
baios_error_t irq_enable(u32 irq);
baios_error_t irq_disable(u32 irq);
void          irq_ack(u32 irq);

/* Chamado pelo hardware / bootstrap quando uma IRQ chega */
void irq_dispatch(u32 irq);

/* Softirq / deferred work */
typedef void (*baios_softirq_t)(void *data);
baios_error_t softirq_raise(baios_softirq_t fn, void *data);
void          softirq_process(void);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_INTERRUPTS_H */
