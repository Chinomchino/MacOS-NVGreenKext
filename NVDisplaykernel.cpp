#include "NVDisplay.h"
#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOService
OSDefineMetaClassAndStructors(NVDisplay, IOService)

bool NVDisplay::start(IOService *provider) {
    IOLog("NVDisplayKext: starting\n");

    IOPCIDevice* device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayKext: provider is not a PCI device\n");
        return false;
    }

    device->retain();
    device->open(this);

    // macOS 15+ uses configRead16 via IOPCIDevice
    UInt16 vendor = 0, deviceId = 0;
    device->configRead16(0x00, &vendor);    // kIOPCIConfigVendorID
    device->configRead16(0x02, &deviceId);  // kIOPCIConfigDeviceID

    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n", vendor, deviceId);

    if (vendor != 0x10DE) {
        IOLog("NVDisplayKext: Not NVIDIA, skipping\n");
        return super::start(provider);
    }

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
        1024 * 768 * 4,  // 1024x768x32bpp
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
    super::stop(provider);
}
