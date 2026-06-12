#import "UtilsPlatformiOS.h"
#import <UIKit/UIKit.h>
#import <Photos/Photos.h>
#import <QDesktopServices>
#import <QFileInfo>
#import <QGuiApplication>
#import <QUrl>
#import <QDir>
#import <QDebug>

#import <UIKit/UIDocumentInteractionController.h>
#import <Foundation/Foundation.h>

using namespace CyberTracker;

@interface DocViewController ()
@end
@implementation DocViewController
#pragma mark -
#pragma mark View Life Cycle
- (void)viewDidLoad {
    [super viewDidLoad];
}
#pragma mark -
#pragma mark Document Interaction Controller Delegate Methods
- (UIViewController *) documentInteractionControllerViewControllerForPreview: (UIDocumentInteractionController *) controller {
#pragma unused (controller)
    return self;
}
- (void)documentInteractionControllerDidEndPreview:(UIDocumentInteractionController *)controller
{
#pragma unused (controller)
    [self removeFromParentViewController];
}

- (void)image:(UIImage *)image didFinishSavingWithError:(NSError *)error contextInfo:(void *)contextInfo {
    if (error) {
        NSLog(@"Error Saving Image: %@", error.localizedDescription);
        return;
    }

    // Fetch the most recent image asset
    PHFetchOptions *fetchOptions = [[PHFetchOptions alloc] init];
    fetchOptions.sortDescriptors = @[[NSSortDescriptor sortDescriptorWithKey:@"creationDate" ascending:NO]];
    fetchOptions.fetchLimit = 1;
    PHFetchResult *fetchResult = [PHAsset fetchAssetsWithMediaType:PHAssetMediaTypeImage options:fetchOptions];

    if (fetchResult.count > 0) {
        PHAsset *lastAsset = fetchResult.firstObject;
        NSString *localIdentifier = lastAsset.localIdentifier;
        NSLog(@"Local Identifier: %@", localIdentifier);
        self.m_iosUtils->mediaSaved(self.mediaFilename, QString::fromNSString(localIdentifier));
    }
    else {
        NSLog(@"Could not retrieve local identifier because the asset is not in the library.");
    }
}

@end

IosUtils::IosUtils(QObject* parent) : PlatformUtils(parent)
{
    // This allows you to write to Qt's AppDataLocation by actually creating a directory there.
    auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir saveDir(appDataPath);
    if (!saveDir.exists())
    {
        bool ok = saveDir.mkpath(appDataPath);
        if (!ok) {
            qWarning() << "Couldn't create dir. " << appDataPath;
        }
    }
}

void IosUtils::init()
{
}

void IosUtils::share(const QUrl& url, const QString& title)
{
#pragma unused(title)

    NSMutableArray *sharingItems = [NSMutableArray new];

    if (url.isValid()) {
        [sharingItems addObject:url.toNSURL()];
    }
    // if (!title.isEmpty()) {
    //     [sharingItems addObject:title.toNSString()];
    // }

    // get the main window rootViewController
    UIViewController *qtUIViewController = [[UIApplication sharedApplication].keyWindow rootViewController];

    UIActivityViewController *activityController = [[UIActivityViewController alloc] initWithActivityItems:sharingItems applicationActivities:nil];
    if ( [activityController respondsToSelector:@selector(popoverPresentationController)] ) { // iOS8
        activityController.popoverPresentationController.sourceView = qtUIViewController.view;
    }
    [qtUIViewController presentViewController:activityController animated:YES completion:nil];
}

void IosUtils::sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId)
{
#pragma unused(title, mimeType)
    
    NSString* nsFilePath = filePath.toNSString();
    NSURL* nsFileUrl = [NSURL fileURLWithPath:nsFilePath];
    
    static DocViewController* docViewController = nil;
    if (docViewController != nil) {
        [docViewController removeFromParentViewController];
        [docViewController release];
    }
    
    UIDocumentInteractionController* documentInteractionController = nil;
    documentInteractionController =
    [UIDocumentInteractionController interactionControllerWithURL:nsFileUrl];
    
    UIViewController* qtUIViewController =
    [[[[UIApplication sharedApplication] windows] firstObject] rootViewController];
    if (qtUIViewController != nil) {
        docViewController = [[DocViewController alloc] init];
        
        docViewController.requestId = requestId;
        docViewController.m_iosUtils = this;
        
        [qtUIViewController addChildViewController:docViewController];
        documentInteractionController.delegate = docViewController;
        
        [documentInteractionController presentPreviewAnimated:YES];
    }
}

bool IosUtils::saveMedia(const QString& filePath, const QString& filename, const QString& mimeType, const QString& directoryName)
{
#pragma unused(mimeType, directoryName)
    NSString* nsFilePath = filePath.toNSString();
    UIImage* image = [[UIImage imageWithContentsOfFile:nsFilePath] retain];

    static DocViewController* docViewController = nil;
    if (docViewController != nil) {
        [docViewController removeFromParentViewController];
        [docViewController release];
    }

    UIViewController* qtUIViewController = [[[[UIApplication sharedApplication] windows] firstObject] rootViewController];
    if (qtUIViewController == nil) {
        return false;
    }

    docViewController = [[DocViewController alloc] init];
    [qtUIViewController addChildViewController:docViewController];

    docViewController.mediaFilename = filename;
    docViewController.m_iosUtils = this;

    [PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
        switch (status) {
        case PHAuthorizationStatusAuthorized:
            UIImageWriteToSavedPhotosAlbum(image, docViewController, @selector(image:didFinishSavingWithError:contextInfo:), nil);
            break;
        case PHAuthorizationStatusDenied:
        case PHAuthorizationStatusRestricted:
        case PHAuthorizationStatusNotDetermined:
            // Handle other cases.
            break;
        default:
            break;
        }
    }];

  return true;
}

bool IosUtils::deleteMedia(const QString& contentUri)
{
    NSString* localIdentifier = contentUri.toNSString();

    // Make sure to check authorization status and request access if necessary
    [PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
        if (status == PHAuthorizationStatusAuthorized) {
            // Fetch the asset with the given local identifier
            PHFetchResult<PHAsset *> *assets = [PHAsset fetchAssetsWithLocalIdentifiers:@[localIdentifier] options:nil];

            // Check if the asset is found
            if (assets.count > 0) {
                // Get the asset
                PHAsset *asset = assets.firstObject;

                // Perform changes on the library
                [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
                    [PHAssetChangeRequest deleteAssets:@[asset]];
                } completionHandler:^(BOOL success, NSError * _Nullable error) {
                    if (success) {
                        NSLog(@"Successfully deleted photo");
                    } else {
                        NSLog(@"Failed to delete photo: %@", error.localizedDescription);
                    }
                }];
            } else {
              NSLog(@"Photo with local identifier %@ not found", localIdentifier);
            }
        } else {
            NSLog(@"Not authorized to access photo library");
        }
    }];

    return true;
}

QString IosUtils::copyContent(const QString& contentUri, const QString& targetPath)
{
    // Attempt regular copy.
    {
        auto sourceFilePath = Utils::urlToLocalFile(contentUri);
        auto targetFilePath = targetPath + "/" + QFileInfo(contentUri).fileName();

        QFile::remove(targetFilePath);

        if (QFile::copy(sourceFilePath, targetFilePath))
        {
            QFile::setPermissions(targetFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther);
            return targetFilePath;
        }
    }

    // Attempt scoped storage copy.
    @autoreleasepool {
        NSURL *url = [NSURL URLWithString:contentUri.toNSString()];
        if (!url) {
            NSLog(@"Invalid URL");
            return "";
        }

        if (![url startAccessingSecurityScopedResource]) {
            NSLog(@"Failed to access security scoped resource");
            return "";
        }

        NSData *fileData = [NSData dataWithContentsOfURL:url];
        [url stopAccessingSecurityScopedResource];

        if (!fileData) {
            NSLog(@"Failed to read file data");
            return "";
        }

        auto targetFilePath = targetPath + "/" + QFileInfo(contentUri).fileName();
        QFile::remove(targetFilePath);
        
        if (fileData.length > 0) {
            NSData *destData = [NSData dataWithData:fileData];
            BOOL success = [destData writeToFile:targetFilePath.toNSString() atomically:YES];
            if (success) {
                QFile::setPermissions(targetFilePath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther);
                return targetFilePath;
            } else {
                NSLog(@"Failed to write file to sandbox");
                return "";
            }
        } else {
            NSLog(@"File was empty or unreadable");
            return "";
        }
    }
}

QString IosUtils::copyToDownload(const QString& filePath, const QString& folder)
{
    return QString();
}

bool IosUtils::hasContent(const QString& uri)
{
  return false;
}

void IosUtils::requestReview()
{
  // Not implemented on iOS.
}
