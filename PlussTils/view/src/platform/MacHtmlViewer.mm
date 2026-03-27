#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <WebKit/WebKit.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include "PlatformHtmlViewer.h"

@interface PlussTilsViewHtmlWindowController : NSObject <NSWindowDelegate>

- (instancetype)initWithTitle:(NSString*)title
                         html:(NSString*)html
                        width:(CGFloat)width
                       height:(CGFloat)height
                 snapshotName:(NSString*)snapshotName;
- (NSWindow*)window;
- (void)present;
- (void)savePNG:(id)sender;

@end

@implementation PlussTilsViewHtmlWindowController {
  NSWindow* window_;
  WKWebView* webView_;
  NSString* snapshotName_;
}

- (instancetype)initWithTitle:(NSString*)title
                         html:(NSString*)html
                        width:(CGFloat)width
                       height:(CGFloat)height
                 snapshotName:(NSString*)snapshotName {
  self = [super init];
  if (!self) {
    return nil;
  }

  snapshotName_ = [snapshotName copy];

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
  [window_ setMinSize:NSMakeSize(640.0, 420.0)];

  NSView* contentView = [window_ contentView];

  NSView* toolbar = [[NSView alloc] initWithFrame:NSZeroRect];
  [toolbar setTranslatesAutoresizingMaskIntoConstraints:NO];
  [contentView addSubview:toolbar];

  NSButton* saveButton = [NSButton buttonWithTitle:@"Save PNG" target:self action:@selector(savePNG:)];
  [saveButton setBezelStyle:NSBezelStyleRounded];
  [saveButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [toolbar addSubview:saveButton];

  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  webView_ = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:configuration];
  [webView_ setTranslatesAutoresizingMaskIntoConstraints:NO];
  [webView_ loadHTMLString:html baseURL:nil];
  [contentView addSubview:webView_];

  [NSLayoutConstraint activateConstraints:@[
    [toolbar.topAnchor constraintEqualToAnchor:contentView.topAnchor],
    [toolbar.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor],
    [toolbar.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor],
    [toolbar.heightAnchor constraintEqualToConstant:46.0],

    [saveButton.trailingAnchor constraintEqualToAnchor:toolbar.trailingAnchor constant:-12.0],
    [saveButton.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

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
  // Hide immediately so close feels instant, then unwind modal loop.
  [window_ orderOut:nil];
  if ([NSApp modalWindow] == window_) {
    [NSApp abortModal];
  } else {
    [NSApp stopModal];
  }
}

- (void)savePNG:(id)sender {
  (void)sender;
  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setCanCreateDirectories:YES];
  [panel setNameFieldStringValue:(snapshotName_ == nil || [snapshotName_ length] == 0) ? @"view.png" : snapshotName_];
  if (@available(macOS 11.0, *)) {
    [panel setAllowedContentTypes:@[UTTypePNG]];
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

namespace pluss::view::detail {

bool mac_html_viewer_available() {
  return true;
}

bool windows_html_viewer_available() {
  return false;
}

bool show_windows_html_window(const std::string& title,
                              const std::string& html,
                              std::int64_t width,
                              std::int64_t height,
                              const std::string& snapshot_name) {
  (void)title;
  (void)html;
  (void)width;
  (void)height;
  (void)snapshot_name;
  return false;
}

bool show_macos_html_window(const std::string& title,
                            const std::string& html,
                            std::int64_t width,
                            std::int64_t height,
                            const std::string& snapshot_name) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSString* title_ns = [NSString stringWithUTF8String:title.c_str()];
    NSString* html_ns = [NSString stringWithUTF8String:html.c_str()];
    NSString* snapshot_ns = [NSString stringWithUTF8String:snapshot_name.c_str()];
    if (title_ns == nil || html_ns == nil || snapshot_ns == nil) {
      return false;
    }

    PlussTilsViewHtmlWindowController* controller =
        [[PlussTilsViewHtmlWindowController alloc] initWithTitle:title_ns
                                                        html:html_ns
                                                       width:static_cast<CGFloat>(std::max<std::int64_t>(640, width))
                                                      height:static_cast<CGFloat>(std::max<std::int64_t>(420, height))
                                                snapshotName:snapshot_ns];
    if (controller == nil) {
      return false;
    }

    [controller present];
    [NSApp runModalForWindow:[controller window]];
    return true;
  }
}

} // namespace pluss::view::detail
