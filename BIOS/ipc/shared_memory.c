/**
 * shared_memory.c — Gerenciador de regiões de memória compartilhada
 *
 * Base do Zero-Copy IPC do Baios.
 * Todas as regiões são alocadas pelo Hardware Microkernel e
 * mapeadas nos dois lados (HW e SW).
 */

#include "../include/memory.h"
#include "../include/ipc.h"
#include "../include/types.h"

#include <string.h>
#include <stdio.h>

#define MAX_SHM_REGIONS 64

static baios_shm_region_t g_shm_table[MAX_SHM_REGIONS];
static u32 g_shm_count = 0;
static baios_handle_t g_next_handle = 1;
static bool g_shm_initialized = false;

/* Simulação de heap para ambientes de desenvolvimento (userspace) */
static u8 g_sim_heap[16 * 1024 * 1024]; /* 16 MB simulados */
static baios_size_t g_sim_heap_used = 0;

static void *sim_alloc(baios_size_t size) {
    size = BAIOS_ALIGN_UP(size, BAIOS_PAGE_SIZE);
    if (g_sim_heap_used + size > sizeof(g_sim_heap)) {
        return NULL;
    }
    void *ptr = &g_sim_heap[g_sim_heap_used];
    g_sim_heap_used += size;
    memset(ptr, 0, size);
    return ptr;
}

baios_error_t baios_shm_create(const char *name, baios_size_t size,
                               u32 flags, baios_handle_t *out_handle) {
    if (!name || size == 0 || !out_handle) {
        return BAIOS_ERR_INVALID_ARG;
    }
    if (g_shm_count >= MAX_SHM_REGIONS) {
        return BAIOS_ERR_NO_MEMORY;
    }

    /* Verifica se já existe região com o mesmo nome */
    for (u32 i = 0; i < g_shm_count; i++) {
        if (strcmp(g_shm_table[i].name, name) == 0) {
            return BAIOS_ERR_ALREADY_EXISTS;
        }
    }

    void *base = sim_alloc(size);
    if (!base) {
        return BAIOS_ERR_NO_MEMORY;
    }

    baios_shm_region_t *reg = &g_shm_table[g_shm_count++];
    reg->handle   = g_next_handle++;
    reg->base     = (baios_addr_t)base;
    reg->size     = size;
    reg->flags    = flags | BAIOS_MEM_SHARED | BAIOS_MEM_ZERO_COPY;
    reg->creator  = 0; /* kernel */
    reg->refcount = 1;
    strncpy(reg->name, name, sizeof(reg->name) - 1);
    reg->name[sizeof(reg->name) - 1] = '\0';

    *out_handle = reg->handle;
    return BAIOS_OK;
}

static baios_shm_region_t *find_by_handle(baios_handle_t handle) {
    for (u32 i = 0; i < g_shm_count; i++) {
        if (g_shm_table[i].handle == handle) {
            return &g_shm_table[i];
        }
    }
    return NULL;
}

baios_error_t baios_shm_attach(baios_handle_t handle, baios_addr_t *out_base) {
    if (!out_base) return BAIOS_ERR_INVALID_ARG;
    baios_shm_region_t *reg = find_by_handle(handle);
    if (!reg) return BAIOS_ERR_NOT_FOUND;

    reg->refcount++;
    *out_base = reg->base;
    return BAIOS_OK;
}

baios_error_t baios_shm_detach(baios_handle_t handle) {
    baios_shm_region_t *reg = find_by_handle(handle);
    if (!reg) return BAIOS_ERR_NOT_FOUND;

    if (reg->refcount > 0) {
        reg->refcount--;
    }
    return BAIOS_OK;
}

baios_error_t baios_shm_destroy(baios_handle_t handle) {
    baios_shm_region_t *reg = find_by_handle(handle);
    if (!reg) return BAIOS_ERR_NOT_FOUND;

    if (reg->refcount > 1) {
        return BAIOS_ERR_BUSY;
    }

    /* Em kernel real liberaríamos as páginas físicas.
     * Aqui apenas marcamos como inválido. */
    reg->handle = 0;
    reg->base   = 0;
    reg->size   = 0;
    reg->refcount = 0;
    reg->name[0] = '\0';

    return BAIOS_OK;
}

baios_error_t baios_shm_find(const char *name, baios_handle_t *out_handle) {
    if (!name || !out_handle) return BAIOS_ERR_INVALID_ARG;

    for (u32 i = 0; i < g_shm_count; i++) {
        if (g_shm_table[i].handle != 0 &&
            strcmp(g_shm_table[i].name, name) == 0) {
            *out_handle = g_shm_table[i].handle;
            return BAIOS_OK;
        }
    }
    return BAIOS_ERR_NOT_FOUND;
}

baios_error_t baios_mem_init(baios_addr_t heap_base, baios_size_t heap_size) {
    (void)heap_base;
    (void)heap_size;
    g_sim_heap_used = 0;
    g_shm_count = 0;
    g_next_handle = 1;
    g_shm_initialized = true;
    return BAIOS_OK;
}

void *baios_kmalloc(baios_size_t size) {
    if (size == 0) return NULL;
    return sim_alloc(size);
}

void *baios_kcalloc(baios_size_t n, baios_size_t size) {
    baios_size_t total = n * size;
    void *p = baios_kmalloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void baios_kfree(void *ptr) {
    (void)ptr; /* em simulação não liberamos de volta ao heap linear */
}

void *baios_krealloc(void *ptr, baios_size_t new_size) {
    if (!ptr) return baios_kmalloc(new_size);
    if (new_size == 0) {
        baios_kfree(ptr);
        return NULL;
    }
    void *n = baios_kmalloc(new_size);
    if (n && ptr) {
        /* não sabemos o tamanho antigo — cópia parcial segura */
        memcpy(n, ptr, new_size);
    }
    return n;
}

baios_error_t baios_mem_stats(baios_mem_stats_t *out) {
    if (!out) return BAIOS_ERR_INVALID_ARG;
    out->total_bytes  = sizeof(g_sim_heap);
    out->used_bytes   = g_sim_heap_used;
    out->free_bytes   = sizeof(g_sim_heap) - g_sim_heap_used;
    out->shared_bytes = 0;
    for (u32 i = 0; i < g_shm_count; i++) {
        if (g_shm_table[i].handle) {
            out->shared_bytes += g_shm_table[i].size;
        }
    }
    out->peak_used    = g_sim_heap_used;
    out->region_count = g_shm_count;
    out->page_faults  = 0;
    return BAIOS_OK;
}
