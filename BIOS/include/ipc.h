#ifndef BAIOS_IPC_H
#define BAIOS_IPC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Zero-Copy IPC do Baios
 *
 * Os dois microkernels se comunicam exclusivamente através de
 * regiões de memória compartilhada mapeadas em ambos os lados.
 * Não há cópia de payload — apenas troca de ponteiros e metadados.
 */

#define BAIOS_IPC_MAX_CHANNELS     64
#define BAIOS_IPC_MAX_MSG_SIZE     (64 * 1024)  /* 64KB por mensagem */
#define BAIOS_IPC_RING_SLOTS       128
#define BAIOS_IPC_MAGIC            0xBA1051PCULL

/* Header de toda mensagem IPC (fica na shared memory) */
typedef struct __attribute__((packed)) {
    u64              magic;
    u32              seq;
    u32              type;          /* baios_ipc_type_t */
    u32              src_id;        /* 0 = HW kernel, 1 = SW kernel, >1 = pid */
    u32              dst_id;
    u32              opcode;        /* operação específica */
    u32              flags;
    u64              payload_offset;/* offset dentro da região SHM */
    u64              payload_size;
    u64              timestamp_ns;
    i32              status;        /* resultado da operação */
    u32              checksum;
} baios_ipc_header_t;

/* Canal IPC bidirecional (ring buffer de headers) */
typedef struct {
    volatile u32     head;          /* produtor escreve */
    volatile u32     tail;          /* consumidor lê */
    u32              capacity;      /* BAIOS_IPC_RING_SLOTS */
    u32              flags;
    baios_ipc_header_t slots[BAIOS_IPC_RING_SLOTS];
} baios_ipc_ring_t;

/* Descritor de canal */
typedef struct {
    baios_handle_t   handle;
    char             name[48];
    baios_addr_t     shm_base;
    baios_size_t     shm_size;
    baios_ipc_ring_t *tx_ring;      /* para o outro lado */
    baios_ipc_ring_t *rx_ring;      /* do outro lado */
    u32              local_id;
    u32              remote_id;
    bool             active;
} baios_ipc_channel_t;

/* Opcodes padrão entre HW e SW kernel */
enum {
    BAIOS_OP_NOP              = 0,
    BAIOS_OP_HEARTBEAT        = 1,
    BAIOS_OP_ALLOC_PAGES      = 10,
    BAIOS_OP_FREE_PAGES       = 11,
    BAIOS_OP_MAP_REGION       = 12,
    BAIOS_OP_UNMAP_REGION     = 13,
    BAIOS_OP_IRQ_REGISTER     = 20,
    BAIOS_OP_IRQ_UNREGISTER   = 21,
    BAIOS_OP_IRQ_ACK          = 22,
    BAIOS_OP_DEVICE_OPEN      = 30,
    BAIOS_OP_DEVICE_CLOSE     = 31,
    BAIOS_OP_DEVICE_READ      = 32,
    BAIOS_OP_DEVICE_WRITE     = 33,
    BAIOS_OP_DEVICE_IOCTL     = 34,
    BAIOS_OP_POWER_STATE      = 40,
    BAIOS_OP_POWER_WAKE       = 41,
    BAIOS_OP_SYSCALL_FWD      = 50,  /* SW encaminha syscall para HW */
    BAIOS_OP_EVENT            = 60,
    BAIOS_OP_SHUTDOWN         = 255
};

/* Flags de mensagem */
#define BAIOS_IPC_FLAG_URGENT     (1u << 0)
#define BAIOS_IPC_FLAG_NO_REPLY   (1u << 1)
#define BAIOS_IPC_FLAG_BROADCAST  (1u << 2)
#define BAIOS_IPC_FLAG_DMA        (1u << 3)

/* API */
baios_error_t baios_ipc_init(void);
baios_error_t baios_ipc_create_channel(const char *name, baios_size_t shm_size,
                                       baios_handle_t *out_handle);
baios_error_t baios_ipc_destroy_channel(baios_handle_t handle);
baios_error_t baios_ipc_send(baios_handle_t channel,
                             const baios_ipc_header_t *hdr,
                             const void *payload, baios_size_t payload_len);
baios_error_t baios_ipc_recv(baios_handle_t channel,
                             baios_ipc_header_t *hdr,
                             void *payload, baios_size_t *payload_len,
                             u32 timeout_ms);
baios_error_t baios_ipc_try_recv(baios_handle_t channel,
                                 baios_ipc_header_t *hdr,
                                 void *payload, baios_size_t *payload_len);

/* Atalhos para comunicação HW <-> SW */
baios_error_t baios_ipc_hw_to_sw(u32 opcode, const void *data, baios_size_t len);
baios_error_t baios_ipc_sw_to_hw(u32 opcode, const void *data, baios_size_t len,
                                 void *reply, baios_size_t *reply_len);

u32 baios_ipc_checksum(const void *data, baios_size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_IPC_H */
