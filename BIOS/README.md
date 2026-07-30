# Baios — Dual Microkernel do TeamOS 3.0

**Baios** (Baios Advanced Interface Operating System core) é o kernel de alto nível do TeamOS 3.0.

Arquitetura: **Duplo Microkernel** com comunicação Zero-Copy via memória compartilhada.

```
┌─────────────────────────────────────────────────────────────┐
│                    Aplicações (User Space)                  │
└───────────────────────────┬─────────────────────────────────┘
                            │ Syscalls
┌───────────────────────────▼─────────────────────────────────┐
│              Microkernel de Software (C++)                  │
│  • Process Manager  • Permissions  • VFS  • IPC Router      │
└───────────────────────────┬─────────────────────────────────┘
                            │ Zero-Copy Shared Memory Ring
┌───────────────────────────▼─────────────────────────────────┐
│              Microkernel de Hardware (C)                    │
│  • Phys Memory  • Interrupts  • Driver Compat (Linux)       │
└───────────────────────────┬─────────────────────────────────┘
                            │
                    Hardware / Linux Drivers
```

## Estrutura

```
BIOS/
├── include/          # Headers públicos e internos
├── src/
│   ├── hw/           # Microkernel de Hardware (C)
│   ├── sw/           # Microkernel de Software (C++)
│   ├── ipc/          # Zero-Copy IPC
│   └── boot/         # Entry point e bootstrap
├── Makefile
└── CMakeLists.txt
```

## Build

```bash
cd BIOS
make          # gera libbaios.a e baios_stub
# ou
mkdir build && cd build && cmake .. && make
```

## Status

- [x] Estrutura dual microkernel
- [x] Zero-Copy ring buffer IPC
- [x] Memory manager físico (buddy-like)
- [x] Interrupt framework
- [x] Driver compatibility layer (Linux-style)
- [x] Process & permission manager
- [x] VFS esqueleto
- [ ] Boot real em hardware / QEMU
- [ ] Drivers reais
- [ ] Userspace completo

Licença: Apache 2.0 (mesmo do repositório raiz).
