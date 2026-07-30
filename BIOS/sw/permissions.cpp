/**
 * permissions.cpp — Controle de capabilities / permissões
 */

#include "../include/permissions.h"
#include "../include/sw_kernel.h"
#include "../include/types.h"

#include <cstring>
#include <cstdio>

#define MAX_PERM_RECORDS 256

static baios_perm_record_t g_perms[MAX_PERM_RECORDS];
static u32 g_perm_count = 0;
static bool g_perm_ready = false;

static baios_perm_record_t *find_record(baios_pid_t pid) {
    for (u32 i = 0; i < g_perm_count; i++) {
        if (g_perms[i].pid == pid) return &g_perms[i];
    }
    return nullptr;
}

extern "C" {

baios_error_t perm_init(void) {
    std::memset(g_perms, 0, sizeof(g_perms));
    g_perm_count = 0;
    g_perm_ready = true;
    return BAIOS_OK;
}

baios_error_t perm_grant(baios_pid_t pid, baios_capability_t cap) {
    if (!g_perm_ready) return BAIOS_ERR_INVALID_ARG;

    baios_perm_record_t *rec = find_record(pid);
    if (!rec) {
        if (g_perm_count >= MAX_PERM_RECORDS) return BAIOS_ERR_NO_MEMORY;
        rec = &g_perms[g_perm_count++];
        rec->pid = pid;
        rec->granted = 0;
        rec->requested = 0;
    }

    rec->granted |= static_cast<u32>(cap);
    rec->requested &= ~static_cast<u32>(cap);
    return BAIOS_OK;
}

baios_error_t perm_revoke(baios_pid_t pid, baios_capability_t cap) {
    baios_perm_record_t *rec = find_record(pid);
    if (!rec) return BAIOS_ERR_NOT_FOUND;
    rec->granted &= ~static_cast<u32>(cap);
    return BAIOS_OK;
}

bool perm_check(baios_pid_t pid, baios_capability_t cap) {
    baios_perm_record_t *rec = find_record(pid);
    if (!rec) {
        /* processo sem registro: usa default do process_manager */
        baios_process_t *p = sw_process_get(pid);
        if (!p) return false;
        return (p->permissions & static_cast<u32>(cap)) != 0 ||
               (p->permissions & BAIOS_CAP_ROOT) != 0;
    }
    return (rec->granted & static_cast<u32>(cap)) != 0 ||
           (rec->granted & BAIOS_CAP_ROOT) != 0;
}

baios_error_t perm_request(baios_pid_t pid, baios_capability_t cap) {
    if (perm_is_dangerous(cap)) {
        /* capabilities perigosas precisam de aprovação explícita */
        baios_perm_record_t *rec = find_record(pid);
        if (!rec) {
            if (g_perm_count >= MAX_PERM_RECORDS) return BAIOS_ERR_NO_MEMORY;
            rec = &g_perms[g_perm_count++];
            rec->pid = pid;
            rec->granted = 0;
            rec->requested = 0;
        }
        rec->requested |= static_cast<u32>(cap);
        return BAIOS_ERR_PERMISSION; /* pendente */
    }
    return perm_grant(pid, cap);
}

baios_error_t perm_get(baios_pid_t pid, baios_perm_set_t *out) {
    if (!out) return BAIOS_ERR_INVALID_ARG;
    baios_perm_record_t *rec = find_record(pid);
    if (rec) {
        *out = rec->granted;
        return BAIOS_OK;
    }
    baios_process_t *p = sw_process_get(pid);
    if (!p) return BAIOS_ERR_NOT_FOUND;
    *out = p->permissions;
    return BAIOS_OK;
}

bool perm_is_dangerous(baios_capability_t cap) {
    return cap == BAIOS_CAP_CAMERA ||
           cap == BAIOS_CAP_MICROPHONE ||
           cap == BAIOS_CAP_LOCATION ||
           cap == BAIOS_CAP_SYSTEM ||
           cap == BAIOS_CAP_ROOT;
}

} // extern "C"
