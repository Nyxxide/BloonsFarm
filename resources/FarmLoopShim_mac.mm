// FarmLoopShim_mac.mm

#if defined(__APPLE__)

#include "../include/BloonsUIMain.h"

#if defined(__OBJC__)
#pragma push_macro("NO")
#pragma push_macro("YES")
#undef NO
#undef YES
#endif

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>

#if defined(__OBJC__)
#pragma pop_macro("YES")
#pragma pop_macro("NO")
#endif

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
