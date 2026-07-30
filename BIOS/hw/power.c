/**
 * hw/power.c — Controle de energia do Hardware Microkernel
 */

#include "../include/hw_kernel.h"
#include "../include/types.h"

#include <stdio.h>

static baios_power_state_t g_power_state = BAIOS_POWER_ON;
static u32 g_wake_sources = 0; /* bitmask de IRQs que podem acordar */

baios_error_t hw_power_set_state(baios_power_state_t state) {
    switch (state) {
        case BAIOS_POWER_ON:
        case BAIOS_POWER_IDLE:
        case BAIOS_POWER_SUSPEND:
        case BAIOS_POWER_HIBERNATE:
        case BAIOS_POWER_OFF:
            g_power_state = state;
            return BAIOS_OK;
        default:
            return BAIOS_ERR_INVALID_ARG;
    }
}

baios_power_state_t hw_power_get_state(void) {
    return g_power_state;
}

baios_error_t hw_power_register_wake_source(u32 irq) {
    if (irq >= 32) return BAIOS_ERR_INVALID_ARG;
    g_wake_sources |= (1u << irq);
    return BAIOS_OK;
}

/* Chamado quando o sistema precisa entrar em baixo consumo */
void hw_power_enter_idle(void) {
    if (g_power_state == BAIOS_POWER_ON) {
        g_power_state = BAIOS_POWER_IDLE;
        /* Em hardware real: instrução WFI / HLT / wfi */
    }
}

void hw_power_wake(void) {
    if (g_power_state == BAIOS_POWER_IDLE ||
        g_power_state == BAIOS_POWER_SUSPEND) {
        g_power_state = BAIOS_POWER_ON;
    }
}
