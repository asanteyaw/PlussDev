#if defined(__APPLE__)

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {

bool find_window(const std::string& requested_title, CGRect& bounds, pid_t& owner_pid) {
  CFArrayRef raw_windows = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
      kCGNullWindowID);
  if (raw_windows == nullptr) {
    return false;
  }

  NSArray* windows = CFBridgingRelease(raw_windows);
  NSString* title = [NSString stringWithUTF8String:requested_title.c_str()];
  for (NSDictionary* window in windows) {
    NSString* name = window[(id)kCGWindowName];
    if (name == nil || ![name isEqualToString:title]) {
      continue;
    }
    NSDictionary* raw_bounds = window[(id)kCGWindowBounds];
    if (raw_bounds != nil &&
        CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)raw_bounds, &bounds)) {
      owner_pid = [window[(id)kCGWindowOwnerPID] intValue];
      return true;
    }
  }
  return false;
}

void post_mouse(const pid_t owner_pid, const CGEventType type, const CGPoint point) {
  CGEventRef event = CGEventCreateMouseEvent(nullptr, type, point, kCGMouseButtonLeft);
  if (event != nullptr) {
    CGEventPostToPid(owner_pid, event);
    CFRelease(event);
  }
}

void move_and_click(const pid_t owner_pid, const CGPoint point) {
  post_mouse(owner_pid, kCGEventMouseMoved, point);
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
  post_mouse(owner_pid, kCGEventLeftMouseDown, point);
  post_mouse(owner_pid, kCGEventLeftMouseUp, point);
  std::this_thread::sleep_for(std::chrono::milliseconds(120));
}

void post_text(const pid_t owner_pid, const std::string& text) {
  NSString* value = [NSString stringWithUTF8String:text.c_str()];
  for (NSUInteger index = 0; index < [value length]; ++index) {
    const UniChar character = [value characterAtIndex:index];
    CGEventRef down = CGEventCreateKeyboardEvent(nullptr, 0, true);
    CGEventRef up = CGEventCreateKeyboardEvent(nullptr, 0, false);
    if (down != nullptr && up != nullptr) {
      CGEventKeyboardSetUnicodeString(down, 1, &character);
      CGEventKeyboardSetUnicodeString(up, 1, &character);
      CGEventPostToPid(owner_pid, down);
      CGEventPostToPid(owner_pid, up);
    }
    if (down != nullptr) CFRelease(down);
    if (up != nullptr) CFRelease(up);
  }
}

} // namespace

int main(int argc, char** argv) {
  if (argc < 5) {
    std::cerr << "usage: macos_input_driver <click|click-text|scroll> <window-title> <x-ratio> <y-ratio> [args]\n";
    return 2;
  }
  const std::string action = argv[1];
  const std::string title = argv[2];
  const double x_ratio = std::strtod(argv[3], nullptr);
  const double y_ratio = std::strtod(argv[4], nullptr);
  CGRect bounds{};
  pid_t owner_pid = 0;
  if (!find_window(title, bounds, owner_pid)) {
    std::cerr << "window not found: " << title << '\n';
    return 3;
  }

  const CGPoint point{
      bounds.origin.x + bounds.size.width * x_ratio,
      bounds.origin.y + bounds.size.height * y_ratio,
  };

  if (action == "click") {
    move_and_click(owner_pid, point);
    return 0;
  }
  if (action == "click-text" && argc >= 6) {
    move_and_click(owner_pid, point);
    post_text(owner_pid, argv[5]);
    return 0;
  }
  if (action == "scroll" && argc >= 7) {
    post_mouse(owner_pid, kCGEventMouseMoved, point);
    const std::int32_t dx = static_cast<std::int32_t>(std::strtol(argv[5], nullptr, 10));
    const std::int32_t dy = static_cast<std::int32_t>(std::strtol(argv[6], nullptr, 10));
    CGEventRef event =
        CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 2, dy, dx);
    if (event == nullptr) {
      return 4;
    }
    CGEventSetLocation(event, point);
    CGEventPostToPid(owner_pid, event);
    CFRelease(event);
    return 0;
  }

  std::cerr << "invalid action or arguments\n";
  return 2;
}

#endif
