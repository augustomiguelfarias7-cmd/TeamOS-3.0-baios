/**
 * ipc_bridge.cpp — Ponte de IPC do lado do Software Microkernel
 */

#include "../include/ipc.h"
#include "../include/sw_kernel.h"
#include "../include/types.h"

#include <cstdio>
#include <cstring>

extern "C" {

baios_error_t sw_ipc_send_to_hw(u32 opcode, const void *data, baios_size_t len) {
    return baios_ipc_sw_to_hw(opcode, data, len, nullptr, nullptr);
}

baios_error_t sw_ipc_notify_event(u32 event_code, const void *data, baios_size_t len) {
    baios_ipc_header_t hdr = {};
    hdr.type   = BAIOS_IPC_EVENT;
    hdr.src_id = BAIOS_SW_KERNEL_ID;
    hdr.dst_id = BAIOS_HW_KERNEL_ID;
    hdr.opcode = event_code;
    hdr.flags  = BAIOS_IPC_FLAG_NO_REPLY;

    /* Usa o canal principal criado em baios_ipc_init */
    baios_handle_t ch = 0;
    if (baios_shm_find("hw-sw-main", &ch) != BAIOS_OK) {
        return BAIOS_ERR_IPC;
    }
    return baios_ipc_send(ch, &hdr, data, len);
}

void sw_ipc_poll(void) {
    baios_handle_t ch = 0;
    if (baios_shm_find("hw-sw-main", &ch) != BAIOS_OK) return;

    baios_ipc_header_t hdr;
    u8 buf[512];
    baios_size_t len = sizeof(buf);

    while (baios_ipc_try_recv(ch, &hdr, buf, &len) == BAIOS_OK) {
        std::printf("[SW] IPC recebido: opcode=%u type=%u size=%llu\n",
                    hdr.opcode, hdr.type, (unsigned long long)hdr.payload_size);
        len = sizeof(buf);
    }
}

} // extern "C"
