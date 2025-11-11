#include <Availability.h>

// Skip DriverKit section entirely if headers are missing
#if __has_include(<DriverKit/DriverKit.h>) && __has_include(<PCIDriverKit/IOPCIDevice.h>)
    #define NV_HAS_DRIVERKIT 1
#else
    #define NV_HAS_DRIVERKIT 0
#endif

#if NV_HAS_DRIVERKIT
// ======================================================================
// ✅ DriverKit / New SDK compatibility path
// ======================================================================
#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>
// ... existing DriverKit implementation ...
#else
// ======================================================================
// ✅ Classic KEXT path
// ======================================================================
// ... your IOKit KEXT implementation ...
#endif

#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>

class NVDisplay : public IOService {
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOPCIDevice *device = nullptr;

public:
    kern_return_t Start(IOService *provider) override {
        IOLog("NVDisplayDext: starting (DriverKit path)\n");
        device = OSDynamicCast(IOPCIDevice, provider);
        if (!device) {
            IOLog("NVDisplayDext: provider is not a PCI device\n");
            return kIOReturnUnsupported;
        }

        IOLog("NVDisplayDext: Detected PCI device\n");
        return kIOReturnSuccess;
    }

    void Stop(IOService *provider) override {
        IOLog("NVDisplayDext: stopping\n");
    }
};

OSDefineDefaultStructors(NVDisplay);

#else
// ======================================================================
// ✅ Classic KEXT Path (macOS <= 15.x Kernel.framework)
// ======================================================================

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService
OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService *provider) {
    IOLog("NVDisplayKext: starting\n");
    device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayKext: not a PCI device\n");
        return false;
    }

    device->retain();
    device->open(this);

    UInt16 vendor = device->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = device->configRead16(kIOPCIConfigDeviceID);
    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    // Only proceed if this is NVIDIA (vendor ID 0x10DE)
    if (vendor != 0x10DE) {
        IOLog("NVDisplayKext: not an NVIDIA device, exiting\n");
        return super::start(provider);
    }

    // Enumerate PCI BARs
    uint32_t barCount = device->getDeviceMemoryCount();
    IOLog("NVDisplayKext: Device has %u BARs\n", barCount);
    for (uint32_t i = 0; i < barCount; i++) {
        IODeviceMemory* mem = device->getDeviceMemoryWithIndex(i);
        if (!mem) continue;
        IOLog("NVDisplayKext: BAR[%u] length=%llu phys=0x%llx\n",
              i, mem->getLength(), mem->getPhysicalAddress());
    }

    // Try mapping the first BAR (usually MMIO registers)
    IOMemoryMap *mmio = device->mapDeviceMemoryWithIndex(0);
    if (mmio) {
        volatile UInt32 *regs = (volatile UInt32 *)mmio->getVirtualAddress();
        IOLog("NVDisplayKext: Mapped BAR0 @ %p, first reg=0x%08x\n", regs, regs[0]);
    } else {
        IOLog("NVDisplayKext: Failed to map BAR0\n");
    }

    // Create a fake framebuffer region in system memory
    IOBufferMemoryDescriptor* fb = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryKernelUserShared | kIOMemoryPageable,
        1024 * 768 * 4, // 1024x768x32bpp
        page_size
    );

    if (fb) {
        void* fbPtr = fb->getBytesNoCopy();
        memset(fbPtr, 0x00, fb->getLength());
        // Paint simple test pattern
        UInt32* pixels = (UInt32*)fbPtr;
        for (size_t y = 0; y < 768; y++) {
            for (size_t x = 0; x < 1024; x++) {
                pixels[y * 1024 + x] = ((x ^ y) & 0xFF) << 16; // red XOR gradient
            }
        }
        IOLog("NVDisplayKext: Created software framebuffer @ %p (4MB)\n", fbPtr);
        fb->release();
    } else {
        IOLog("NVDisplayKext: Failed to allocate fake framebuffer\n");
    }

    IOLog("NVDisplayKext: Initialization complete\n");
    return super::start(provider);
}

void NVDisplay::stop(IOService *provider) {
    IOLog("NVDisplayKext: stopping\n");
    if (device) {
        device->close(this);
        device->release();
        device = nullptr;
    }
    super::stop(provider);
}

#endif

