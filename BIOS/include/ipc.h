/**
 * @file ipc.h
 * @brief Zero-Copy IPC — Shared Memory Ring Buffer
 *
 * Comunicação entre Microkernel de Software e Microkernel de Hardware
 * sem cópia de dados grandes: apenas ponteiros e metadados passam
 * pelo anel.
 */
#ifndef BAIOS_IPC_H
#define BAIOS_IPC_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Tipos de mensagem IPC
 * ============================================================ */
typedef enum {
    BAIOS_IPC_MSG_NONE = 0,
    BAIOS_IPC_MSG_SYSCALL,       /* SW → HW: pedido de syscall */
    BAIOS_IPC_MSG_IRQ,           /* HW → SW: notificação de IRQ */
    BAIOS_IPC_MSG_DMA_DONE,      /* HW → SW: DMA finalizado */
    BAIOS_IPC_MSG_DRIVER,        /* bidirecional: controle de driver */
    BAIOS_IPC_MSG_POWER,         /* controle de energia */
    BAIOS_IPC_MSG_REPLY,         /* resposta genérica */
    BAIOS_IPC_MSG_CUSTOM = 100,
} baios_ipc_msg_type_t;

/* ============================================================
 * Slot de mensagem (cabeçalho + payload opaco)
 * ============================================================ */
typedef struct {
    u32  type;           /* baios_ipc_msg_type_t */
    u32  flags;
    u32  src_id;         /* quem enviou */
    u32  dst_id;         /* destino (0 = broadcast kernel) */
    u64  seq;            /* número de sequência */
    u64  timestamp_ns;
    u32  data_len;       /* bytes válidos em data[] */
    u32  reserved;
    u8   data[BAIOS_IPC_MAX_MSG_SIZE];
} baios_ipc_msg_t;

#define BAIOS_IPC_FLAG_URGENT   (1u << 0)
#define BAIOS_IPC_FLAG_NOCOPY   (1u << 1)  /* data[] contém apenas ponteiro */
#define BAIOS_IPC_FLAG_REPLY    (1u << 2)

/* ============================================================
 * Ring buffer compartilhado
 * ============================================================ */
typedef struct {
    volatile u32 head;   /* produtor escreve */
    volatile u32 tail;   /* consumidor lê */
    u32          capacity;
    u32          slot_size;
    u8           slots[BAIOS_IPC_SLOT_COUNT][sizeof(baios_ipc_msg_t)];
} baios_ipc_ring_t;

/* ============================================================
 * Canal IPC (par de anéis: SW→HW e HW→SW)
 * ============================================================ */
typedef struct {
    u32              id;
    baios_ipc_ring_t *sw_to_hw;
    baios_ipc_ring_t *hw_to_sw;
    u64              tx_count;
    u64              rx_count;
    u64              drop_count;
} baios_ipc_channel_t;

/* ============================================================
 * API
 * ============================================================ */
baios_status_t baios_ipc_init(void);
void           baios_ipc_shutdown(void);

baios_status_t baios_ipc_create_channel(u32 *out_id);
baios_status_t baios_ipc_destroy_channel(u32 id);

/** Envia mensagem (copia o header + data_len bytes). Zero-copy se FLAG_NOCOPY. */
baios_status_t baios_ipc_send(u32 channel_id, const baios_ipc_msg_t *msg);

/** Recebe mensagem (bloqueante com timeout_ms, 0 = non-blocking). */
baios_status_t baios_ipc_recv(u32 channel_id, baios_ipc_msg_t *out, u32 timeout_ms);

/** Versão dirigida: SW envia para HW ou vice-versa */
baios_status_t baios_ipc_send_to_hw(const baios_ipc_msg_t *msg);
baios_status_t baios_ipc_recv_from_hw(baios_ipc_msg_t *out, u32 timeout_ms);
baios_status_t baios_ipc_send_to_sw(const baios_ipc_msg_t *msg);
baios_status_t baios_ipc_recv_from_sw(baios_ipc_msg_t *out, u32 timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_IPC_H */
