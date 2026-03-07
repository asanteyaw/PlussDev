#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "pluss/view/window/shell.h"

namespace {

void pump_app_events() {
  for (;;) {
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if (event == nil) {
      break;
    }
    [NSApp sendEvent:event];
  }
  [NSApp updateWindows];
}

NSImage* image_from_canvas(const pluss::view::window::PixelCanvas& canvas) {
  const auto size = canvas.size();
  const auto bytes = canvas.bytes();
  if (size.width <= 0 || size.height <= 0 || bytes.empty()) {
    return nil;
  }

  NSBitmapImageRep* rep = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:nullptr
                    pixelsWide:size.width
                    pixelsHigh:size.height
                 bitsPerSample:8
               samplesPerPixel:4
                      hasAlpha:YES
                      isPlanar:NO
                colorSpaceName:NSCalibratedRGBColorSpace
                   bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                    bytesPerRow:size.width * 4
                   bitsPerPixel:32];
  if (rep == nil || [rep bitmapData] == nullptr) {
    return nil;
  }

  auto* dst = [rep bitmapData];
  const std::size_t row_bytes = static_cast<std::size_t>(size.width) * 4U;
  for (int y = 0; y < size.height; ++y) {
    const std::size_t src_row = static_cast<std::size_t>(y);
    std::memcpy(dst + static_cast<std::size_t>(y) * row_bytes,
                bytes.data() + src_row * row_bytes,
                row_bytes);
  }

  NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(size.width, size.height)];
  if (image == nil) {
    return nil;
  }
  [image addRepresentation:rep];
  return image;
}

NSString* shortcut_note_for_key_event(NSEvent* event) {
  if (event == nil) {
    return nil;
  }

  NSString* chars = [event charactersIgnoringModifiers];
  if (chars == nil || [chars length] == 0) {
    return nil;
  }
  const unichar key = [chars characterAtIndex:0];

  if (key == '+' || key == '=') return @"zoom_in";
  if (key == '-' || key == '_') return @"zoom_out";
  if (key == '0') return @"reset_view";
  if (key == 's' || key == 'S') return @"save_snapshot";
  if (key == 'j' || key == 'J' || key == NSPageDownFunctionKey) return @"next_page";
  if (key == 'k' || key == 'K' || key == NSPageUpFunctionKey) return @"previous_page";
  if (key == NSLeftArrowFunctionKey) return @"pan_left";
  if (key == NSRightArrowFunctionKey) return @"pan_right";
  if (key == NSUpArrowFunctionKey) return @"pan_up";
  if (key == NSDownArrowFunctionKey) return @"pan_down";
  return nil;
}

} // namespace

@interface PlussViewCocoaShellDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) BOOL closed;
@end

@implementation PlussViewCocoaShellDelegate
- (void)windowWillClose:(NSNotification*)notification {
  (void)notification;
  self.closed = YES;
}
@end

@interface PlussViewCocoaShellHost : NSObject
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) NSImageView* image_view;
@property(nonatomic, strong) PlussViewCocoaShellDelegate* delegate;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* input_events;
@property(nonatomic, strong) id key_monitor;
@property(nonatomic, strong) id scroll_monitor;
@property(nonatomic, strong) id drag_monitor;
@property(nonatomic, strong) id down_monitor;
@end

@implementation PlussViewCocoaShellHost
@end

namespace pluss::view::window {

class CocoaWindowShell final : public IWindowShell {
 public:
  bool open(const ShellConfig& config) override {
    @autoreleasepool {
      [NSApplication sharedApplication];
      [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

      if (host_ == nil) {
        host_ = [PlussViewCocoaShellHost new];
      }

      const CGFloat width = static_cast<CGFloat>(std::max(640, config.initial_size.width));
      const CGFloat height = static_cast<CGFloat>(std::max(420, config.initial_size.height));
      const NSRect frame = NSMakeRect(0.0, 0.0, width, height);
      const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                      NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

      host_.window = [[NSWindow alloc] initWithContentRect:frame
                                                 styleMask:style
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
      if (host_.window == nil) {
        events_.push_back(ShellEvent{ShellEventKind::Error, {}, "cocoa_window_create_failed"});
        return false;
      }

      const std::string title = config.title.empty() ? std::string("PlussView") : config.title;
      NSString* ns_title = [NSString stringWithUTF8String:title.c_str()];
      [host_.window setTitle:(ns_title != nil ? ns_title : @"PlussView")];
      [host_.window center];
      [host_.window setReleasedWhenClosed:NO];
      [host_.window setMinSize:NSMakeSize(640.0, 420.0)];
      [host_.window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

      host_.delegate = [PlussViewCocoaShellDelegate new];
      host_.delegate.closed = NO;
      [host_.window setDelegate:host_.delegate];
      host_.input_events = [NSMutableArray array];

      NSView* content = [host_.window contentView];
      host_.image_view = [[NSImageView alloc] initWithFrame:[content bounds]];
      [host_.image_view setImageScaling:NSImageScaleAxesIndependently];
      [host_.image_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
      [content addSubview:host_.image_view];

      __weak PlussViewCocoaShellHost* weak_host = host_;
      host_.key_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                                 handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                   PlussViewCocoaShellHost* strong_host = weak_host;
                                                                   if (strong_host == nil || e == nil || e.window != strong_host.window) {
                                                                     return e;
                                                                   }
                                                                   NSString* note = shortcut_note_for_key_event(e);
                                                                   if (note != nil) {
                                                                     [strong_host.input_events addObject:@{
                                                                       @"kind" : @"shortcut",
                                                                       @"note" : note
                                                                     }];
                                                                     return e;
                                                                   }

                                                                   NSString* chars = [e characters];
                                                                   if (chars == nil || [chars length] == 0) {
                                                                     return e;
                                                                   }

                                                                   const NSEventModifierFlags flags =
                                                                       [e modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
                                                                   const bool text_allowed =
                                                                       (flags & (NSEventModifierFlagCommand |
                                                                                 NSEventModifierFlagControl |
                                                                                 NSEventModifierFlagOption)) == 0;

                                                                   if (!text_allowed) {
                                                                     return e;
                                                                   }

                                                                   for (NSUInteger i = 0; i < [chars length]; ++i) {
                                                                     const unichar ch = [chars characterAtIndex:i];
                                                                     if (ch == NSDeleteCharacter || ch == NSBackspaceCharacter ||
                                                                         ch == 0x7F) {
                                                                       [strong_host.input_events addObject:@{
                                                                         @"kind" : @"text_input",
                                                                         @"note" : @"__backspace__"
                                                                       }];
                                                                       continue;
                                                                     }
                                                                     if (ch < 0x20 || ch == NSTabCharacter ||
                                                                         ch == NSCarriageReturnCharacter || ch == NSEnterCharacter) {
                                                                       continue;
                                                                     }
                                                                     NSString* scalar = [NSString stringWithCharacters:&ch length:1];
                                                                     if (scalar == nil || [scalar length] == 0) {
                                                                       continue;
                                                                     }
                                                                     [strong_host.input_events addObject:@{
                                                                       @"kind" : @"text_input",
                                                                       @"note" : scalar
                                                                     }];
                                                                   }
                                                                   return e;
                                                                 }];

      host_.scroll_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                                                    handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                      PlussViewCocoaShellHost* strong_host = weak_host;
                                                                      if (strong_host == nil || e == nil || e.window != strong_host.window) {
                                                                        return e;
                                                                      }
                                                                      [strong_host.input_events addObject:@{
                                                                        @"kind" : @"scroll",
                                                                        @"dx" : @(e.scrollingDeltaX),
                                                                        @"dy" : @(e.scrollingDeltaY)
                                                                      }];
                                                                      return e;
                                                                    }];

      host_.drag_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged
                                                                  handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                    PlussViewCocoaShellHost* strong_host = weak_host;
                                                                    if (strong_host == nil || e == nil || e.window != strong_host.window) {
                                                                      return e;
                                                                    }
                                                                    [strong_host.input_events addObject:@{
                                                                      @"kind" : @"drag",
                                                                      @"dx" : @(e.deltaX),
                                                                      @"dy" : @(e.deltaY)
                                                                    }];
                                                                    return e;
                                                                  }];

      host_.down_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                                                  handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                    PlussViewCocoaShellHost* strong_host = weak_host;
                                                                    if (strong_host == nil || e == nil || e.window != strong_host.window ||
                                                                        strong_host.window == nil ||
                                                                        strong_host.image_view == nil) {
                                                                      return e;
                                                                    }
                                                                    NSPoint in_window = [e locationInWindow];
                                                                    NSPoint in_view = [strong_host.image_view convertPoint:in_window fromView:nil];
                                                                    const NSRect bounds = [strong_host.image_view bounds];
                                                                    const double x = std::clamp(static_cast<double>(in_view.x), 0.0, static_cast<double>(bounds.size.width));
                                                                    const double y = std::clamp(static_cast<double>(bounds.size.height - in_view.y),
                                                                                                0.0,
                                                                                                static_cast<double>(bounds.size.height));
                                                                    [strong_host.input_events addObject:@{
                                                                      @"kind" : @"pointer_down",
                                                                      @"x" : @(x),
                                                                      @"y" : @(y)
                                                                    }];
                                                                    return e;
                                                                  }];

      [host_.window makeKeyAndOrderFront:nil];
      [NSApp activateIgnoringOtherApps:YES];

      open_ = true;
      last_size_ = SizeI{static_cast<int>(width), static_cast<int>(height)};
      events_.push_back(ShellEvent{ShellEventKind::Opened, last_size_, "cocoa_shell_opened"});
      pump_app_events();
      return true;
    }
  }

  bool present(const PixelCanvas& canvas) override {
    @autoreleasepool {
      if (!open_ || host_ == nil || host_.window == nil || host_.image_view == nil || canvas.empty()) {
        events_.push_back(ShellEvent{ShellEventKind::Error, last_size_, "cocoa_shell_present_failed"});
        return false;
      }

      NSImage* image = image_from_canvas(canvas);
      if (image == nil) {
        events_.push_back(ShellEvent{ShellEventKind::Error, last_size_, "cocoa_image_convert_failed"});
        return false;
      }

      [host_.image_view setImage:image];
      [host_.window makeKeyAndOrderFront:nil];
      pump_app_events();

      const NSRect bounds = [[host_.window contentView] bounds];
      last_size_ = SizeI{static_cast<int>(std::round(bounds.size.width)),
                         static_cast<int>(std::round(bounds.size.height))};
      events_.push_back(ShellEvent{ShellEventKind::Presented, last_size_, "cocoa_shell_presented"});
      return true;
    }
  }

  void close() noexcept override {
    @autoreleasepool {
      if (!open_) {
        return;
      }

      if (host_ != nil && host_.window != nil) {
        if (host_.key_monitor != nil) {
          [NSEvent removeMonitor:host_.key_monitor];
          host_.key_monitor = nil;
        }
        if (host_.scroll_monitor != nil) {
          [NSEvent removeMonitor:host_.scroll_monitor];
          host_.scroll_monitor = nil;
        }
        if (host_.drag_monitor != nil) {
          [NSEvent removeMonitor:host_.drag_monitor];
          host_.drag_monitor = nil;
        }
        if (host_.down_monitor != nil) {
          [NSEvent removeMonitor:host_.down_monitor];
          host_.down_monitor = nil;
        }
        [host_.window close];
      }
      open_ = false;
      events_.push_back(ShellEvent{ShellEventKind::Closed, last_size_, "cocoa_shell_closed"});
      pump_app_events();
    }
  }

  std::vector<ShellEvent> poll_events() override {
    @autoreleasepool {
      pump_app_events();

      if (open_ && host_ != nil && host_.delegate != nil && host_.delegate.closed) {
        open_ = false;
        events_.push_back(ShellEvent{ShellEventKind::Closed, last_size_, "cocoa_shell_closed_by_user"});
      }

      if (open_ && host_ != nil && host_.window != nil) {
        const NSRect bounds = [[host_.window contentView] bounds];
        const SizeI current{static_cast<int>(std::round(bounds.size.width)),
                            static_cast<int>(std::round(bounds.size.height))};
        if (current.width != last_size_.width || current.height != last_size_.height) {
          last_size_ = current;
          events_.push_back(ShellEvent{ShellEventKind::ResizeRequested, current, "cocoa_shell_resized"});
        }
      }

      if (host_ != nil && host_.input_events != nil) {
        for (NSDictionary* packet in host_.input_events) {
          NSString* kind = packet[@"kind"];
          if ([kind isEqualToString:@"shortcut"]) {
            NSString* note = packet[@"note"];
            const char* cnote = note != nil ? [note UTF8String] : "";
            events_.push_back(ShellEvent{
                ShellEventKind::ShortcutCommand,
                last_size_,
                cnote != nullptr ? std::string(cnote) : std::string{},
            });
          } else if ([kind isEqualToString:@"scroll"]) {
            const double dx = [packet[@"dx"] doubleValue];
            const double dy = [packet[@"dy"] doubleValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerScroll,
                last_size_,
                "cocoa_scroll",
                dx,
                dy,
            });
          } else if ([kind isEqualToString:@"drag"]) {
            const double dx = [packet[@"dx"] doubleValue];
            const double dy = [packet[@"dy"] doubleValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerDrag,
                last_size_,
                "cocoa_drag",
                dx,
                dy,
            });
          } else if ([kind isEqualToString:@"pointer_down"]) {
            const double x = [packet[@"x"] doubleValue];
            const double y = [packet[@"y"] doubleValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerDown,
                last_size_,
                "cocoa_pointer_down",
                0.0,
                0.0,
                x,
                y,
            });
          } else if ([kind isEqualToString:@"text_input"]) {
            NSString* note = packet[@"note"];
            const char* cnote = note != nil ? [note UTF8String] : "";
            events_.push_back(ShellEvent{
                ShellEventKind::TextInput,
                last_size_,
                cnote != nullptr ? std::string(cnote) : std::string{},
            });
          }
        }
        [host_.input_events removeAllObjects];
      }

      auto buffered = std::move(events_);
      events_.clear();
      return buffered;
    }
  }

  ShellBackend backend() const noexcept override {
    return ShellBackend::Cocoa;
  }

  ShellCapabilities capabilities() const noexcept override {
    return ShellCapabilities{
        .supports_live_present = true,
        .supports_resize_events = true,
        .supports_shortcut_events = true,
        .supports_pointer_scroll = true,
        .supports_pointer_drag = true,
        .supports_pointer_click = true,
        .supports_text_input = true,
    };
  }

 private:
  bool open_{false};
  SizeI last_size_{};
  std::vector<ShellEvent> events_{};
  __strong PlussViewCocoaShellHost* host_{nil};
};

std::unique_ptr<IWindowShell> make_cocoa_window_shell() {
  return std::make_unique<CocoaWindowShell>();
}

} // namespace pluss::view::window

#endif
