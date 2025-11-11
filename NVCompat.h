#pragma once

// Detect DriverKit vs Kernel KEXT
#if defined(__DRIVERKIT__)
    #define NV_USE_DRIVERKIT 1
#else
    #define NV_USE_DRIVERKIT 0
#endif

// Detect new vs old SDK layout
#if __has_include(<PCIDriverKit/IOPCIDevice.h>)
    #define NV_HAS_PCIDRIVERKIT 1
#else
    #define NV_HAS_PCIDRIVERKIT 0
#endif

// Choose correct include paths
#if NV_USE_DRIVERKIT || NV_HAS_PCIDRIVERKIT
    // New SDK (DriverKit)
    #include <DriverKit/DriverKit.h>
    #include <PCIDriverKit/IOPCIDevice.h>
#else
    // Legacy kernel IOKit (KEXT)
    #include <IOKit/IOLib.h>
    #include <IOKit/pci/IOPCIDevice.h>
#endif
