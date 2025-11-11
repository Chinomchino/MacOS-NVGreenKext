#ifndef NVDISPLAY_H
#define NVDISPLAY_H

#if defined(__DRIVERKIT__)
#include <DriverKit/DriverKit.h>
#else
#include <IOKit/IOService.h>
#endif

class NVDisplay : public IOService {
public:
    #if defined(__DRIVERKIT__)
    kern_return_t Start(IOService* provider) override;
    void Stop(IOService* provider) override;
    #else
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    #endif
};

#endif // NVDISPLAY_H
