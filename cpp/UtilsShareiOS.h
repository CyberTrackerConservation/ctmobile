#ifndef UTILS_SHARE_IOS_H
#define UTILS_SHARE_IOS_H

#import <UIKit/UIKit.h>

#import "UtilsShare.h"

@interface DocViewController : UIViewController <UIDocumentInteractionControllerDelegate>

@property int requestId;

@property IosShareUtils* m_iosShareUtils;

@end

#endif // UTILS_SHARE_IOS_H
