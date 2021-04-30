#import "requestmicpermission.h"
#import "AVFoundation/AVFoundation.h"

bool RequestMicPermission::openMic()
{
    if (@available(macOS 10.14, *)) {

        AVAuthorizationStatus st = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio];
        if (st == AVAuthorizationStatusAuthorized) {
          return true;
        }

        dispatch_group_t group = dispatch_group_create();

        __block bool accessGranted = false;

        if (st != AVAuthorizationStatusAuthorized) {
          dispatch_group_enter(group);
          [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {

              accessGranted = granted;
              NSLog(@"Granted!");
              dispatch_group_leave(group);
            }];
        }

        dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)));

        return accessGranted;
    } else  {
        return true;
    }
}
