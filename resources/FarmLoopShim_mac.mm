// FarmLoopShim_mac.mm
#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include "../include/BloonsUIMain.h"   // <-- include your class so self->farmLoop() is known

extern "C" bool BF_EnsureScreenCapturePermission() {
    if (@available(macOS 10.15, *)) {
        if (!CGPreflightScreenCaptureAccess()) {
            return CGRequestScreenCaptureAccess();
        }
    }
    return true;
}

extern "C" void BF_RunFarmLoopWithAutorelease(BloonsUIMain* self) {
    @autoreleasepool {
        self->farmLoop();
    }
}
#endif
