#include <Availability.h>

// Detect DriverKit vs Kernel
#if defined(__DRIVERKIT__)
    #define NV_USE_DRIVERKIT 1
#else
    #define NV_USE_DRIVERKIT 0
#endif

// Detect if the SDK provides DriverKit headers in Kernel.framework
#if __has_include(<PCIDriverKit/IOPCIDevice.h>)
    #define NV_HAS_PCIDRIVERKIT 1
#else
    #define NV_HAS_PCIDRIVERKIT 0
#endif

#if NV_USE_DRIVERKIT || NV_HAS_PCIDRIVERKIT
// ======================================================================
// ✅ DriverKit / New SDK compatibility path
// ======================================================================

#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>

class NVDisplay : public IOService
{
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOPCIDevice *device = nullptr;

public:
    kern_return_t Start(IOService *provider) override {
        IOLog("NVDisplayDext: starting\n");
        device = OSDynamicCast(IOPCIDevice, provider);
        if (!device) {
            IOLog("NVDisplayDext: not a PCI device\n");
            return kIOReturnUnsupported;
        }

        IOLog("NVDisplayDext: found PCI device (DriverKit)\n");
        return kIOReturnSuccess;
    }

    void Stop(IOService *provider) override {
        IOLog("NVDisplayDext: stopping\n");
    }
};

OSDefineDefaultStructors(NVDisplay);

#else
// ======================================================================
// ✅ Classic KEXT path
// ======================================================================

#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>

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
    IOLog("NVDisplayKext: vendor=0x%x device=0x%x\n",
          device->configRead16(kIOPCIConfigVendorID),
          device->configRead16(kIOPCIConfigDeviceID));

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
