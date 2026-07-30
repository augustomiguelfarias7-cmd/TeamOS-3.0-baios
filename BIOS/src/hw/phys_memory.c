/**
 * @file phys_memory.c
 * @brief Buddy allocator físico para o Microkernel de Hardware
 *
 * Implementação clássica de buddy system com listas por order.
 * Adequada para ambiente de host (simulação) e futura portagem.
 */

#include "memory.h"
#include "baios.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================
 * Estado interno
 * ============================================================ */

static baios_page_t *g_pages = NULL;          /* array de descritores */
static u64           g_total_pages = 0;
static u64           g_phys_base = 0;

static baios_page_t *g_free_lists[BAIOS_BUDDY_MAX_ORDER + 1];
static u64           g_free_count[BAIOS_BUDDY_MAX_ORDER + 1];

static baios_mem_stats_t g_stats;
static bool              g_mm_ready = false;

/* ============================================================
 * Helpers internos
 * ============================================================ */

static inline u64 order_to_pages(u32 order) {
    return 1ULL << order;
}

static void list_add(baios_page_t **head, baios_page_t *page) {
    page->next = *head;
    page->prev = NULL;
    if (*head)
        (*head)->prev = page;
    *head = page;
}

static void list_remove(baios_page_t **head, baios_page_t *page) {
    if (page->prev)
        page->prev->next = page->next;
    else
        *head = page->next;
    if (page->next)
        page->next->prev = page->prev;
    page->next = page->prev = NULL;
}

static baios_page_t *pfn_to_page(u64 pfn) {
    if (pfn >= g_total_pages)
        return NULL;
    return &g_pages[pfn];
}

/* ============================================================
 * Inicialização
 * ============================================================ */

baios_status_t baios_mm_init(u64 phys_base, u64 phys_size) {
    if (g_mm_ready)
        return BAIOS_ERR_STATE;

    if (phys_size < BAIOS_PAGE_SIZE)
        return BAIOS_ERR_INVAL;

    g_phys_base   = BAIOS_ALIGN_DOWN(phys_base, BAIOS_PAGE_SIZE);
    g_total_pages = phys_size >> BAIOS_PAGE_SHIFT;

    if (g_total_pages > BAIOS_MAX_PHYS_PAGES)
        g_total_pages = BAIOS_MAX_PHYS_PAGES;

    g_pages = (baios_page_t *)calloc((size_t)g_total_pages, sizeof(baios_page_t));
    if (!g_pages)
        return BAIOS_ERR_NOMEM;

    memset(g_free_lists, 0, sizeof(g_free_lists));
    memset(g_free_count, 0, sizeof(g_free_count));
    memset(&g_stats, 0, sizeof(g_stats));

    g_stats.total_pages = g_total_pages;

    /* Inicializa todas as páginas como free e monta buddies do maior order possível */
    for (u64 i = 0; i < g_total_pages; i++) {
        g_pages[i].pfn      = i;
        g_pages[i].order     = 0;
        g_pages[i].flags     = BAIOS_PAGE_FLAG_FREE;
        g_pages[i].refcount  = 0;
        g_pages[i].next      = NULL;
        g_pages[i].prev      = NULL;
    }

    /* Coloca blocos do maior order possível na free list */
    u64 remaining = g_total_pages;
    u64 pfn = 0;
    while (remaining > 0) {
        u32 order = BAIOS_BUDDY_MAX_ORDER;
        while (order > 0 && order_to_pages(order) > remaining)
            order--;

        baios_page_t *page = pfn_to_page(pfn);
        page->order = order;
        page->flags = BAIOS_PAGE_FLAG_FREE;
        list_add(&g_free_lists[order], page);
        g_free_count[order]++;

        u64 n = order_to_pages(order);
        pfn += n;
        remaining -= n;
        g_stats.free_pages += n;
    }

    g_mm_ready = true;
    BAIOS_LOG_INFO("mm: initialized %llu pages (%.2f MiB) base=0x%llx",
                   (unsigned long long)g_total_pages,
                   (double)(g_total_pages * BAIOS_PAGE_SIZE) / (1024.0 * 1024.0),
                   (unsigned long long)g_phys_base);
    return BAIOS_OK;
}

void baios_mm_shutdown(void) {
    if (!g_mm_ready)
        return;
    free(g_pages);
    g_pages = NULL;
    g_total_pages = 0;
    g_mm_ready = false;
    memset(g_free_lists, 0, sizeof(g_free_lists));
    memset(g_free_count, 0, sizeof(g_free_count));
    memset(&g_stats, 0, sizeof(g_stats));
    BAIOS_LOG_INFO("mm: shutdown complete");
}

/* ============================================================
 * Alloc / Free
 * ============================================================ */

static baios_page_t *split_block(baios_page_t *page, u32 target_order) {
    while (page->order > target_order) {
        u32 new_order = page->order - 1;
        u64 buddy_pfn = page->pfn + order_to_pages(new_order);
        baios_page_t *buddy = pfn_to_page(buddy_pfn);

        buddy->order = new_order;
        buddy->flags = BAIOS_PAGE_FLAG_FREE;
        buddy->refcount = 0;
        list_add(&g_free_lists[new_order], buddy);
        g_free_count[new_order]++;

        page->order = new_order;
    }
    return page;
}

u64 baios_mm_alloc_pages(u32 order) {
    if (!g_mm_ready || order > BAIOS_BUDDY_MAX_ORDER)
        return (u64)-1;

    for (u32 o = order; o <= BAIOS_BUDDY_MAX_ORDER; o++) {
        if (g_free_lists[o]) {
            baios_page_t *page = g_free_lists[o];
            list_remove(&g_free_lists[o], page);
            g_free_count[o]--;

            page = split_block(page, order);

            page->flags &= ~BAIOS_PAGE_FLAG_FREE;
            page->flags |= BAIOS_PAGE_FLAG_KERNEL;
            page->refcount = 1;

            u64 n = order_to_pages(order);
            g_stats.free_pages -= n;
            g_stats.kernel_pages += n;
            if (g_stats.total_pages - g_stats.free_pages > g_stats.peak_used)
                g_stats.peak_used = g_stats.total_pages - g_stats.free_pages;

            return page->pfn;
        }
    }
    return (u64)-1;
}

void baios_mm_free_pages(u64 pfn, u32 order) {
    if (!g_mm_ready || order > BAIOS_BUDDY_MAX_ORDER)
        return;

    baios_page_t *page = pfn_to_page(pfn);
    if (!page || (page->flags & BAIOS_PAGE_FLAG_FREE))
        return;

    u64 n = order_to_pages(order);
    g_stats.free_pages += n;
    if (page->flags & BAIOS_PAGE_FLAG_KERNEL)
        g_stats.kernel_pages -= n;
    if (page->flags & BAIOS_PAGE_FLAG_USER)
        g_stats.user_pages -= n;

    page->flags = BAIOS_PAGE_FLAG_FREE;
    page->refcount = 0;
    page->order = order;

    /* Merge com buddy enquanto possível */
    while (order < BAIOS_BUDDY_MAX_ORDER) {
        u64 buddy_pfn = pfn ^ order_to_pages(order);
        if (buddy_pfn >= g_total_pages)
            break;

        baios_page_t *buddy = pfn_to_page(buddy_pfn);
        if (!buddy || !(buddy->flags & BAIOS_PAGE_FLAG_FREE) || buddy->order != order)
            break;

        list_remove(&g_free_lists[order], buddy);
        g_free_count[order]--;

        if (buddy_pfn < pfn) {
            page = buddy;
            pfn = buddy_pfn;
        }
        order++;
        page->order = order;
    }

    list_add(&g_free_lists[order], page);
    g_free_count[order]++;
}

void baios_mm_get_stats(baios_mem_stats_t *out) {
    if (out)
        *out = g_stats;
}
