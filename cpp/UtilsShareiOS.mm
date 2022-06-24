#import "UtilsShare.h"

#import <UIKit/UIKit.h>
#import <QDesktopServices>
#import <QFileInfo>
#import <QGuiApplication>
#import <QUrl>
#import <QDir>
#import <QDebug>

#import <UIKit/UIDocumentInteractionController.h>

#import "UtilsShareiOS.h"

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
@end

IosShareUtils::IosShareUtils(QObject* parent) : PlatformShareUtils(parent)
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

void IosShareUtils::share(const QUrl& url, const QString& title)
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

void IosShareUtils::sendFile(const QString& filePath, const QString& title, const QString& mimeType, int requestId)
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
        docViewController.m_iosShareUtils = this;
        
        [qtUIViewController addChildViewController:docViewController];
        documentInteractionController.delegate = docViewController;
        
        [documentInteractionController presentPreviewAnimated:YES];
    }
}
