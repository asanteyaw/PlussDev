#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "pluss/tils/view/window/shell.h"

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

NSBitmapImageRep* bitmap_rep_for_size(const pluss::view::window::SizeI size) {
  if (size.width <= 0 || size.height <= 0) {
    return nil;
  }
  return [[NSBitmapImageRep alloc]
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
  const NSEventModifierFlags flags =
      [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
  const bool command = (flags & NSEventModifierFlagCommand) != 0;

  if (command && (key == '+' || key == '=')) return @"zoom_in";
  if (command && (key == '-' || key == '_')) return @"zoom_out";
  if (command && key == '0') return @"reset_view";
  if (command && (key == 's' || key == 'S')) return @"save_snapshot";
  if (command && (key == 'j' || key == 'J')) return @"next_page";
  if (command && (key == 'k' || key == 'K')) return @"previous_page";
  if (key == NSPageDownFunctionKey) return @"page_down";
  if (key == NSPageUpFunctionKey) return @"page_up";
  if (key == NSHomeFunctionKey) return @"home";
  if (key == NSEndFunctionKey) return @"end";
  if (key == NSLeftArrowFunctionKey) return @"pan_left";
  if (key == NSRightArrowFunctionKey) return @"pan_right";
  if (key == NSUpArrowFunctionKey) return @"pan_up";
  if (key == NSDownArrowFunctionKey) return @"pan_down";
  return nil;
}

bool event_targets_window(NSEvent* event, NSWindow* window) {
  return event != nil && window != nil &&
         (event.window == window || (event.window == nil && window.isKeyWindow));
}

NSPoint event_location_in_window(NSEvent* event, NSWindow* window) {
  if (event != nil && event.window != nil) {
    return [event locationInWindow];
  }
  return [window convertPointFromScreen:[NSEvent mouseLocation]];
}

} // namespace

@interface PlussTilsViewCocoaShellDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) BOOL closed;
@end

@implementation PlussTilsViewCocoaShellDelegate
- (void)windowWillClose:(NSNotification*)notification {
  (void)notification;
  self.closed = YES;
}
@end

@interface PlussTilsViewCocoaShellHost : NSObject
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) NSView* toolbar;
@property(nonatomic, strong) NSPopUpButton* save_button;
@property(nonatomic, strong) NSImageView* image_view;
@property(nonatomic, strong) PlussTilsViewCocoaShellDelegate* delegate;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* input_events;
@property(nonatomic, strong) id key_monitor;
@property(nonatomic, strong) id scroll_monitor;
@property(nonatomic, strong) id drag_monitor;
@property(nonatomic, strong) id down_monitor;
@property(nonatomic, strong) id up_monitor;
- (void)saveAs:(id)sender;
@end

@implementation PlussTilsViewCocoaShellHost
- (void)saveAs:(id)sender {
  NSMenuItem* item = [sender isKindOfClass:[NSMenuItem class]] ? (NSMenuItem*)sender : nil;
  NSString* extension = [item representedObject];
  if (self.input_events != nil && extension != nil) {
    [self.input_events addObject:@{
      @"kind" : @"shortcut",
      @"note" : [@"save_as:" stringByAppendingString:extension]
    }];
  }
}
@end

namespace pluss::view::window {

class CocoaWindowShell final : public IWindowShell {
 public:
  ~CocoaWindowShell() override { close(); }

  bool open(const ShellConfig& config) override {
    @autoreleasepool {
      [NSApplication sharedApplication];
      [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
      if (![NSApp isRunning]) {
        [NSApp finishLaunching];
      }

      if (host_ == nil) {
        host_ = [PlussTilsViewCocoaShellHost new];
      }

      const CGFloat width = static_cast<CGFloat>(std::max(640, config.initial_size.width));
      const CGFloat height = static_cast<CGFloat>(std::max(420, config.initial_size.height));
      const bool show_save_button = !config.save_formats.empty();
      const CGFloat toolbar_height = show_save_button ? 46.0 : 0.0;
      const NSRect frame = NSMakeRect(0.0, 0.0, width, height + toolbar_height);
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

      const std::string title = config.title.empty() ? std::string("PlussTilsView") : config.title;
      NSString* ns_title = [NSString stringWithUTF8String:title.c_str()];
      [host_.window setTitle:(ns_title != nil ? ns_title : @"PlussTilsView")];
      [host_.window center];
      [host_.window setReleasedWhenClosed:NO];
      [host_.window setMinSize:NSMakeSize(640.0, 420.0)];
      [host_.window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];

      host_.delegate = [PlussTilsViewCocoaShellDelegate new];
      host_.delegate.closed = NO;
      [host_.window setDelegate:host_.delegate];
      host_.input_events = [NSMutableArray array];

      NSView* content = [host_.window contentView];
      const NSRect content_bounds = [content bounds];
      if (show_save_button) {
        host_.toolbar = [[NSView alloc]
            initWithFrame:NSMakeRect(0.0,
                                     content_bounds.size.height - toolbar_height,
                                     content_bounds.size.width,
                                     toolbar_height)];
        [host_.toolbar setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
        [host_.toolbar setWantsLayer:YES];
        host_.toolbar.layer.backgroundColor = [[NSColor windowBackgroundColor] CGColor];
        [content addSubview:host_.toolbar];

        host_.save_button = [[NSPopUpButton alloc]
            initWithFrame:NSMakeRect(
                std::max<CGFloat>(12.0, content_bounds.size.width - 104.0), 8.0, 92.0, 30.0)
                pullsDown:YES];
        [host_.save_button setFrame:NSMakeRect(
            std::max<CGFloat>(12.0, content_bounds.size.width - 104.0), 8.0, 92.0, 30.0)];
        [host_.save_button setAutoresizingMask:NSViewMinXMargin];
        [host_.save_button setBezelStyle:NSBezelStyleRounded];
        [host_.save_button setFont:[NSFont systemFontOfSize:13.0 weight:NSFontWeightSemibold]];
        [host_.save_button addItemWithTitle:@"Save As"];
        [host_.toolbar addSubview:host_.save_button];
        for (const auto& format : config.save_formats) {
          NSString* label = [[NSString alloc] initWithBytes:format.label.data()
                                                    length:format.label.size()
                                                  encoding:NSUTF8StringEncoding];
          NSString* extension = [[NSString alloc] initWithBytes:format.extension.data()
                                                        length:format.extension.size()
                                                      encoding:NSUTF8StringEncoding];
          if (label == nil || extension == nil) {
            continue;
          }
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                       action:@selector(saveAs:)
                                                keyEquivalent:@""];
          [item setTarget:host_];
          [item setRepresentedObject:extension];
          [[host_.save_button menu] addItem:item];
        }
      } else {
        host_.toolbar = nil;
        host_.save_button = nil;
      }

      host_.image_view = [[NSImageView alloc]
          initWithFrame:NSMakeRect(0.0, 0.0, content_bounds.size.width, height)];
      [host_.image_view setImageScaling:NSImageScaleAxesIndependently];
      [host_.image_view setImageAlignment:NSImageAlignCenter];
      [host_.image_view setEditable:NO];
      [host_.image_view setAnimates:NO];
      [host_.image_view setWantsLayer:YES];
      // The bitmap storage is reused by present(). Keep the NSImageView draw
      // synchronous with the AppKit thread so Core Animation never uploads a
      // buffer while the next frame is being copied into it.
      host_.image_view.layer.drawsAsynchronously = NO;
      host_.image_view.layer.minificationFilter = kCAFilterLinear;
      host_.image_view.layer.magnificationFilter = kCAFilterLinear;
      [host_.image_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
      [content addSubview:host_.image_view];
      const char* disable_metal =
          std::getenv("PLUSSTILS_VIEW_DISABLE_METAL");
      if (disable_metal == nullptr ||
          std::string_view(disable_metal) != "1") {
        (void)setup_metal_();
      }

      __weak PlussTilsViewCocoaShellHost* weak_host = host_;
      host_.key_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                                                 handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                   PlussTilsViewCocoaShellHost* strong_host = weak_host;
                                                                   if (strong_host == nil ||
                                                                       !event_targets_window(e, strong_host.window)) {
                                                                     return e;
                                                                   }
                                                                   NSString* note = shortcut_note_for_key_event(e);
                                                                   if (note != nil) {
                                                                     [strong_host.input_events addObject:@{
                                                                       @"kind" : @"shortcut",
                                                                       @"note" : note
                                                                     }];
                                                                     return nil;
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

                                                                   bool consumed = false;
                                                                   for (NSUInteger i = 0; i < [chars length]; ++i) {
                                                                     const unichar ch = [chars characterAtIndex:i];
                                                                     if (ch == NSDeleteCharacter || ch == NSBackspaceCharacter ||
                                                                         ch == 0x7F) {
                                                                       [strong_host.input_events addObject:@{
                                                                         @"kind" : @"text_input",
                                                                         @"note" : @"__backspace__"
                                                                       }];
                                                                       consumed = true;
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
                                                                     consumed = true;
                                                                   }
                                                                   // The software input owns accepted text. Passing the same event
                                                                   // to NSImageView makes AppKit reject it with NSBeep; users with
                                                                   // visual alerts enabled see that rejection as a screen flash.
                                                                   return consumed ? nil : e;
                                                                 }];

      host_.scroll_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                                                    handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                      PlussTilsViewCocoaShellHost* strong_host = weak_host;
                                                                      if (strong_host == nil ||
                                                                          !event_targets_window(e, strong_host.window)) {
                                                                        return e;
                                                                      }
                                                                      NSPoint in_window =
                                                                          event_location_in_window(e, strong_host.window);
                                                                      NSPoint in_view = [strong_host.image_view convertPoint:in_window fromView:nil];
                                                                      const NSRect bounds = [strong_host.image_view bounds];
                                                                      const double x = std::clamp(static_cast<double>(in_view.x),
                                                                                                  0.0,
                                                                                                  static_cast<double>(bounds.size.width));
                                                                      const double y = std::clamp(
                                                                          static_cast<double>(bounds.size.height - in_view.y),
                                                                          0.0,
                                                                          static_cast<double>(bounds.size.height));
                                                                      const NSEventModifierFlags flags =
                                                                          [e modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
                                                                      [strong_host.input_events addObject:@{
                                                                        @"kind" : @"scroll",
                                                                        @"dx" : @(e.scrollingDeltaX),
                                                                        @"dy" : @(e.scrollingDeltaY),
                                                                        @"x" : @(x),
                                                                        @"y" : @(y),
                                                                        @"precise" : @(e.hasPreciseScrollingDeltas),
                                                                        @"shift" : @((flags & NSEventModifierFlagShift) != 0)
                                                                      }];
                                                                      return nil;
                                                                    }];

      host_.drag_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged
                                                                  handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                    PlussTilsViewCocoaShellHost* strong_host = weak_host;
                                                                    if (strong_host == nil ||
                                                                        !event_targets_window(e, strong_host.window)) {
                                                                      return e;
                                                                    }
                                                                    NSPoint in_window =
                                                                        event_location_in_window(e, strong_host.window);
                                                                    NSPoint in_view = [strong_host.image_view convertPoint:in_window fromView:nil];
                                                                    const NSRect bounds = [strong_host.image_view bounds];
                                                                    const double x = std::clamp(static_cast<double>(in_view.x),
                                                                                                0.0,
                                                                                                static_cast<double>(bounds.size.width));
                                                                    const double y = std::clamp(
                                                                        static_cast<double>(bounds.size.height - in_view.y),
                                                                        0.0,
                                                                        static_cast<double>(bounds.size.height));
                                                                    [strong_host.input_events addObject:@{
                                                                      @"kind" : @"drag",
                                                                      @"dx" : @(e.deltaX),
                                                                      @"dy" : @(e.deltaY),
                                                                      @"x" : @(x),
                                                                      @"y" : @(y)
                                                                    }];
                                                                    return nil;
                                                                  }];

      host_.down_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                                                  handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                    PlussTilsViewCocoaShellHost* strong_host = weak_host;
                                                                    if (strong_host == nil ||
                                                                        !event_targets_window(e, strong_host.window) ||
                                                                        strong_host.window == nil ||
                                                                        strong_host.image_view == nil) {
                                                                      return e;
                                                                    }
                                                                    NSPoint in_window =
                                                                        event_location_in_window(e, strong_host.window);
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

      host_.up_monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp
                                                               handler:^NSEvent* _Nullable(NSEvent* e) {
                                                                 PlussTilsViewCocoaShellHost* strong_host = weak_host;
                                                                 if (strong_host == nil ||
                                                                     !event_targets_window(e, strong_host.window) ||
                                                                     strong_host.window == nil ||
                                                                     strong_host.image_view == nil) {
                                                                   return e;
                                                                 }
                                                                 NSPoint in_window =
                                                                     event_location_in_window(e, strong_host.window);
                                                                 NSPoint in_view = [strong_host.image_view convertPoint:in_window fromView:nil];
                                                                 const NSRect bounds = [strong_host.image_view bounds];
                                                                 const double x = std::clamp(static_cast<double>(in_view.x),
                                                                                             0.0,
                                                                                             static_cast<double>(bounds.size.width));
                                                                 const double y = std::clamp(
                                                                     static_cast<double>(bounds.size.height - in_view.y),
                                                                     0.0,
                                                                     static_cast<double>(bounds.size.height));
                                                                 [strong_host.input_events addObject:@{
                                                                   @"kind" : @"pointer_up",
                                                                   @"x" : @(x),
                                                                   @"y" : @(y)
                                                                 }];
                                                                 return e;
                                                               }];

      [host_.window makeKeyAndOrderFront:nil];
      if (@available(macOS 14.0, *)) {
        [NSApp activate];
      } else {
        [NSApp activateIgnoringOtherApps:YES];
      }
      [host_.window makeKeyWindow];

      open_ = true;
      start_display_link_();
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

      if (metal_ready_ && present_metal_(canvas)) {
        const NSRect bounds = [host_.image_view bounds];
        last_size_ = SizeI{static_cast<int>(std::round(bounds.size.width)),
                           static_cast<int>(std::round(bounds.size.height))};
        events_.push_back(ShellEvent{
            ShellEventKind::Presented, last_size_, "cocoa_metal_presented"});
        return true;
      }

      const auto canvas_size = canvas.size();
      if (bitmap_size_.width != canvas_size.width ||
          bitmap_size_.height != canvas_size.height) {
        for (std::size_t index = 0; index < kFrameBufferCount; ++index) {
          bitmap_reps_[index] = bitmap_rep_for_size(canvas_size);
          canvas_images_[index] = [[NSImage alloc]
              initWithSize:NSMakeSize(canvas_size.width, canvas_size.height)];
          if (bitmap_reps_[index] != nil && canvas_images_[index] != nil) {
            [canvas_images_[index] addRepresentation:bitmap_reps_[index]];
          }
        }
        bitmap_size_ = canvas_size;
        frame_slot_ = 0;
      }
      frame_slot_ = (frame_slot_ + 1U) % kFrameBufferCount;
      NSBitmapImageRep* bitmap_rep = bitmap_reps_[frame_slot_];
      NSImage* canvas_image = canvas_images_[frame_slot_];
      if (bitmap_rep == nil || canvas_image == nil ||
          [bitmap_rep bitmapData] == nullptr) {
        events_.push_back(ShellEvent{ShellEventKind::Error, last_size_, "cocoa_image_convert_failed"});
        return false;
      }

      const auto bytes = canvas.bytes();
      std::memcpy([bitmap_rep bitmapData], bytes.data(), bytes.size());
      [host_.image_view setImage:canvas_image];
      [host_.image_view setNeedsDisplay:YES];
      [host_.image_view displayIfNeeded];

      const NSRect bounds = [host_.image_view bounds];
      last_size_ = SizeI{static_cast<int>(std::round(bounds.size.width)),
                         static_cast<int>(std::round(bounds.size.height))};
      events_.push_back(ShellEvent{ShellEventKind::Presented, last_size_, "cocoa_shell_presented"});
      return true;
    }
  }

  void close() noexcept override {
    @autoreleasepool {
      const bool was_open = open_;
      stop_display_link_();
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
        if (host_.up_monitor != nil) {
          [NSEvent removeMonitor:host_.up_monitor];
          host_.up_monitor = nil;
        }
        if (open_) {
          [host_.window close];
        }
      }
      open_ = false;
      if (metal_layer_ != nil) {
        [metal_layer_ removeFromSuperlayer];
      }
      metal_layer_ = nil;
      metal_pipeline_ = nil;
      metal_queue_ = nil;
      metal_texture_ = nil;
      metal_device_ = nil;
      metal_ready_ = false;
      for (std::size_t index = 0; index < kFrameBufferCount; ++index) {
        bitmap_reps_[index] = nil;
        canvas_images_[index] = nil;
      }
      bitmap_size_ = {};
      frame_slot_ = 0;
      if (was_open) {
        events_.push_back(ShellEvent{ShellEventKind::Closed, last_size_, "cocoa_shell_closed"});
      }
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
        const NSRect bounds = [host_.image_view bounds];
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
            const double x = [packet[@"x"] doubleValue];
            const double y = [packet[@"y"] doubleValue];
            const bool precise = [packet[@"precise"] boolValue];
            const bool shift = [packet[@"shift"] boolValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerScroll,
                last_size_,
                "cocoa_scroll",
                dx,
                dy,
                x,
                y,
                precise,
                shift,
            });
          } else if ([kind isEqualToString:@"drag"]) {
            const double dx = [packet[@"dx"] doubleValue];
            const double dy = [packet[@"dy"] doubleValue];
            const double x = [packet[@"x"] doubleValue];
            const double y = [packet[@"y"] doubleValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerDrag,
                last_size_,
                "cocoa_drag",
                dx,
                dy,
                x,
                y,
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
          } else if ([kind isEqualToString:@"pointer_up"]) {
            const double x = [packet[@"x"] doubleValue];
            const double y = [packet[@"y"] doubleValue];
            events_.push_back(ShellEvent{
                ShellEventKind::PointerUp,
                last_size_,
                "cocoa_pointer_up",
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

  double backing_scale_factor() const noexcept override {
    @autoreleasepool {
      if (host_ == nil || host_.window == nil) {
        return 1.0;
      }
      return std::clamp(static_cast<double>([host_.window backingScaleFactor]), 1.0, 2.0);
    }
  }

  bool presentation_frame_due() noexcept override {
    if (display_link_ == nullptr) {
      return true;
    }
    return frame_due_.exchange(false, std::memory_order_acq_rel);
  }

  bool set_clipboard_text(const std::string_view text) override {
    @autoreleasepool {
      NSString* value =
          [[NSString alloc] initWithBytes:text.data()
                                  length:text.size()
                                encoding:NSUTF8StringEncoding];
      if (value == nil) {
        return false;
      }
      NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
      [pasteboard clearContents];
      return [pasteboard setString:value forType:NSPasteboardTypeString] == YES;
    }
  }

  std::string choose_save_path(const std::string_view suggested_name,
                               const std::string_view extension) override {
    @autoreleasepool {
      NSSavePanel* panel = [NSSavePanel savePanel];
      [panel setCanCreateDirectories:YES];

      NSString* name =
          [[NSString alloc] initWithBytes:suggested_name.data()
                                  length:suggested_name.size()
                                encoding:NSUTF8StringEncoding];
      if (name != nil && [name length] > 0) {
        [panel setNameFieldStringValue:name];
      }

      NSString* ext =
          [[NSString alloc] initWithBytes:extension.data()
                                  length:extension.size()
                                encoding:NSUTF8StringEncoding];
      if (@available(macOS 11.0, *)) {
        if ([ext caseInsensitiveCompare:@"png"] == NSOrderedSame) {
          [panel setAllowedContentTypes:@[UTTypePNG]];
        } else if ([ext caseInsensitiveCompare:@"svg"] == NSOrderedSame) {
          UTType* svg_type = [UTType typeWithFilenameExtension:@"svg"];
          if (svg_type != nil) {
            [panel setAllowedContentTypes:@[svg_type]];
          }
        } else {
          UTType* type = [UTType typeWithFilenameExtension:ext];
          if (type != nil) {
            [panel setAllowedContentTypes:@[type]];
          }
        }
      }

      if ([panel runModal] != NSModalResponseOK || [panel URL] == nil) {
        return {};
      }
      const char* path = [[[panel URL] path] fileSystemRepresentation];
      return path != nullptr ? std::string(path) : std::string{};
    }
  }

  bool save_canvas_pdf(const PixelCanvas& canvas, const std::string_view path) override {
    @autoreleasepool {
      if (canvas.empty() || path.empty()) {
        return false;
      }
      NSBitmapImageRep* bitmap = bitmap_rep_for_size(canvas.size());
      if (bitmap == nil || [bitmap bitmapData] == nullptr) {
        return false;
      }
      const auto bytes = canvas.bytes();
      std::memcpy([bitmap bitmapData], bytes.data(), bytes.size());
      NSImage* image = [[NSImage alloc]
          initWithSize:NSMakeSize(canvas.size().width, canvas.size().height)];
      [image addRepresentation:bitmap];
      NSImageView* image_view = [[NSImageView alloc]
          initWithFrame:NSMakeRect(0.0, 0.0, canvas.size().width,
                                   canvas.size().height)];
      [image_view setImageScaling:NSImageScaleAxesIndependently];
      [image_view setImage:image];
      NSData* pdf =
          [image_view dataWithPDFInsideRect:[image_view bounds]];
      NSString* output = [[NSString alloc] initWithBytes:path.data()
                                                 length:path.size()
                                               encoding:NSUTF8StringEncoding];
      return pdf != nil && output != nil && [pdf writeToFile:output atomically:YES] == YES;
    }
  }

 private:
  static constexpr std::size_t kFrameBufferCount = 3;

  bool setup_metal_() {
    if (host_ == nil || host_.image_view == nil) {
      return false;
    }
    metal_device_ = MTLCreateSystemDefaultDevice();
    if (metal_device_ == nil) {
      return false;
    }
    metal_queue_ = [metal_device_ newCommandQueue];
    if (metal_queue_ == nil) {
      metal_device_ = nil;
      return false;
    }

    static constexpr char shader_source_utf8[] = R"METAL(
      #include <metal_stdlib>
      using namespace metal;

      struct RasterVertex {
        float4 position [[position]];
        float2 uv;
      };

      vertex RasterVertex pluss_vertex(uint vertex_id [[vertex_id]]) {
        const float2 positions[3] = {
          float2(-1.0, -1.0),
          float2( 3.0, -1.0),
          float2(-1.0,  3.0)
        };
        const float2 coordinates[3] = {
          float2(0.0, 1.0),
          float2(2.0, 1.0),
          float2(0.0, -1.0)
        };
        RasterVertex output;
        output.position = float4(positions[vertex_id], 0.0, 1.0);
        output.uv = coordinates[vertex_id];
        return output;
      }

      fragment float4 pluss_fragment(
          RasterVertex input [[stage_in]],
          texture2d<float> raster [[texture(0)]]) {
        constexpr sampler sample_filter(
            mag_filter::nearest, min_filter::nearest,
            address::clamp_to_edge);
        return raster.sample(sample_filter, input.uv);
      }
    )METAL";
    NSString* shader_source =
        [NSString stringWithUTF8String:shader_source_utf8];
    if (shader_source == nil) {
      metal_queue_ = nil;
      metal_device_ = nil;
      return false;
    }

    NSError* library_error = nil;
    id<MTLLibrary> library =
        [metal_device_ newLibraryWithSource:shader_source
                                   options:nil
                                     error:&library_error];
    if (library == nil) {
      metal_queue_ = nil;
      metal_device_ = nil;
      return false;
    }
    id<MTLFunction> vertex = [library newFunctionWithName:@"pluss_vertex"];
    id<MTLFunction> fragment =
        [library newFunctionWithName:@"pluss_fragment"];
    if (vertex == nil || fragment == nil) {
      metal_queue_ = nil;
      metal_device_ = nil;
      return false;
    }

    MTLRenderPipelineDescriptor* descriptor =
        [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.vertexFunction = vertex;
    descriptor.fragmentFunction = fragment;
    descriptor.colorAttachments[0].pixelFormat =
        MTLPixelFormatBGRA8Unorm_sRGB;
    NSError* pipeline_error = nil;
    metal_pipeline_ =
        [metal_device_ newRenderPipelineStateWithDescriptor:descriptor
                                                      error:&pipeline_error];
    if (metal_pipeline_ == nil) {
      metal_queue_ = nil;
      metal_device_ = nil;
      return false;
    }

    metal_layer_ = [CAMetalLayer layer];
    metal_layer_.device = metal_device_;
    metal_layer_.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    metal_layer_.framebufferOnly = YES;
    metal_layer_.opaque = YES;
    metal_layer_.contentsScale =
        host_.window != nil ? host_.window.backingScaleFactor : 1.0;
    metal_layer_.frame = host_.image_view.bounds;
    metal_layer_.autoresizingMask =
        kCALayerWidthSizable | kCALayerHeightSizable;
    [host_.image_view.layer addSublayer:metal_layer_];
    metal_ready_ = true;
    return true;
  }

  bool present_metal_(const PixelCanvas& canvas) {
    if (!metal_ready_ || metal_layer_ == nil || metal_device_ == nil ||
        metal_queue_ == nil || metal_pipeline_ == nil || canvas.empty()) {
      return false;
    }
    const SizeI canvas_size = canvas.size();
    metal_layer_.frame = host_.image_view.bounds;
    metal_layer_.drawableSize =
        CGSizeMake(static_cast<CGFloat>(canvas_size.width),
                   static_cast<CGFloat>(canvas_size.height));

    if (metal_texture_ == nil ||
        metal_texture_.width != static_cast<NSUInteger>(canvas_size.width) ||
        metal_texture_.height != static_cast<NSUInteger>(canvas_size.height)) {
      MTLTextureDescriptor* texture_descriptor =
          [MTLTextureDescriptor
              texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                           width:static_cast<NSUInteger>(
                                                     canvas_size.width)
                                          height:static_cast<NSUInteger>(
                                                     canvas_size.height)
                                       mipmapped:NO];
      texture_descriptor.usage = MTLTextureUsageShaderRead;
      texture_descriptor.storageMode = MTLStorageModeShared;
      metal_texture_ =
          [metal_device_ newTextureWithDescriptor:texture_descriptor];
      if (metal_texture_ == nil) {
        return false;
      }
    }
    const auto bytes = canvas.bytes();
    [metal_texture_
        replaceRegion:MTLRegionMake2D(
                          0, 0, static_cast<NSUInteger>(canvas_size.width),
                          static_cast<NSUInteger>(canvas_size.height))
           mipmapLevel:0
             withBytes:bytes.data()
           bytesPerRow:static_cast<NSUInteger>(canvas_size.width) * 4U];

    id<CAMetalDrawable> drawable = [metal_layer_ nextDrawable];
    id<MTLCommandBuffer> command_buffer = [metal_queue_ commandBuffer];
    if (drawable == nil || command_buffer == nil) {
      return true;
    }
    MTLRenderPassDescriptor* pass =
        [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(1.0, 1.0, 1.0, 1.0);
    id<MTLRenderCommandEncoder> encoder =
        [command_buffer renderCommandEncoderWithDescriptor:pass];
    if (encoder == nil) {
      return false;
    }
    [encoder setRenderPipelineState:metal_pipeline_];
    [encoder setFragmentTexture:metal_texture_ atIndex:0];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:3];
    [encoder endEncoding];
    [command_buffer presentDrawable:drawable];
    [command_buffer commit];
    // Keep at most one drawable transition in flight. The CPU raster work is
    // already isolated on its worker; waiting here prevents WindowServer
    // capture/composition from observing a drawable before its upload has
    // completed.
    [command_buffer waitUntilCompleted];
    return command_buffer.status == MTLCommandBufferStatusCompleted;
  }

  static CVReturn display_link_callback_(CVDisplayLinkRef,
                                         const CVTimeStamp*,
                                         const CVTimeStamp*,
                                         CVOptionFlags,
                                         CVOptionFlags*,
                                         void* context) {
    auto* shell = static_cast<CocoaWindowShell*>(context);
    if (shell != nullptr) {
      shell->frame_due_.store(true, std::memory_order_release);
    }
    return kCVReturnSuccess;
  }

  void start_display_link_() {
    stop_display_link_();
    frame_due_.store(true, std::memory_order_release);
    CVDisplayLinkRef link = nullptr;
    if (CVDisplayLinkCreateWithActiveCGDisplays(&link) != kCVReturnSuccess ||
        link == nullptr) {
      return;
    }
    if (CVDisplayLinkSetOutputCallback(
            link, &CocoaWindowShell::display_link_callback_, this) !=
            kCVReturnSuccess ||
        CVDisplayLinkStart(link) != kCVReturnSuccess) {
      CVDisplayLinkRelease(link);
      return;
    }
    display_link_ = link;
  }

  void stop_display_link_() noexcept {
    CVDisplayLinkRef link = display_link_;
    display_link_ = nullptr;
    if (link != nullptr) {
      if (CVDisplayLinkIsRunning(link)) {
        (void)CVDisplayLinkStop(link);
      }
      CVDisplayLinkRelease(link);
    }
    frame_due_.store(true, std::memory_order_release);
  }

  bool open_{false};
  SizeI last_size_{};
  SizeI bitmap_size_{};
  std::size_t frame_slot_{0};
  std::vector<ShellEvent> events_{};
  __strong PlussTilsViewCocoaShellHost* host_{nil};
  __strong NSBitmapImageRep* bitmap_reps_[kFrameBufferCount]{};
  __strong NSImage* canvas_images_[kFrameBufferCount]{};
  CVDisplayLinkRef display_link_{nullptr};
  std::atomic_bool frame_due_{true};
  bool metal_ready_{false};
  __strong id<MTLDevice> metal_device_{nil};
  __strong id<MTLCommandQueue> metal_queue_{nil};
  __strong id<MTLRenderPipelineState> metal_pipeline_{nil};
  __strong id<MTLTexture> metal_texture_{nil};
  __strong CAMetalLayer* metal_layer_{nil};
};

std::unique_ptr<IWindowShell> make_cocoa_window_shell() {
  return std::make_unique<CocoaWindowShell>();
}

} // namespace pluss::view::window

#endif
