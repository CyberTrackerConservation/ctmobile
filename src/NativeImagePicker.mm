#include "NativeImagePicker.h"
#import <UIKit/UIKit.h>
#import <QtGui/QGuiApplication>
#import <QtCore/QUrl>

@interface ImagePickerDelegate : NSObject <UIImagePickerControllerDelegate, UINavigationControllerDelegate>
@property (nonatomic, assign) NativeImagePicker *picker;
@end

@implementation ImagePickerDelegate

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
    UIImage *image = info[UIImagePickerControllerOriginalImage];
    if (image) {
        NSString *tmp = [NSTemporaryDirectory() stringByAppendingPathComponent:@"selected_image.jpg"];
        NSData *data = UIImageJPEGRepresentation(image, 1.0);
        [data writeToFile:tmp atomically:YES];
        emit self.picker->accepted("file:///" + QString::fromNSString(tmp));
    }
    [picker dismissViewControllerAnimated:YES completion:nil];
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
    emit self.picker->rejected();
    [picker dismissViewControllerAnimated:YES completion:nil];
}

@end

NativeImagePicker::NativeImagePicker(QObject *parent) : QObject(parent)
{
}

void NativeImagePicker::open()
{
    UIImagePickerController *controller = [[UIImagePickerController alloc] init];
    controller.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;

    ImagePickerDelegate *delegate = [[ImagePickerDelegate alloc] init];
    delegate.picker = this;
    controller.delegate = delegate;

    UIViewController *rootVC = UIApplication.sharedApplication.keyWindow.rootViewController;
    [rootVC presentViewController:controller animated:YES completion:nil];
}
