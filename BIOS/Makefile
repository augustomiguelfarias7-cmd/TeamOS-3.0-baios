# Baios Dual Microkernel — Makefile
# Build host stub (Linux userspace simulation)

CC      ?= gcc
CXX     ?= g++
AR      ?= ar
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -g -Iinclude -DBAIOS_DEBUG=1
CXXFLAGS?= -std=c++17 -Wall -Wextra -O2 -g -Iinclude -DBAIOS_DEBUG=1
LDFLAGS ?= -lpthread

BUILD   := build
LIB     := $(BUILD)/libbaios.a
BIN     := $(BUILD)/baios_stub

C_SRCS  := \
	src/hw/phys_memory.c \
	src/hw/interrupt.c \
	src/hw/driver_compat.c \
	src/hw/hw_kernel.c \
	src/ipc/zero_copy.c \
	src/sw/process_manager.c \
	src/boot/entry.c

CXX_SRCS := \
	src/sw/sw_kernel.cpp

C_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(C_SRCS))
CXX_OBJS := $(patsubst %.cpp,$(BUILD)/%.o,$(CXX_SRCS))
OBJS     := $(C_OBJS) $(CXX_OBJS)

.PHONY: all clean dirs run

all: dirs $(BIN)

dirs:
	@mkdir -p $(BUILD)/src/hw $(BUILD)/src/sw $(BUILD)/src/ipc $(BUILD)/src/boot

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB): $(filter-out $(BUILD)/src/boot/entry.o,$(OBJS))
	$(AR) rcs $@ $^

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BUILD)

# Contagem aproximada de linhas
loc:
	@find include src -name '*.h' -o -name '*.c' -o -name '*.cpp' | xargs wc -l
