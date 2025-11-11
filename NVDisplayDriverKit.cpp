#include "NVDisplay.h"
#include <DriverKit/DriverKit.h>
#include <PCIDriverKit/IOPCIDevice.h>

OSDeclareDefaultStructors(NVDisplay)

kern_return_t NVDisplay::Start(IOService* provider) {
    IOLog("NVDisplayDext: starting (DriverKit)\n");

    IOPCIDevice* device = OSDynamicCast(IOPCIDevice, provider);
    if (!device) {
        IOLog("NVDisplayDext: provider not a PCI device\n");
        return kIOReturnUnsupported;
    }

    IOLog("NVDisplayDext: Detected PCI device\n");
    return kIOReturnSuccess;
}

void NVDisplay::Stop(IOService* provider) {
    IOLog("NVDisplayDext: stopping\n");
}
