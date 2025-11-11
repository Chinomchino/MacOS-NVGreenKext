# ===========================================
#  NVDisplayKext — minimal NVIDIA framebuffer example
#  Build with: make
# ===========================================
SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path)
KEXT_DIR = NVDisplay.kext
OBJ = $(KEXT_DIR)/Contents/MacOS/NVDisplay.o
BIN = $(KEXT_DIR)/Contents/MacOS/NVDisplay

$(OBJ): NVDisplay.cpp
	mkdir -p $(dir $@)
	clang++ -Wall -Wextra -std=c++17 -fno-exceptions -fno-rtti \
		-nostdinc \
		-I"$(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers" \
		-I"$(SDKROOT)/System/Library/Extensions/IOKit.framework/Headers" \
		-c $< -o $@

$(BIN): $(OBJ)
	clang -r $< -o $@

all: $(BIN)
	@echo "✅ Built $(BIN)"

clean:
	rm -rf $(KEXT_DIR)

