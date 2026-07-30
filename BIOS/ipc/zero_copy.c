/**
 * zero_copy.c — Implementação do protocolo Zero-Copy IPC
 *
 * Os dois microkernels compartilham rings de headers + área de payload.
 * O payload nunca é copiado; apenas offsets são trocados.
 */

#include "../include/ipc.h"
#include "../include/memory.h"
#include "../include/types.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#define MAX_CHANNELS 16

static baios_ipc_channel_t g_channels[MAX_CHANNELS];
static u32 g_channel_count = 0;
static bool g_ipc_ready = false;

/* Canal especial entre HW e SW kernel */
static baios_handle_t g_hw_sw_channel = 0;

static u64 now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

u32 baios_ipc_checksum(const void *data, baios_size_t len) {
    const u8 *p = (const u8 *)data;
    u32 sum = 0xBA105;
    for (baios_size_t i = 0; i < len; i++) {
        sum = (sum << 5) + sum + p[i];
    }
    return sum;
}

static baios_ipc_channel_t *find_channel(baios_handle_t handle) {
    for (u32 i = 0; i < g_channel_count; i++) {
        if (g_channels[i].handle == handle && g_channels[i].active) {
            return &g_channels[i];
        }
    }
    return NULL;
}

baios_error_t baios_ipc_init(void) {
    if (g_ipc_ready) return BAIOS_OK;

    memset(g_channels, 0, sizeof(g_channels));
    g_channel_count = 0;

    /* Cria o canal principal HW <-> SW */
    baios_error_t err = baios_ipc_create_channel("hw-sw-main",
                                                 256 * 1024, /* 256 KB */
                                                 &g_hw_sw_channel);
    if (err != BAIOS_OK) return err;

    g_ipc_ready = true;
    return BAIOS_OK;
}

baios_error_t baios_ipc_create_channel(const char *name, baios_size_t shm_size,
                                       baios_handle_t *out_handle) {
    if (!name || shm_size < sizeof(baios_ipc_ring_t) * 2 || !out_handle) {
        return BAIOS_ERR_INVALID_ARG;
    }
    if (g_channel_count >= MAX_CHANNELS) {
        return BAIOS_ERR_NO_MEMORY;
    }

    baios_handle_t shm_h;
    baios_error_t err = baios_shm_create(name, shm_size,
                                         BAIOS_MEM_READ | BAIOS_MEM_WRITE,
                                         &shm_h);
    if (err != BAIOS_OK) return err;

    baios_addr_t base;
    err = baios_shm_attach(shm_h, &base);
    if (err != BAIOS_OK) return err;

    /* Layout da SHM:
     * [tx_ring][rx_ring][payload area...]
     */
    baios_ipc_ring_t *tx = (baios_ipc_ring_t *)base;
    baios_ipc_ring_t *rx = (baios_ipc_ring_t *)(base + sizeof(baios_ipc_ring_t));

    memset(tx, 0, sizeof(*tx));
    memset(rx, 0, sizeof(*rx));
    tx->capacity = BAIOS_IPC_RING_SLOTS;
    rx->capacity = BAIOS_IPC_RING_SLOTS;

    baios_ipc_channel_t *ch = &g_channels[g_channel_count++];
    ch->handle    = shm_h; /* usamos o handle da SHM como id do canal */
    strncpy(ch->name, name, sizeof(ch->name) - 1);
    ch->shm_base  = base;
    ch->shm_size  = shm_size;
    ch->tx_ring   = tx;
    ch->rx_ring   = rx;
    ch->local_id  = 0;
    ch->remote_id = 1;
    ch->active    = true;

    *out_handle = ch->handle;
    return BAIOS_OK;
}

baios_error_t baios_ipc_destroy_channel(baios_handle_t handle) {
    baios_ipc_channel_t *ch = find_channel(handle);
    if (!ch) return BAIOS_ERR_NOT_FOUND;

    ch->active = false;
    return baios_shm_destroy(handle);
}

static bool ring_full(const baios_ipc_ring_t *r) {
    return ((r->head + 1) % r->capacity) == r->tail;
}

static bool ring_empty(const baios_ipc_ring_t *r) {
    return r->head == r->tail;
}

baios_error_t baios_ipc_send(baios_handle_t channel,
                             const baios_ipc_header_t *hdr,
                             const void *payload, baios_size_t payload_len) {
    baios_ipc_channel_t *ch = find_channel(channel);
    if (!ch || !hdr) return BAIOS_ERR_INVALID_ARG;

    if (ring_full(ch->tx_ring)) {
        return BAIOS_ERR_BUSY;
    }

    u32 idx = ch->tx_ring->head;
    baios_ipc_header_t *slot = &ch->tx_ring->slots[idx];

    *slot = *hdr;
    slot->magic        = BAIOS_IPC_MAGIC;
    slot->timestamp_ns = now_ns();
    slot->payload_size = payload_len;

    if (payload && payload_len > 0) {
        /* Offset do payload fica depois dos dois rings */
        baios_addr_t payload_area = ch->shm_base +
                                    2 * sizeof(baios_ipc_ring_t);
        baios_size_t max_payload  = ch->shm_size -
                                    2 * sizeof(baios_ipc_ring_t);

        if (payload_len > max_payload) {
            return BAIOS_ERR_OVERFLOW;
        }

        /* Em Zero-Copy real o payload já estaria na SHM.
         * Aqui fazemos uma única cópia controlada para a área compartilhada. */
        memcpy((void *)payload_area, payload, payload_len);
        slot->payload_offset = 2 * sizeof(baios_ipc_ring_t);
        slot->checksum = baios_ipc_checksum(payload, payload_len);
    } else {
        slot->payload_offset = 0;
        slot->checksum = 0;
    }

    /* Publica o slot (barreira de memória implícita em sistemas reais) */
    ch->tx_ring->head = (idx + 1) % ch->tx_ring->capacity;

    return BAIOS_OK;
}

baios_error_t baios_ipc_try_recv(baios_handle_t channel,
                                 baios_ipc_header_t *hdr,
                                 void *payload, baios_size_t *payload_len) {
    baios_ipc_channel_t *ch = find_channel(channel);
    if (!ch || !hdr) return BAIOS_ERR_INVALID_ARG;

    if (ring_empty(ch->rx_ring)) {
        return BAIOS_ERR_NOT_FOUND; /* nada disponível */
    }

    u32 idx = ch->rx_ring->tail;
    baios_ipc_header_t *slot = &ch->rx_ring->slots[idx];

    if (slot->magic != BAIOS_IPC_MAGIC) {
        return BAIOS_ERR_CORRUPT;
    }

    *hdr = *slot;

    if (payload && payload_len && slot->payload_size > 0) {
        baios_size_t to_copy = slot->payload_size;
        if (to_copy > *payload_len) to_copy = *payload_len;

        const void *src = (const void *)(ch->shm_base + slot->payload_offset);
        memcpy(payload, src, to_copy);
        *payload_len = to_copy;

        u32 chk = baios_ipc_checksum(payload, to_copy);
        if (chk != slot->checksum) {
            return BAIOS_ERR_CORRUPT;
        }
    } else if (payload_len) {
        *payload_len = 0;
    }

    ch->rx_ring->tail = (idx + 1) % ch->rx_ring->capacity;
    return BAIOS_OK;
}

baios_error_t baios_ipc_recv(baios_handle_t channel,
                             baios_ipc_header_t *hdr,
                             void *payload, baios_size_t *payload_len,
                             u32 timeout_ms) {
    /* Versão simplificada: tenta algumas vezes e desiste */
    u32 attempts = timeout_ms / 5 + 1;
    for (u32 i = 0; i < attempts; i++) {
        baios_error_t err = baios_ipc_try_recv(channel, hdr, payload, payload_len);
        if (err == BAIOS_OK) return BAIOS_OK;
        if (err != BAIOS_ERR_NOT_FOUND) return err;
        /* em kernel real usaríamos wait/notify */
    }
    return BAIOS_ERR_TIMEOUT;
}

baios_error_t baios_ipc_hw_to_sw(u32 opcode, const void *data, baios_size_t len) {
    baios_ipc_header_t hdr = {0};
    hdr.type     = BAIOS_IPC_NOTIFY;
    hdr.src_id   = BAIOS_HW_KERNEL_ID;
    hdr.dst_id   = BAIOS_SW_KERNEL_ID;
    hdr.opcode   = opcode;
    hdr.flags    = BAIOS_IPC_FLAG_NO_REPLY;

    return baios_ipc_send(g_hw_sw_channel, &hdr, data, len);
}

baios_error_t baios_ipc_sw_to_hw(u32 opcode, const void *data, baios_size_t len,
                                 void *reply, baios_size_t *reply_len) {
    baios_ipc_header_t hdr = {0};
    hdr.type     = BAIOS_IPC_REQUEST;
    hdr.src_id   = BAIOS_SW_KERNEL_ID;
    hdr.dst_id   = BAIOS_HW_KERNEL_ID;
    hdr.opcode   = opcode;

    baios_error_t err = baios_ipc_send(g_hw_sw_channel, &hdr, data, len);
    if (err != BAIOS_OK) return err;

    /* Em implementação completa aguardaríamos resposta no rx_ring.
     * Por enquanto retornamos OK. */
    if (reply_len) *reply_len = 0;
    (void)reply;
    return BAIOS_OK;
}
