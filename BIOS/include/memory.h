/**
 * @file memory.h
 * @brief Gerenciador de memória física (buddy allocator) — Microkernel de Hardware
 */
#ifndef BAIOS_MEMORY_H
#define BAIOS_MEMORY_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Página física
 * ============================================================ */
typedef struct baios_page {
    u64           pfn;          /* page frame number */
    u32           order;        /* buddy order (0 = 4K) */
    u32           flags;
    u32           refcount;
    struct baios_page *next;
    struct baios_page *prev;
} baios_page_t;

#define BAIOS_PAGE_FLAG_FREE     (1u << 0)
#define BAIOS_PAGE_FLAG_RESERVED (1u << 1)
#define BAIOS_PAGE_FLAG_KERNEL   (1u << 2)
#define BAIOS_PAGE_FLAG_USER     (1u << 3)
#define BAIOS_PAGE_FLAG_DMA      (1u << 4)

/* ============================================================
 * Estatísticas de memória
 * ============================================================ */
typedef struct {
    u64 total_pages;
    u64 free_pages;
    u64 reserved_pages;
    u64 kernel_pages;
    u64 user_pages;
    u64 peak_used;
} baios_mem_stats_t;

/* ============================================================
 * API pública do memory manager
 * ============================================================ */

baios_status_t baios_mm_init(u64 phys_base, u64 phys_size);
void           baios_mm_shutdown(void);

/** Aloca 2^order páginas contíguas. Retorna PFN ou (u64)-1 em erro. */
u64  baios_mm_alloc_pages(u32 order);
void baios_mm_free_pages(u64 pfn, u32 order);

/** Atalhos para 1 página (4K) */
static inline u64  baios_mm_alloc_page(void) { return baios_mm_alloc_pages(0); }
static inline void baios_mm_free_page(u64 pfn) { baios_mm_free_pages(pfn, 0); }

/** Converte PFN <-> endereço físico */
static inline u64 baios_pfn_to_phys(u64 pfn) {
    return pfn << BAIOS_PAGE_SHIFT;
}
static inline u64 baios_phys_to_pfn(u64 phys) {
    return phys >> BAIOS_PAGE_SHIFT;
}

void baios_mm_get_stats(baios_mem_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_MEMORY_H */
