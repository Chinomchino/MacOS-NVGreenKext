#include <Availability.h>

// Detect DriverKit headers
#if __has_include(<DriverKit/DriverKit.h>) && __has_include(<PCIDriverKit/IOPCIDevice.h>)
#define NV_HAS_DRIVERKIT 1
#else
#define NV_HAS_DRIVERKIT 0
#endif

// Force skip DriverKit in CI builds
#ifdef SKIP_DRIVERKIT
#undef NV_HAS_DRIVERKIT
#define NV_HAS_DRIVERKIT 0
#endif

// ---------------- DriverKit path (local dev only) ----------------
#if NV_HAS_DRIVERKIT

#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>

class NVDisplay : public IOService
{
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOPCIDevice* device = nullptr;

public:
    kern_return_t Start(IOService* provider) override {
        IOLog("NVDisplayDext: DriverKit start\n");
        device = OSDynamicCast(IOPCIDevice, provider);
        if (!device) return kIOReturnUnsupported;
        return kIOReturnSuccess;
    }

    void Stop(IOService* provider) override {
        IOLog("NVDisplayDext: DriverKit stop\n");
    }
};

OSDefineDefaultStructors(NVDisplay)

#else

// ---------------- Classic KEXT path (CI-friendly) ----------------
#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService
OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService* provider) {
    IOLog("NVDisplayKext: starting\n");

    IOPCIDevice* device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayKext: not a PCI device\n");
        return false;
    }

    device->retain();
    device->open(this);

    UInt16 vendor = device->configRead16(kIOPCIConfigVendorID);
    UInt16 deviceId = device->configRead16(kIOPCIConfigDeviceID);
    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    // Optional: only NVIDIA
    if (vendor != 0x10DE) return super::start(provider);

    // Enumerate BARs
    for (uint32_t i = 0; i < device->getDeviceMemoryCount(); i++) {
        IODeviceMemory* mem = device->getDeviceMemoryWithIndex(i);
        if (!mem) continue;
        IOLog("BAR[%u] length=%llu phys=0x%llx\n", i, mem->getLength(), mem->getPhysicalAddress());
    }

    // Software framebuffer example
    IOBufferMemoryDescriptor* fb = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryKernelUserShared | kIOMemoryPageable,
        1024*768*4,
        page_size
    );
    if (fb) {
        void* ptr = fb->getBytesNoCopy();
        memset(ptr, 0x00, fb->getLength());
        fb->release();
    }

    return super::start(provider);
}

void NVDisplay::stop(IOService* provider) {
    IOLog("NVDisplayKext: stopping\n");
    super::stop(provider);
}

#endif // NV_HAS_DRIVERKIT
