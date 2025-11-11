# ===========================================
#  NVDisplayKext — minimal NVIDIA framebuffer example
#  Build with: make
# ===========================================

KEXT_NAME      := NVDisplayKext
KEXT_BUNDLE_ID := com.example.NVDisplayKext
KEXT_VERSION   := 1.0.0

SRC_FILES      := NVDisplay.cpp
OBJ_FILES      := $(SRC_FILES:.cpp=.o)

SDKROOT        := $(shell xcrun --sdk macosx --show-sdk-path)
CXX            := clang++
LD             := clang++

CXXFLAGS := -Wall -Wextra -Wno-deprecated-declarations \
            -std=c++17 -fno-exceptions -fno-rtti \
            -mkernel -nostdinc \
            -I$(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers \
            -I$(SDKROOT)/System/Library/Extensions/IOKit.framework/Headers

LDFLAGS := -lkmod

KEXT_DIR := $(KEXT_NAME).kext
KEXT_BIN := $(KEXT_DIR)/Contents/MacOS/$(KEXT_NAME)
KEXT_PLIST := $(KEXT_DIR)/Contents/Info.plist

.PHONY: all clean install

all: $(KEXT_BIN)
	@echo "✅ Built $(KEXT_NAME)"

$(KEXT_BIN): $(OBJ_FILES) $(KEXT_PLIST)
	@mkdir -p $(dir $@)
	$(LD) -o $@ $(OBJ_FILES) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(KEXT_PLIST):
	@mkdir -p $(dir $@)
	cp Info.plist $@

clean:
	rm -rf $(OBJ_FILES) $(KEXT_DIR)
	@echo "🧹 Cleaned build files"

install: all
	sudo cp -R $(KEXT_DIR) /Library/Extensions/
	sudo chown -R root:wheel /Library/Extensions/$(KEXT_NAME).kext
	sudo kextcache -i /
