#import <PhotosUI/PhotosUI.h>
#import <UIKit/UIKit.h>

#include "iosutil.h"
#include <QImage>
#include <functional>

// ─── PHPicker 代理 ────────────────────────────────────────────────────────────
@interface QtPhotoPickerDelegate : NSObject <PHPickerViewControllerDelegate>
@property (nonatomic, copy) void (^completion)(QImage);
@end

@implementation QtPhotoPickerDelegate

- (void)picker:(PHPickerViewController *)picker
    didFinishPicking:(NSArray<PHPickerResult *> *)results
{
    [picker dismissViewControllerAnimated:YES completion:nil];

    if (results.count == 0) {
        if (self.completion) self.completion(QImage());
        return;
    }

    PHPickerResult *result = results.firstObject;
    [result.itemProvider
        loadDataRepresentationForTypeIdentifier:@"public.image"
        completionHandler:^(NSData *data, NSError *error) {
            QImage img;
            if (data && !error) {
                QByteArray ba(static_cast<const char *>(data.bytes),
                              static_cast<int>(data.length));
                img.loadFromData(ba);
            }
            // 回调必须在主线程执行（Qt UI 操作线程安全要求）
            dispatch_async(dispatch_get_main_queue(), ^{
                if (self.completion) self.completion(img);
            });
        }];
}

@end

// 保持代理生命周期直到选择完成
static QtPhotoPickerDelegate *g_pickerDelegate = nil;

// ─── 公开 C++ 接口 ─────────────────────────────────────────────────────────────
void iosOpenPhotoLibrary(std::function<void(QImage)> callback)
{
    PHPickerConfiguration *config =
        [[PHPickerConfiguration alloc] initWithPhotoLibrary:PHPhotoLibrary.sharedPhotoLibrary];
    config.selectionLimit = 1;
    config.filter = [PHPickerFilter imagesFilter];

    PHPickerViewController *picker =
        [[PHPickerViewController alloc] initWithConfiguration:config];

    g_pickerDelegate = [[QtPhotoPickerDelegate alloc] init];
    g_pickerDelegate.completion = ^(QImage img) {
        callback(img);
        g_pickerDelegate = nil;
    };
    picker.delegate = g_pickerDelegate;

    UIViewController *rootVC =
        [UIApplication sharedApplication].connectedScenes
            .allObjects.firstObject;
    if ([rootVC isKindOfClass:[UIWindowScene class]]) {
        UIWindowScene *scene = (UIWindowScene *)rootVC;
        rootVC = scene.windows.firstObject.rootViewController;
    }
    // 找到最顶层的 VC
    while (rootVC.presentedViewController) {
        rootVC = rootVC.presentedViewController;
    }
    [rootVC presentViewController:picker animated:YES completion:nil];
}
