#ifndef BAIOS_MEMORY_H
#define BAIOS_MEMORY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Região de memória física ou virtual */
typedef struct baios_mem_region {
    baios_addr_t base;
    baios_size_t size;
    u32          flags;
    const char  *name;
    struct baios_mem_region *next;
} baios_mem_region_t;

/* Descritor de página */
typedef struct {
    baios_addr_t phys;
    baios_addr_t virt;
    u32          flags;
    u32          refcount;
    baios_pid_t  owner;
} baios_page_t;

/* Pool de alocação rápida (buddy-like simplificado) */
#define BAIOS_BUDDY_ORDERS 12  /* até 4MB (2^12 * 4KB) */

typedef struct {
    baios_addr_t free_list[BAIOS_BUDDY_ORDERS];
    u64          free_count[BAIOS_BUDDY_ORDERS];
    u64          total_pages;
    u64          used_pages;
    baios_addr_t heap_base;
    baios_size_t heap_size;
} baios_buddy_t;

/* API pública de memória (usada por ambos microkernels) */
baios_error_t baios_mem_init(baios_addr_t heap_base, baios_size_t heap_size);
void         *baios_kmalloc(baios_size_t size);
void         *baios_kcalloc(baios_size_t n, baios_size_t size);
void          baios_kfree(void *ptr);
void         *baios_krealloc(void *ptr, baios_size_t new_size);

baios_error_t baios_map_page(baios_addr_t virt, baios_addr_t phys, u32 flags);
baios_error_t baios_unmap_page(baios_addr_t virt);
baios_error_t baios_protect_page(baios_addr_t virt, u32 flags);

baios_page_t *baios_get_page(baios_addr_t virt);
baios_error_t baios_pin_page(baios_addr_t virt);
baios_error_t baios_unpin_page(baios_addr_t virt);

/* Shared memory regions (base do Zero-Copy) */
typedef struct {
    baios_handle_t handle;
    baios_addr_t   base;
    baios_size_t   size;
    u32            flags;
    baios_pid_t    creator;
    u32            refcount;
    char           name[64];
} baios_shm_region_t;

baios_error_t baios_shm_create(const char *name, baios_size_t size,
                               u32 flags, baios_handle_t *out_handle);
baios_error_t baios_shm_attach(baios_handle_t handle, baios_addr_t *out_base);
baios_error_t baios_shm_detach(baios_handle_t handle);
baios_error_t baios_shm_destroy(baios_handle_t handle);
baios_error_t baios_shm_find(const char *name, baios_handle_t *out_handle);

/* Estatísticas de memória */
typedef struct {
    u64 total_bytes;
    u64 used_bytes;
    u64 free_bytes;
    u64 shared_bytes;
    u64 peak_used;
    u32 region_count;
    u32 page_faults;
} baios_mem_stats_t;

baios_error_t baios_mem_stats(baios_mem_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_MEMORY_H */
