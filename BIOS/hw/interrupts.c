/**
 * hw/interrupts.c — Gerenciamento de interrupções do Hardware Microkernel
 */

#include "../include/interrupts.h"
#include "../include/types.h"

#include <string.h>
#include <stdio.h>

static baios_irq_desc_t g_irq_table[BAIOS_MAX_IRQ];
static bool g_irq_ready = false;

/* Softirq queue simples */
#define MAX_SOFTIRQ 32
typedef struct {
    baios_softirq_t fn;
    void *data;
} softirq_item_t;

static softirq_item_t g_softirq_queue[MAX_SOFTIRQ];
static u32 g_softirq_head = 0;
static u32 g_softirq_tail = 0;

baios_error_t irq_init(void) {
    memset(g_irq_table, 0, sizeof(g_irq_table));
    g_softirq_head = g_softirq_tail = 0;
    g_irq_ready = true;
    return BAIOS_OK;
}

baios_error_t irq_register(u32 irq, baios_irq_handler_t handler,
                           void *context, const char *name) {
    if (!g_irq_ready || irq >= BAIOS_MAX_IRQ || !handler) {
        return BAIOS_ERR_INVALID_ARG;
    }
    if (g_irq_table[irq].handler != NULL) {
        return BAIOS_ERR_BUSY;
    }

    g_irq_table[irq].irq      = irq;
    g_irq_table[irq].handler  = handler;
    g_irq_table[irq].context  = context;
    g_irq_table[irq].flags     = 0;
    g_irq_table[irq].count     = 0;
    g_irq_table[irq].enabled   = true;
    if (name) {
        strncpy(g_irq_table[irq].name, name, sizeof(g_irq_table[irq].name) - 1);
    } else {
        snprintf(g_irq_table[irq].name, sizeof(g_irq_table[irq].name), "irq%u", irq);
    }
    return BAIOS_OK;
}

baios_error_t irq_unregister(u32 irq) {
    if (irq >= BAIOS_MAX_IRQ) return BAIOS_ERR_INVALID_ARG;
    memset(&g_irq_table[irq], 0, sizeof(g_irq_table[irq]));
    return BAIOS_OK;
}

baios_error_t irq_enable(u32 irq) {
    if (irq >= BAIOS_MAX_IRQ || !g_irq_table[irq].handler) {
        return BAIOS_ERR_INVALID_ARG;
    }
    g_irq_table[irq].enabled = true;
    return BAIOS_OK;
}

baios_error_t irq_disable(u32 irq) {
    if (irq >= BAIOS_MAX_IRQ) return BAIOS_ERR_INVALID_ARG;
    g_irq_table[irq].enabled = false;
    return BAIOS_OK;
}

void irq_ack(u32 irq) {
    (void)irq;
    /* Em hardware real: escrever no controlador de interrupções (APIC/GIC) */
}

void irq_dispatch(u32 irq) {
    if (irq >= BAIOS_MAX_IRQ) return;

    baios_irq_desc_t *desc = &g_irq_table[irq];
    if (!desc->handler || !desc->enabled) return;

    desc->count++;
    desc->handler(irq, desc->context);
    irq_ack(irq);
}

baios_error_t softirq_raise(baios_softirq_t fn, void *data) {
    if (!fn) return BAIOS_ERR_INVALID_ARG;

    u32 next = (g_softirq_head + 1) % MAX_SOFTIRQ;
    if (next == g_softirq_tail) {
        return BAIOS_ERR_BUSY; /* fila cheia */
    }

    g_softirq_queue[g_softirq_head].fn   = fn;
    g_softirq_queue[g_softirq_head].data = data;
    g_softirq_head = next;
    return BAIOS_OK;
}

void softirq_process(void) {
    while (g_softirq_tail != g_softirq_head) {
        softirq_item_t item = g_softirq_queue[g_softirq_tail];
        g_softirq_tail = (g_softirq_tail + 1) % MAX_SOFTIRQ;
        if (item.fn) {
            item.fn(item.data);
        }
    }
}
