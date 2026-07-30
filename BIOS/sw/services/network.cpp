/**
 * services/network.cpp — Serviço de rede de alto nível
 */

#include "../../include/sw_kernel.h"
#include "../../include/types.h"
#include "../../include/permissions.h"

#include <cstdio>
#include <cstring>

namespace {

struct NetState {
    bool initialized;
    bool link_up;
    u64  rx_bytes;
    u64  tx_bytes;
    u32  open_sockets;
};

static NetState g_net = {false, false, 0, 0, 0};

} // namespace

extern "C" {

baios_error_t service_network_init(void) {
    g_net.initialized = true;
    g_net.link_up = true; /* simulado */
    g_net.rx_bytes = 0;
    g_net.tx_bytes = 0;
    g_net.open_sockets = 0;
    std::printf("[SW] Serviço de rede inicializado (link up)\n");
    return BAIOS_OK;
}

baios_error_t service_network_socket(baios_pid_t caller, u32 *out_fd) {
    if (!g_net.initialized || !out_fd) return BAIOS_ERR_INVALID_ARG;
    if (!perm_check(caller, BAIOS_CAP_NETWORK)) {
        return BAIOS_ERR_PERMISSION;
    }
    g_net.open_sockets++;
    *out_fd = 1000 + g_net.open_sockets; /* fd fake */
    return BAIOS_OK;
}

baios_error_t service_network_send(u32 sock, const void *data, baios_size_t len) {
    (void)sock;
    if (!data || len == 0) return BAIOS_ERR_INVALID_ARG;
    if (!g_net.link_up) return BAIOS_ERR_IO;
    g_net.tx_bytes += len;
    return BAIOS_OK;
}

baios_error_t service_network_recv(u32 sock, void *buf, baios_size_t len, baios_size_t *out) {
    (void)sock; (void)buf; (void)len;
    if (!out) return BAIOS_ERR_INVALID_ARG;
    *out = 0; /* nada disponível na simulação */
    return BAIOS_OK;
}

void service_network_shutdown(void) {
    g_net.initialized = false;
    g_net.link_up = false;
}

} // extern "C"
