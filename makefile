SDKROOT := $(shell xcrun --sdk macosx --show-sdk-path)
KEXT := NVDisplay.kext/Contents/MacOS/NVDisplay
SRC := NVDisplayKernel.cpp
CXXFLAGS := -Wall -Wextra -std=c++17 -fno-exceptions -fno-rtti -DSKIP_DRIVERKIT \
            -nostdinc \
            -I"$(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers" \
            -I"$(SDKROOT)/System/Library/Extensions/IOKit.framework/Headers"

all: $(KEXT)

$(KEXT): $(SRC)
	mkdir -p NVDisplay.kext/Contents/MacOS
	clang++ $(CXXFLAGS) -c $(SRC) -o NVDisplay.kext/Contents/MacOS/NVDisplay.o
	clang -r NVDisplay.kext/Contents/MacOS/NVDisplay.o -o NVDisplay.kext/Contents/MacOS/NVDisplay

clean:
	rm -rf NVDisplay.kext
