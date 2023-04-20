ARCH_SRCS = $(shell	find $(SRC_DIR)/arch/x86_64 -name "*.S")
ARCH_SRC_DIR = $(SRC_DIR)/arch/x86_64
SRCS += $(ARCH_SRCS)