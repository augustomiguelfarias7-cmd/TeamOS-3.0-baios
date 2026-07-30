# Baios Dual Microkernel — Makefile
# TeamOS 3.0

CC      ?= gcc
CXX     ?= g++
AS      ?= as
AR      ?= ar

CFLAGS  := -std=c11 -Wall -Wextra -O2 -g -Iinclude -fno-strict-aliasing
CXXFLAGS:= -std=c++17 -Wall -Wextra -O2 -g -Iinclude -fno-strict-aliasing
LDFLAGS := -lpthread -lrt

# Fontes C (Hardware + IPC + common)
C_SRCS := \
	ipc/shared_memory.c \
	ipc/zero_copy.c \
	hw/memory.c \
	hw/interrupts.c \
	hw/power.c \
	hw/linux_compat.c \
	hw/main.c \
	common/utils.c \
	demo_main.c

# Fontes C++ (Software Microkernel)
CXX_SRCS := \
	sw/process_manager.cpp \
	sw/filesystem.cpp \
	sw/permissions.cpp \
	sw/ipc_bridge.cpp \
	sw/services/audio.cpp \
	sw/services/network.cpp \
	sw/services/render.cpp \
	sw/main.cpp

# Assembly
ASM_SRCS := hw/entry.S

C_OBJS   := $(C_SRCS:.c=.o)
CXX_OBJS := $(CXX_SRCS:.cpp=.o)
ASM_OBJS := $(ASM_SRCS:.S=.o)

OBJS := $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS)

TARGET := baios_demo

.PHONY: all clean run info

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "Build concluído: ./$(TARGET)"
	@echo "Execute com: make run"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Limpeza concluída."

info:
	@echo "Baios Dual Microkernel — TeamOS 3.0"
	@echo "Fontes C:   $(words $(C_SRCS)) arquivos"
	@echo "Fontes C++: $(words $(CXX_SRCS)) arquivos"
	@echo "Assembly:   $(words $(ASM_SRCS)) arquivos"
	@echo "Total objs: $(words $(OBJS))"
