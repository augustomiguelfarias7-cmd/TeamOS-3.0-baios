# Baios — Dual Microkernel do TeamOS 3.0

**Baios** (BIOS folder) é o kernel de alto nível do TeamOS 3.0.

## Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                    Aplicações / Apps                        │
└──────────────────────────┬──────────────────────────────────┘
                           │ Syscalls / Requests
┌──────────────────────────▼──────────────────────────────────┐
│              Software Microkernel (C++)                     │
│  Process Manager • Filesystem • Permissions • Services      │
│  (Audio / Network / Render)                                 │
└──────────────────────────┬──────────────────────────────────┘
                           │ Zero-Copy IPC (Shared Memory)
┌──────────────────────────▼──────────────────────────────────┐
│              Hardware Microkernel (C + Assembly)            │
│  Memory • Interrupts • Power • Linux Driver Compat Layer    │
└──────────────────────────┬──────────────────────────────────┘
                           │
                    Hardware / Linux Drivers
```

## Estrutura de Pastas

```
BIOS/
├── include/          # Headers públicos
├── hw/               # Microkernel de Hardware (C + ASM)
├── sw/               # Microkernel de Software (C++)
│   └── services/     # Serviços de alto nível
├── ipc/              # Zero-Copy IPC e Shared Memory
├── common/           # Utilitários compartilhados
├── Makefile
└── CMakeLists.txt
```

## Build

```bash
cd BIOS
make          # ou cmake -B build && cmake --build build
```

## Princípios

- Isolamento de falhas entre os dois microkernels
- Comunicação exclusivamente via Zero-Copy IPC
- Reaproveitamento de drivers Linux através de camada de compatibilidade
- Aplicações nunca tocam hardware diretamente
