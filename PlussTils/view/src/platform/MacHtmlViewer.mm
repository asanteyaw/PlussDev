#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <WebKit/WebKit.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include "PlatformHtmlViewer.h"

@interface PlussTilsViewHtmlWindowController : NSObject <NSWindowDelegate, WKNavigationDelegate>

- (instancetype)initWithTitle:(NSString*)title
                         html:(NSString*)html
                        width:(CGFloat)width
                       height:(CGFloat)height
                 snapshotName:(NSString*)snapshotName;
- (NSWindow*)window;
- (void)present;
- (void)savePNG:(id)sender;
- (void)saveAs:(NSMenuItem*)sender;
- (void)selectView:(id)sender;

@end

@implementation PlussTilsViewHtmlWindowController {
  NSWindow* window_;
  WKWebView* webView_;
  NSString* snapshotName_;
  NSPopUpButton* viewButton_;
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

  // One "Save" pull-down (the chevron marks it as a menu) offering PNG, PDF and, for a plot, SVG.
  NSPopUpButton* saveButton = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:YES];
  [saveButton setBezelStyle:NSBezelStyleRounded];
  [saveButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [saveButton addItemWithTitle:@"Save"];
  for (NSArray<NSString*>* format in @[ @[ @"PNG", @"png" ], @[ @"PDF", @"pdf" ], @[ @"SVG", @"svg" ] ]) {
    [saveButton addItemWithTitle:format[0]];
    NSMenuItem* item = [[saveButton menu] itemAtIndex:[saveButton numberOfItems] - 1];
    [item setTarget:self];
    [item setAction:@selector(saveAs:)];
    [item setRepresentedObject:format[1]];
  }
  [toolbar addSubview:saveButton];

  // "View" pull-down beside Save, filled from the page's own view tabs once it has loaded
  // (and hidden when the page has none). It replaces the in-page picker, which sat above the table.
  viewButton_ = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:YES];
  [viewButton_ setBezelStyle:NSBezelStyleRounded];
  [viewButton_ setTranslatesAutoresizingMaskIntoConstraints:NO];
  [viewButton_ addItemWithTitle:@"View"];
  [viewButton_ setHidden:YES];
  [toolbar addSubview:viewButton_];

  WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
  webView_ = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:configuration];
  [webView_ setNavigationDelegate:self];
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

    [viewButton_.trailingAnchor constraintEqualToAnchor:saveButton.leadingAnchor constant:-8.0],
    [viewButton_.centerYAnchor constraintEqualToAnchor:toolbar.centerYAnchor],

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

// Reads the page's view tabs (button.tab: id, label, active) and builds the View menu from them.
- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation {
  (void)navigation;
  NSString* script =
      @"(function(){const v=window.plussViews;if(!v||v.length<2){return '[]';}"
      @"const a=document.querySelector('button.tab.active');const activeId=a?a.id:'';"
      @"const p=document.querySelector('.view-picker');if(p){p.remove();}"
      @"return JSON.stringify(v.map(x=>({id:x.id,label:x.label,active:x.id===activeId})));})()";
  [webView evaluateJavaScript:script completionHandler:^(id result, NSError* error) {
    if (error != nil || ![result isKindOfClass:[NSString class]]) {
      return;
    }
    NSData* data = [(NSString*)result dataUsingEncoding:NSUTF8StringEncoding];
    NSArray* tabs = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    if (![tabs isKindOfClass:[NSArray class]] || [tabs count] < 2) {
      return;
    }
    NSMenu* menu = [self->viewButton_ menu];
    while ([menu numberOfItems] > 1) {
      [menu removeItemAtIndex:1];
    }
    for (NSDictionary* tab in tabs) {
      NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:tab[@"label"] action:@selector(selectView:) keyEquivalent:@""];
      [item setTarget:self];
      [item setRepresentedObject:tab[@"id"]];
      [item setState:[tab[@"active"] boolValue] ? NSControlStateValueOn : NSControlStateValueOff];
      [menu addItem:item];
    }
    [self->viewButton_ setHidden:NO];
  }];
}

// Picks a view through the page's own plussSelectView, so the page's script does the switching.
- (void)selectView:(id)sender {
  NSMenuItem* item = [sender isKindOfClass:[NSMenuItem class]] ? (NSMenuItem*)sender : nil;
  NSString* tab_id = [item representedObject];
  if (tab_id == nil) {
    return;
  }
  for (NSMenuItem* other in [[viewButton_ menu] itemArray]) {
    [other setState:other == item ? NSControlStateValueOn : NSControlStateValueOff];
  }
  NSString* script = [NSString stringWithFormat:@"window.plussSelectView('%@');", tab_id];
  [webView_ evaluateJavaScript:script completionHandler:nil];
}

- (void)present {
  [window_ makeKeyAndOrderFront:nil];
  [window_ displayIfNeeded];
  [NSApp activateIgnoringOtherApps:YES];
  [window_ makeKeyWindow];
  [NSApp updateWindows];
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

- (void)saveAs:(NSMenuItem*)sender {
  NSString* extension = [sender representedObject];
  if ([extension isEqualToString:@"png"]) {
    [self savePNG:sender];
    return;
  }
  NSSavePanel* panel = [NSSavePanel savePanel];
  [panel setCanCreateDirectories:YES];
  NSString* base = (snapshotName_ == nil || [snapshotName_ length] == 0) ? @"view" : [snapshotName_ stringByDeletingPathExtension];
  [panel setNameFieldStringValue:[base stringByAppendingPathExtension:extension]];
  if (@available(macOS 11.0, *)) {
    [panel setAllowedContentTypes:@[[extension isEqualToString:@"pdf"] ? UTTypePDF : UTTypeSVG]];
  }
  if ([panel runModal] != NSModalResponseOK || [panel URL] == nil) {
    return;
  }
  NSURL* outputURL = [panel URL];
  if ([extension isEqualToString:@"pdf"]) {
    if (@available(macOS 11.0, *)) {
      [webView_ createPDFWithConfiguration:[[WKPDFConfiguration alloc] init]
                         completionHandler:^(NSData* data, NSError* error) {
        if (error == nil && data != nil) {
          [data writeToURL:outputURL atomically:YES];
        }
      }];
    }
    return;
  }
  [webView_ evaluateJavaScript:@"(document.querySelector('svg')||{}).outerHTML||''"
             completionHandler:^(id result, NSError* error) {
    if (error == nil && [result isKindOfClass:[NSString class]] && [(NSString*)result length] > 0) {
      [(NSString*)result writeToURL:outputURL atomically:YES encoding:NSUTF8StringEncoding error:nil];
    }
  }];
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
    if (![NSApp isRunning]) {
      [NSApp finishLaunching];
    }

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
    if ([controller window] == nil || ![[controller window] isVisible]) {
      return false;
    }
    [NSApp runModalForWindow:[controller window]];
    return true;
  }
}

} // namespace pluss::view::detail
