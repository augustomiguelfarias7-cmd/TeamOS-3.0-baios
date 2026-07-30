/**
 * @file zero_copy.c
 * @brief Implementação do Zero-Copy IPC (shared memory ring buffers)
 */

#include "ipc.h"
#include "baios.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================
 * Estado
 * ============================================================ */

static baios_ipc_ring_t   g_sw_to_hw_ring;
static baios_ipc_ring_t   g_hw_to_sw_ring;
static baios_ipc_channel_t g_channels[BAIOS_MAX_IPC_CHANNELS];
static u32                 g_channel_count = 0;
static bool                g_ipc_ready = false;
static u64                 g_seq_counter = 1;

/* Canal default 0: comunicação principal SW <-> HW */
#define BAIOS_IPC_DEFAULT_CH 0

/* ============================================================
 * Helpers de ring
 * ============================================================ */

static void ring_init(baios_ipc_ring_t *r) {
    memset(r, 0, sizeof(*r));
    r->capacity  = BAIOS_IPC_SLOT_COUNT;
    r->slot_size = sizeof(baios_ipc_msg_t);
    r->head = 0;
    r->tail = 0;
}

static bool ring_full(const baios_ipc_ring_t *r) {
    return ((r->head + 1) % r->capacity) == r->tail;
}

static bool ring_empty(const baios_ipc_ring_t *r) {
    return r->head == r->tail;
}

static baios_status_t ring_push(baios_ipc_ring_t *r, const baios_ipc_msg_t *msg) {
    if (ring_full(r))
        return BAIOS_ERR_OVERFLOW;

    baios_ipc_msg_t *slot = (baios_ipc_msg_t *)r->slots[r->head];
    memcpy(slot, msg, sizeof(*msg));
    /* memory barrier real seria necessário em SMP */
    r->head = (r->head + 1) % r->capacity;
    return BAIOS_OK;
}

static baios_status_t ring_pop(baios_ipc_ring_t *r, baios_ipc_msg_t *out) {
    if (ring_empty(r))
        return BAIOS_ERR_UNDERFLOW;

    baios_ipc_msg_t *slot = (baios_ipc_msg_t *)r->slots[r->tail];
    memcpy(out, slot, sizeof(*out));
    r->tail = (r->tail + 1) % r->capacity;
    return BAIOS_OK;
}

static u64 now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

/* ============================================================
 * API pública
 * ============================================================ */

baios_status_t baios_ipc_init(void) {
    if (g_ipc_ready)
        return BAIOS_ERR_STATE;

    ring_init(&g_sw_to_hw_ring);
    ring_init(&g_hw_to_sw_ring);
    memset(g_channels, 0, sizeof(g_channels));
    g_channel_count = 0;
    g_seq_counter = 1;

    /* Cria canal default */
    g_channels[0].id = 0;
    g_channels[0].sw_to_hw = &g_sw_to_hw_ring;
    g_channels[0].hw_to_sw = &g_hw_to_sw_ring;
    g_channel_count = 1;

    g_ipc_ready = true;
    BAIOS_LOG_INFO("ipc: zero-copy rings ready (slots=%d, msg_max=%d)",
                   BAIOS_IPC_SLOT_COUNT, BAIOS_IPC_MAX_MSG_SIZE);
    return BAIOS_OK;
}

void baios_ipc_shutdown(void) {
    if (!g_ipc_ready)
        return;
    g_ipc_ready = false;
    memset(g_channels, 0, sizeof(g_channels));
    g_channel_count = 0;
    BAIOS_LOG_INFO("ipc: shutdown");
}

baios_status_t baios_ipc_create_channel(u32 *out_id) {
    if (!g_ipc_ready || !out_id)
        return BAIOS_ERR_INVAL;
    if (g_channel_count >= BAIOS_MAX_IPC_CHANNELS)
        return BAIOS_ERR_OVERFLOW;

    u32 id = g_channel_count++;
    g_channels[id].id = id;
    /* Por simplicidade reutilizamos os anéis principais; em produção
       cada canal teria seus próprios anéis alocados. */
    g_channels[id].sw_to_hw = &g_sw_to_hw_ring;
    g_channels[id].hw_to_sw = &g_hw_to_sw_ring;
    *out_id = id;
    return BAIOS_OK;
}

baios_status_t baios_ipc_destroy_channel(u32 id) {
    if (!g_ipc_ready || id == 0 || id >= g_channel_count)
        return BAIOS_ERR_INVAL;
    memset(&g_channels[id], 0, sizeof(g_channels[id]));
    return BAIOS_OK;
}

static baios_status_t ipc_send_on(baios_ipc_ring_t *ring, baios_ipc_channel_t *ch,
                                  const baios_ipc_msg_t *msg) {
    baios_ipc_msg_t local = *msg;
    local.seq = g_seq_counter++;
    local.timestamp_ns = now_ns();

    baios_status_t st = ring_push(ring, &local);
    if (st == BAIOS_OK)
        ch->tx_count++;
    else
        ch->drop_count++;
    return st;
}

baios_status_t baios_ipc_send(u32 channel_id, const baios_ipc_msg_t *msg) {
    if (!g_ipc_ready || !msg || channel_id >= g_channel_count)
        return BAIOS_ERR_INVAL;
    return ipc_send_on(g_channels[channel_id].sw_to_hw, &g_channels[channel_id], msg);
}

baios_status_t baios_ipc_recv(u32 channel_id, baios_ipc_msg_t *out, u32 timeout_ms) {
    if (!g_ipc_ready || !out || channel_id >= g_channel_count)
        return BAIOS_ERR_INVAL;

    baios_ipc_channel_t *ch = &g_channels[channel_id];
    u64 deadline = now_ns() + (u64)timeout_ms * 1000000ULL;

    for (;;) {
        baios_status_t st = ring_pop(ch->hw_to_sw, out);
        if (st == BAIOS_OK) {
            ch->rx_count++;
            return BAIOS_OK;
        }
        if (timeout_ms == 0)
            return BAIOS_ERR_AGAIN;
        if (now_ns() >= deadline)
            return BAIOS_ERR_TIMEOUT;
        /* spin simples; em kernel real usaria wait queue */
    }
}

baios_status_t baios_ipc_send_to_hw(const baios_ipc_msg_t *msg) {
    if (!g_ipc_ready || !msg)
        return BAIOS_ERR_INVAL;
    return ipc_send_on(&g_sw_to_hw_ring, &g_channels[0], msg);
}

baios_status_t baios_ipc_recv_from_hw(baios_ipc_msg_t *out, u32 timeout_ms) {
    return baios_ipc_recv(BAIOS_IPC_DEFAULT_CH, out, timeout_ms);
}

baios_status_t baios_ipc_send_to_sw(const baios_ipc_msg_t *msg) {
    if (!g_ipc_ready || !msg)
        return BAIOS_ERR_INVAL;
    return ipc_send_on(&g_hw_to_sw_ring, &g_channels[0], msg);
}

baios_status_t baios_ipc_recv_from_sw(baios_ipc_msg_t *out, u32 timeout_ms) {
    if (!g_ipc_ready || !out)
        return BAIOS_ERR_INVAL;

    u64 deadline = now_ns() + (u64)timeout_ms * 1000000ULL;
    for (;;) {
        baios_status_t st = ring_pop(&g_sw_to_hw_ring, out);
        if (st == BAIOS_OK) {
            g_channels[0].rx_count++;
            return BAIOS_OK;
        }
        if (timeout_ms == 0)
            return BAIOS_ERR_AGAIN;
        if (now_ns() >= deadline)
            return BAIOS_ERR_TIMEOUT;
    }
}
