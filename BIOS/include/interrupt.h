/**
 * @file interrupt.h
 * @brief Framework de interrupções — Microkernel de Hardware
 */
#ifndef BAIOS_INTERRUPT_H
#define BAIOS_INTERRUPT_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Handler de IRQ */
typedef void (*baios_irq_handler_t)(u32 irq, void *opaque);

typedef struct {
    baios_irq_handler_t handler;
    void               *opaque;
    const char         *name;
    u32                 flags;
    u64                 count;      /* quantas vezes disparou */
} baios_irq_desc_t;

#define BAIOS_IRQ_FLAG_SHARED   (1u << 0)
#define BAIOS_IRQ_FLAG_ONESHOT  (1u << 1)
#define BAIOS_IRQ_FLAG_EDGE     (1u << 2)
#define BAIOS_IRQ_FLAG_LEVEL    (1u << 3)

baios_status_t baios_irq_init(void);
void           baios_irq_shutdown(void);

baios_status_t baios_irq_request(u32 irq, baios_irq_handler_t handler,
                                 void *opaque, const char *name, u32 flags);
baios_status_t baios_irq_free(u32 irq, void *opaque);

void baios_irq_enable(u32 irq);
void baios_irq_disable(u32 irq);

/** Chamado pelo low-level quando uma IRQ real chega */
void baios_irq_dispatch(u32 irq);

/** Estatísticas simples */
u64 baios_irq_get_count(u32 irq);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_INTERRUPT_H */
