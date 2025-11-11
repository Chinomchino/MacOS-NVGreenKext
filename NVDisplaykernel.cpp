#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService

class NVDisplay : public IOService {
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOService* providerDevice = nullptr;

public:
    virtual bool start(IOService* provider) override;
    virtual void stop(IOService* provider) override;
};

OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService* provider) {
    IOLog("NVDisplayKext: starting\n");

    providerDevice = provider;
    providerDevice->retain();
    providerDevice->open(this);

    UInt16 vendor = 0, deviceId = 0;

    // Read PCI config space via first BAR mapping
    if (providerDevice->getDeviceMemoryCount() > 0) {
        IOMemoryMap* cfgMap = providerDevice->mapDeviceMemoryWithIndex(0);
        if (cfgMap) {
            volatile UInt8* cfg = (volatile UInt8*)cfgMap->getVirtualAddress();
            vendor = *(volatile UInt16*)(cfg + 0x00);
            deviceId = *(volatile UInt16*)(cfg + 0x02);
            cfgMap->release();
        }
    }

    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    // Allocate fake framebuffer
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
    if (providerDevice) {
        providerDevice->close(this);
        providerDevice->release();
        providerDevice = nullptr;
    }
    super::stop(provider);
}
