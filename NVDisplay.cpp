#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>

class NVDisplay : public IOService {
    OSDeclareDefaultStructors(NVDisplay)
private:
    IOPCIDevice *device = nullptr;
public:
    bool start(IOService *provider) override {
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

    void stop(IOService *provider) override {
        IOLog("NVDisplayKext: stopping\n");
        if (device) {
            device->close(this);
            device->release();
            device = nullptr;
        }
        super::stop(provider);
    }
};

OSDefineMetaClassAndStructors(NVDisplay, IOService)
