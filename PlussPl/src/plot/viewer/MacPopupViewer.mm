#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <WebKit/WebKit.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

#include "plot/viewer/PlatformPopupViewer.h"

@interface PlussPlPlotWindowController : NSObject <NSWindowDelegate>

- (instancetype)initWithTitle:(NSString*)title
                      svgPath:(NSString*)svgPath
                        width:(CGFloat)width
                       height:(CGFloat)height;
- (NSWindow*)window;
- (void)present;
- (void)saveSVG:(id)sender;
- (void)savePNG:(id)sender;

@end

@implementation PlussPlPlotWindowController {
  NSWindow* window_;
  WKWebView* webView_;
  NSString* svgPath_;
}

- (instancetype)initWithTitle:(NSString*)title
                      svgPath:(NSString*)svgPath
                        width:(CGFloat)width
                       height:(CGFloat)height {
  self = [super init];
  if (!self) {
    return nil;
  }

  svgPath_ = [svgPath copy];

  const NSRect frame = NSMakeRect(0.0, 0.0, width, height);
  const NSWindowStyleMask style =
      NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
  window_ = [[NSWindow alloc] initWithContentRect:frame
                                        styleMask:style
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
  [window_ setTitle:title];
  [window_ center];
  [window_ setDelegate:self];
  [window_ setReleasedWhenClosed:NO];
  [window_ setMinSize:NSMakeSize(700.0, 500.0)];

  NSView* contentView = [window_ contentView];

  NSView* toolbar = [[NSView alloc] initWithFrame:NSZeroRect];
  [toolbar setTranslatesAutoresizingMaskIntoConstraints:NO];
  [contentView addSubview:toolbar];

  NSButton* saveSvgButton = [NSButton buttonWithTitle:@"Save SVG" target:self action:@selector(saveSVG:)];
  [saveSvgButton setBezelStyle:NSBezelStyleRounded];
  [saveSvgButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [toolbar addSubview:saveSvgButton];

  NSButton* savePngButton = [NSButton buttonWithTitle:@"Save PNG" target:self action:@selector(savePNG:)];
  [savePngButton setBezelStyle:NSBezelStyleRounded];
  [savePngButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [toolbar addSubview:savePngButton];

  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  webView_ = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:configuration];
  [webView_ setTranslatesAutoresizingMaskIntoConstraints:NO];
  [contentView addSubview:webView_];

  NSURL* svgURL = [NSURL fileURLWithPath:svgPath_];
  if (svgURL != nil) {
    [webView_ loadFileURL:svgURL allowingReadAccessToURL:[svgURL URLByDeletingLastPathComponent]];
  }

  [NSLayoutConstraint activateConstraints:@[
    [toolbar.topAnchor constraintEqualToAnchor:contentView.topAnchor],
    [toolbar.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
    [toolbar.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
    [toolbar.heightAnchor constraintEqualToConstant:46.0],

    [savePngButton.trailingAnchor constraintEqualToAnchor:toolbar.trailingAnchor constant:-12.0],
    [savePngButton.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

    [saveSvgButton.trailingAnchor constraintEqualToAnchor:savePngButton.leadingAnchor constant:-8.0],
    [saveSvgButton.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

    [webView_.topAnchor constraintEqualToAnchor:toolbar.bottomAnchor],
    [webView_.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
    [webView_.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
    [webView_.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor],
  ]];

  return self;
}

- (NSWindow*)window {
  return window_;
}

- (void)present {
  [window_ makeKeyAndOrderFront:nil];
  [NSApp activateIgnoringOtherApps:YES];
}

- (void)windowWillClose:(NSNotification*)notification {
  (void)notification;
  [NSApp stopModal];
}

- (void)saveSVG:(id)sender {
  (void)sender;
  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setCanCreateDirectories:YES];
  [panel setNameFieldStringValue:@"plot.svg"];
  if (@available(macOS 11.0, *)) {
    [panel setAllowedContentTypes:@[UTTypeSVG]];
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [panel setAllowedFileTypes:@[@"svg"]];
#pragma clang diagnostic pop
  }

  if ([panel runModal] != NSModalResponseOK) {
    return;
  }

  NSURL* outputURL = [panel URL];
  if (outputURL == nil) {
    return;
  }

  NSURL* inputURL = [NSURL fileURLWithPath:svgPath_];
  if (inputURL == nil) {
    return;
  }
  [[NSFileManager defaultManager] copyItemAtURL:inputURL toURL:outputURL error:nil];
}

- (void)savePNG:(id)sender {
  (void)sender;
  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setCanCreateDirectories:YES];
  [panel setNameFieldStringValue:@"plot.png"];
  if (@available(macOS 11.0, *)) {
    [panel setAllowedContentTypes:@[UTTypePNG]];
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [panel setAllowedFileTypes:@[@"png"]];
#pragma clang diagnostic pop
  }

  if ([panel runModal] != NSModalResponseOK) {
    return;
  }

  NSURL* outputURL = [panel URL];
  if (outputURL == nil) {
    return;
  }

  if (@available(macOS 10.13, *)) {
    WKSnapshotConfiguration* config = [[WKSnapshotConfiguration alloc] init];
    [webView_ takeSnapshotWithConfiguration:config
                          completionHandler:^(NSImage* image, NSError* error) {
      if (error != nil || image == nil) {
        return;
      }
      CGImageRef cgImage = [image CGImageForProposedRect:nullptr context:nil hints:nil];
      if (cgImage == nullptr) {
        return;
      }
      NSBitmapImageRep* rep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
      NSData* pngData = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
      if (pngData != nil) {
        [pngData writeToURL:outputURL atomically:YES];
      }
    }];
  }
}

@end

namespace pluss::plotism::detail {

bool mac_plot_viewer_available() {
  return true;
}

bool show_macos_plot_window(const std::string& title,
                            const std::string& svg_path,
                            std::int64_t width,
                            std::int64_t height) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSString* title_ns = [NSString stringWithUTF8String:title.c_str()];
    NSString* path_ns = [NSString stringWithUTF8String:svg_path.c_str()];
    if (title_ns == nil || path_ns == nil) {
      return false;
    }

    PlussPlPlotWindowController* controller =
        [[PlussPlPlotWindowController alloc] initWithTitle:title_ns
                                                   svgPath:path_ns
                                                     width:static_cast<CGFloat>(std::max<std::int64_t>(700, width))
                                                    height:static_cast<CGFloat>(std::max<std::int64_t>(500, height))];
    if (controller == nil) {
      return false;
    }

    [controller present];
    [NSApp runModalForWindow:[controller window]];
    return true;
  }
}

} // namespace pluss::plotism::detail
