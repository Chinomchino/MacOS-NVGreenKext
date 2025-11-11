#include <Availability.h>

// Step 1: Detect DriverKit headers
#if __has_include(<DriverKit/DriverKit.h>) && __has_include(<PCIDriverKit/IOPCIDevice.h>)
#define NV_HAS_DRIVERKIT 1
#else
#define NV_HAS_DRIVERKIT 0
#endif

// Step 2: Allow CI override
#ifdef SKIP_DRIVERKIT
#undef NV_HAS_DRIVERKIT
#define NV_HAS_DRIVERKIT 0
#endif

// ======================================================================
// DRIVERKIT PATH (local only, full SDK required)
// ======================================================================
#if NV_HAS_DRIVERKIT

#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>

class NVDisplay : public IOService
{
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOPCIDevice *device = nullptr;

public:
    kern_return_t Start(IOService *provider) override {
        IOLog("NVDisplayDext: starting (DriverKit)\n");
        device = OSDynamicCast(IOPCIDevice, provider);
        if (!device) {
            IOLog("NVDisplayDext: provider not a PCI device\n");
            return kIOReturnUnsupported;
        }
        IOLog("NVDisplayDext: Detected PCI device\n");
        return kIOReturnSuccess;
    }

    void Stop(IOService *provider) override {
        IOLog("NVDisplayDext: stopping\n");
    }
};

OSDefineDefaultStructors(NVDisplay)

#else

// ======================================================================
// CLASSIC KEXT PATH (CI and standard Kernel.framework)
// ======================================================================

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService
OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService *provider) {
    IOLog("NVDisplayKext: starting\n");

    IOPCIDevice *device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayKext: provider is not a PCI device\n");
        return false;
    }

    device->retain();
    device->open(this);

    UInt16 vendor = device->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = device->configRead16(kIOPCIConfigDeviceID);

    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    // Optional: only handle NVIDIA
    if (vendor != 0x10DE) {
        IOLog("NVDisplayKext: not NVIDIA, skipping\n");
        return super::start(provider);
    }

    // Enumerate BARs
    uint32_t barCount = device->getDeviceMemoryCount();
    IOLog("NVDisplayKext: Device has %u BARs\n", barCount);
    for (uint32_t i = 0; i < barCount; i++) {
        IODeviceMemory* mem = device->getDeviceMemoryWithIndex(i);
        if (!mem) continue;
        IOLog("NVDisplayKext: BAR[%u] length=%llu phys=0x%llx\n",
              i, mem->getLength(), mem->getPhysicalAddress());
    }

    // Create simple software framebuffer
    IOBufferMemoryDescriptor* fb = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryKernelUserShared | kIOMemoryPageable,
        1024 * 768 * 4,
        page_size
    );

    if (fb) {
        void* fbPtr = fb->getBytesNoCopy();
        memset(fbPtr, 0x00, fb->getLength());
        IOLog("NVDisplayKext: Created software framebuffer @ %p\n", fbPtr);
        fb->release();
    } else {
        IOLog("NVDisplayKext: Failed to allocate fake framebuffer\n");
    }

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

#endif // NV_HAS_DRIVERKIT
