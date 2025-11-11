#define SKIP_DRIVERKIT 1

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService
class NVDisplay : public IOService {
    OSDeclareDefaultStructors(NVDisplay)
    
private:
    IOPCIDevice* device = nullptr;

public:
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
};

OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService* provider) {
    IOLog("NVDisplayKext: starting\n");

    device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayKext: provider is not a PCI device\n");
        return false;
    }

    device->retain();
    device->open(this);

    UInt16 vendor = 0, deviceId = 0;

    // Modern macOS 15+ way to read PCI config space
    if (device->getDeviceMemoryCount() > 0) {
        IOMemoryMap* cfgMap = device->mapDeviceMemoryWithIndex(0);
        if (cfgMap) {
            volatile UInt8* cfg = (volatile UInt8*)cfgMap->getVirtualAddress();
            vendor = *(volatile UInt16*)(cfg + 0x00);
            deviceId = *(volatile UInt16*)(cfg + 0x02);
            cfgMap->release();
        }
    }

    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    // Map first BAR
    IOMemoryMap* mmio = device->mapDeviceMemoryWithIndex(0);
    if (mmio) {
        volatile UInt32* regs = (volatile UInt32*)mmio->getVirtualAddress();
        IOLog("NVDisplayKext: Mapped BAR0 @ %p, first reg=0x%08x\n", regs, regs[0]);
    } else {
        IOLog("NVDisplayKext: Failed to map BAR0\n");
    }

    // Allocate dummy framebuffer
    IOBufferMemoryDescriptor* fb = IOBufferMemoryDescriptor::inTaskWithOptions(
        kernel_task,
        kIOMemoryKernelUserShared,
        1024 * 768 * 4,
        page_size
    );

    if (fb) {
        void* fbPtr = fb->getBytesNoCopy();
        memset(fbPtr, 0x00, fb->getLength());
        IOLog("NVDisplayKext: Created software framebuffer @ %p\n", fbPtr);
        fb->release();
    }

    IOLog("NVDisplayKext: Initialization complete\n");
    return super::start(provider);
}

void NVDisplay::stop(IOService* provider) {
    IOLog("NVDisplayKext: stopping\n");
    if (device) {
        device->close(this);
        device->release();
        device = nullptr;
    }
    super::stop(provider);
}

