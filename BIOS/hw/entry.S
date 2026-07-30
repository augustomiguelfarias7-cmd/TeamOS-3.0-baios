/*
 * entry.S — Stub de entrada Assembly do Hardware Microkernel
 *
 * Em um kernel real este arquivo conteria:
 * - Código de bootstrap (multiboot / UEFI handoff)
 * - Configuração inicial de GDT/IDT
 * - Troca para long mode / paging
 * - Chamada para hw_kernel_init()
 *
 * Aqui deixamos apenas um stub documentado para manter
 * a estrutura do Dual Microkernel completa.
 */

#if defined(__x86_64__) || defined(_M_X64)

.section .text
.global hw_entry_asm
.type hw_entry_asm, @function

hw_entry_asm:
    /* Em ambiente real:
     * 1. Desabilita interrupções
     * 2. Configura stack do kernel
     * 3. Chama hw_kernel_init
     * 4. Entra no main_loop
     */
    ret

.size hw_entry_asm, .-hw_entry_asm

#else

/* Fallback para outras arquiteturas / userspace build */
.section .text
.global hw_entry_asm
hw_entry_asm:
    ret

#endif
