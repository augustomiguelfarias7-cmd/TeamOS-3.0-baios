/**
 * hw/memory.c — Gerenciador de memória física do Hardware Microkernel
 *
 * Implementa um buddy allocator simplificado + mapeamento de páginas.
 * Em ambiente real rodaria em modo privilegiado; aqui é simulação.
 */

#include "../include/hw_kernel.h"
#include "../include/memory.h"
#include "../include/types.h"

#include <string.h>
#include <stdio.h>

static baios_buddy_t g_buddy;
static bool g_mem_ready = false;

/* Tabela simples de páginas mapeadas (simulação) */
#define MAX_MAPPED_PAGES 1024
static baios_page_t g_pages[MAX_MAPPED_PAGES];
static u32 g_page_count = 0;

static u32 order_from_size(baios_size_t size) {
    u32 order = 0;
    baios_size_t s = BAIOS_PAGE_SIZE;
    while (s < size && order < BAIOS_BUDDY_ORDERS - 1) {
        s <<= 1;
        order++;
    }
    return order;
}

baios_error_t hw_mem_init_buddy(baios_addr_t base, baios_size_t size) {
    memset(&g_buddy, 0, sizeof(g_buddy));
    g_buddy.heap_base = base;
    g_buddy.heap_size = size;
    g_buddy.total_pages = size / BAIOS_PAGE_SIZE;
    g_buddy.used_pages = 0;

    /* Coloca todo o heap no maior order possível */
    u32 max_order = 0;
    baios_size_t s = BAIOS_PAGE_SIZE;
    while ((s << 1) <= size && max_order < BAIOS_BUDDY_ORDERS - 1) {
        s <<= 1;
        max_order++;
    }

    g_buddy.free_list[max_order] = base;
    g_buddy.free_count[max_order] = 1;

    g_mem_ready = true;
    return BAIOS_OK;
}

baios_error_t hw_mem_alloc_pages(u32 order, baios_addr_t *out_phys) {
    if (!g_mem_ready || !out_phys || order >= BAIOS_BUDDY_ORDERS) {
        return BAIOS_ERR_INVALID_ARG;
    }

    /* Procura o menor order >= solicitado que tenha bloco livre */
    for (u32 o = order; o < BAIOS_BUDDY_ORDERS; o++) {
        if (g_buddy.free_count[o] > 0) {
            baios_addr_t block = g_buddy.free_list[o];
            g_buddy.free_list[o] = 0; /* simplificado: só um bloco por order */
            g_buddy.free_count[o]--;

            /* Quebra blocos maiores até chegar no order pedido */
            while (o > order) {
                o--;
                baios_addr_t buddy = block + ((baios_addr_t)BAIOS_PAGE_SIZE << o);
                g_buddy.free_list[o] = buddy;
                g_buddy.free_count[o]++;
            }

            g_buddy.used_pages += (1ULL << order);
            *out_phys = block;
            return BAIOS_OK;
        }
    }
    return BAIOS_ERR_NO_MEMORY;
}

baios_error_t hw_mem_free_pages(baios_addr_t phys, u32 order) {
    if (!g_mem_ready || order >= BAIOS_BUDDY_ORDERS) {
        return BAIOS_ERR_INVALID_ARG;
    }

    /* Versão simplificada: apenas devolve para a lista do order */
    g_buddy.free_list[order] = phys;
    g_buddy.free_count[order]++;
    g_buddy.used_pages -= (1ULL << order);
    return BAIOS_OK;
}

baios_error_t hw_mem_map_physical(baios_addr_t phys, baios_size_t size,
                                  u32 flags, baios_addr_t *out_virt) {
    if (!out_virt || size == 0) return BAIOS_ERR_INVALID_ARG;
    if (g_page_count >= MAX_MAPPED_PAGES) return BAIOS_ERR_NO_MEMORY;

    /* Em simulação virt == phys (identity map) */
    baios_page_t *pg = &g_pages[g_page_count++];
    pg->phys     = phys;
    pg->virt     = phys;
    pg->flags    = flags;
    pg->refcount = 1;
    pg->owner    = 0;

    *out_virt = phys;
    return BAIOS_OK;
}

baios_error_t baios_map_page(baios_addr_t virt, baios_addr_t phys, u32 flags) {
    return hw_mem_map_physical(phys, BAIOS_PAGE_SIZE, flags, &virt);
}

baios_error_t baios_unmap_page(baios_addr_t virt) {
    for (u32 i = 0; i < g_page_count; i++) {
        if (g_pages[i].virt == virt) {
            g_pages[i].refcount = 0;
            g_pages[i].virt = 0;
            return BAIOS_OK;
        }
    }
    return BAIOS_ERR_NOT_FOUND;
}

baios_page_t *baios_get_page(baios_addr_t virt) {
    for (u32 i = 0; i < g_page_count; i++) {
        if (g_pages[i].virt == virt && g_pages[i].refcount > 0) {
            return &g_pages[i];
        }
    }
    return NULL;
}

baios_error_t baios_pin_page(baios_addr_t virt) {
    baios_page_t *pg = baios_get_page(virt);
    if (!pg) return BAIOS_ERR_NOT_FOUND;
    pg->refcount++;
    return BAIOS_OK;
}

baios_error_t baios_unpin_page(baios_addr_t virt) {
    baios_page_t *pg = baios_get_page(virt);
    if (!pg) return BAIOS_ERR_NOT_FOUND;
    if (pg->refcount > 0) pg->refcount--;
    return BAIOS_OK;
}
