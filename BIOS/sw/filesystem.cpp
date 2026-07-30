/**
 * filesystem.cpp — Sistema de arquivos lógico do Software Microkernel
 *
 * Implementação em memória (ramfs) para desenvolvimento.
 * Em produção seria ligado a um VFS real + storage via HW kernel.
 */

#include "../include/sw_kernel.h"
#include "../include/types.h"
#include "../include/memory.h"

#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace {

struct VNode {
    std::string name;
    bool is_dir;
    std::vector<u8> data;
    std::unordered_map<std::string, VNode*> children;
    u32 mode;
    baios_uid_t uid;
    baios_uid_t gid;

    VNode(const std::string& n, bool dir)
        : name(n), is_dir(dir), mode(dir ? 0755 : 0644), uid(0), gid(0) {}

    ~VNode() {
        for (auto& kv : children) delete kv.second;
    }
};

static VNode* g_root = nullptr;
static std::mutex g_fs_mutex;

VNode* resolve(const char* path) {
    if (!g_root || !path) return nullptr;
    if (path[0] != '/') return nullptr;

    VNode* cur = g_root;
    const char* p = path + 1;

    while (*p) {
        char component[256];
        size_t i = 0;
        while (*p && *p != '/' && i < sizeof(component) - 1) {
            component[i++] = *p++;
        }
        component[i] = '\0';
        if (*p == '/') p++;

        if (component[0] == '\0') continue;
        if (!cur->is_dir) return nullptr;

        auto it = cur->children.find(component);
        if (it == cur->children.end()) return nullptr;
        cur = it->second;
    }
    return cur;
}

} // namespace

extern "C" {

baios_error_t fs_init(void) {
    std::lock_guard<std::mutex> lock(g_fs_mutex);
    if (g_root) return BAIOS_OK;

    g_root = new (std::nothrow) VNode("/", true);
    if (!g_root) return BAIOS_ERR_NO_MEMORY;

    /* Cria estrutura mínima */
    const char* dirs[] = { "dev", "proc", "sys", "tmp", "home", "etc" };
    for (const char* d : dirs) {
        g_root->children[d] = new VNode(d, true);
    }

    /* Arquivos de exemplo em /etc */
    VNode* etc = g_root->children["etc"];
    VNode* hostname = new VNode("hostname", false);
    const char* hn = "teamos\n";
    hostname->data.assign(hn, hn + std::strlen(hn));
    etc->children["hostname"] = hostname;

    std::printf("[SW] Filesystem (ramfs) inicializado\n");
    return BAIOS_OK;
}

baios_error_t fs_open(const char* path, u32 flags, u32* out_fd) {
    (void)flags;
    if (!path || !out_fd) return BAIOS_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(g_fs_mutex);
    VNode* node = resolve(path);
    if (!node) return BAIOS_ERR_NOT_FOUND;
    if (node->is_dir) return BAIOS_ERR_INVALID_ARG;

    /* FD simplificado: usamos o ponteiro como handle (apenas demo) */
    *out_fd = static_cast<u32>(reinterpret_cast<uintptr_t>(node) & 0xFFFFFFFFu);
    return BAIOS_OK;
}

baios_error_t fs_read(u32 fd, void* buf, baios_size_t len, baios_size_t* out_read) {
    if (!buf || !out_read) return BAIOS_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(g_fs_mutex);
    VNode* node = reinterpret_cast<VNode*>(static_cast<uintptr_t>(fd));
    /* Validação fraca — em produção teríamos tabela de FDs real */
    if (!node || node->is_dir) return BAIOS_ERR_INVALID_ARG;

    baios_size_t to_copy = node->data.size();
    if (to_copy > len) to_copy = len;
    std::memcpy(buf, node->data.data(), to_copy);
    *out_read = to_copy;
    return BAIOS_OK;
}

baios_error_t fs_write(u32 fd, const void* buf, baios_size_t len, baios_size_t* out_written) {
    if (!buf || !out_written) return BAIOS_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(g_fs_mutex);
    VNode* node = reinterpret_cast<VNode*>(static_cast<uintptr_t>(fd));
    if (!node || node->is_dir) return BAIOS_ERR_INVALID_ARG;

    node->data.assign(static_cast<const u8*>(buf),
                      static_cast<const u8*>(buf) + len);
    *out_written = len;
    return BAIOS_OK;
}

baios_error_t fs_mkdir(const char* path) {
    if (!path || path[0] != '/') return BAIOS_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(g_fs_mutex);

    /* Separa parent e nome */
    std::string p(path);
    auto pos = p.find_last_of('/');
    std::string parent_path = (pos == 0) ? "/" : p.substr(0, pos);
    std::string name = p.substr(pos + 1);

    if (name.empty()) return BAIOS_ERR_INVALID_ARG;

    VNode* parent = resolve(parent_path.c_str());
    if (!parent || !parent->is_dir) return BAIOS_ERR_NOT_FOUND;
    if (parent->children.count(name)) return BAIOS_ERR_ALREADY_EXISTS;

    parent->children[name] = new (std::nothrow) VNode(name, true);
    if (!parent->children[name]) return BAIOS_ERR_NO_MEMORY;
    return BAIOS_OK;
}

void fs_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_fs_mutex);
    delete g_root;
    g_root = nullptr;
}

} // extern "C"
