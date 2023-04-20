ARCH      = x86_64

WORK_DIR  = $(shell pwd)
DST_DIR   = $(WORK_DIR)/build
SRC_DIR   = $(WORK_DIR)/src

SRCS = $(shell	find $(SRC_DIR) -name "*.c")

INCLUDE = -I"$(WORK_DIR)/include"
CFLAGS = -O2 -g -Wall -Werror -Wno-error=unused-variable $(INCLUDE)
CC     = gcc

$(shell	mkdir -p $(DST_DIR))

include $(WORK_DIR)/scripts/$(ARCH).mk

OBJS      = $(addprefix $(DST_DIR)/, $(addsuffix .o, $(notdir $(basename $(SRCS)))))
TARGET    = $(DST_DIR)/coa


$(DST_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $^
	@$(CC) $(CFLAGS) $^ -c -o $@

$(DST_DIR)/%.o: $(ARCH_SRC_DIR)/%.S
	@echo Compiling $^
	@$(CC) $(CFLAGS) $^ -c -o $@

$(TARGET): $(OBJS)
	@echo Linking $@
	@$(CC) $^ -o $@

all: $(TARGET)
run: $(TARGET)
	@$(TARGET)
clean:
	@rm -rf $(DST_DIR)