/**
 * @file interrupt.c
 * @brief Framework de IRQs do Microkernel de Hardware
 */

#include "interrupt.h"
#include "baios.h"
#include <string.h>

static baios_irq_desc_t g_irqs[BAIOS_MAX_IRQ];
static bool             g_irq_ready = false;
static u64              g_irq_total = 0;

baios_status_t baios_irq_init(void) {
    if (g_irq_ready)
        return BAIOS_ERR_STATE;

    memset(g_irqs, 0, sizeof(g_irqs));
    g_irq_total = 0;
    g_irq_ready = true;

    BAIOS_LOG_INFO("irq: framework initialized (max %d)", BAIOS_MAX_IRQ);
    return BAIOS_OK;
}

void baios_irq_shutdown(void) {
    if (!g_irq_ready)
        return;
    memset(g_irqs, 0, sizeof(g_irqs));
    g_irq_ready = false;
    BAIOS_LOG_INFO("irq: shutdown (total fired %llu)", (unsigned long long)g_irq_total);
}

baios_status_t baios_irq_request(u32 irq, baios_irq_handler_t handler,
                                 void *opaque, const char *name, u32 flags) {
    if (!g_irq_ready || irq >= BAIOS_MAX_IRQ || !handler)
        return BAIOS_ERR_INVAL;

    baios_irq_desc_t *desc = &g_irqs[irq];

    if (desc->handler && !(flags & BAIOS_IRQ_FLAG_SHARED))
        return BAIOS_ERR_BUSY;

    desc->handler = handler;
    desc->opaque  = opaque;
    desc->name    = name ? name : "unnamed";
    desc->flags   = flags;
    desc->count   = 0;

    BAIOS_LOG_DEBUG("irq: registered IRQ %u (%s)", irq, desc->name);
    return BAIOS_OK;
}

baios_status_t baios_irq_free(u32 irq, void *opaque) {
    if (!g_irq_ready || irq >= BAIOS_MAX_IRQ)
        return BAIOS_ERR_INVAL;

    baios_irq_desc_t *desc = &g_irqs[irq];
    if (desc->opaque != opaque && opaque != NULL)
        return BAIOS_ERR_PERM;

    memset(desc, 0, sizeof(*desc));
    return BAIOS_OK;
}

void baios_irq_enable(u32 irq) {
    (void)irq;
    /* Em hardware real: unmask no controlador de interrupções */
}

void baios_irq_disable(u32 irq) {
    (void)irq;
    /* Em hardware real: mask no controlador de interrupções */
}

void baios_irq_dispatch(u32 irq) {
    if (!g_irq_ready || irq >= BAIOS_MAX_IRQ)
        return;

    baios_irq_desc_t *desc = &g_irqs[irq];
    if (desc->handler) {
        desc->count++;
        g_irq_total++;
        desc->handler(irq, desc->opaque);
    }
}

u64 baios_irq_get_count(u32 irq) {
    if (irq >= BAIOS_MAX_IRQ)
        return 0;
    return g_irqs[irq].count;
}
