#ifndef UTILS_SHARE_IOS_H
#define UTILS_SHARE_IOS_H

#import <UIKit/UIKit.h>
#import "UtilsPlatform.h"

@interface DocViewController : UIViewController <UIDocumentInteractionControllerDelegate>

@property int requestId;
@property QString mediaFilename;

@property PlatformUtils* m_iosUtils;

@end

#endif // UTILS_SHARE_IOS_H
