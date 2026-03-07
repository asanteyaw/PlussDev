#if defined(__APPLE__)

#include "text_raster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>

namespace pluss::view::window::detail {

namespace {

std::vector<std::string> split_font_family_list(const std::string& css_value) {
  std::vector<std::string> out;
  std::string token;
  token.reserve(css_value.size());
  bool in_quote = false;
  char quote = '\0';

  const auto flush = [&]() {
    std::size_t begin = 0;
    while (begin < token.size() && std::isspace(static_cast<unsigned char>(token[begin])) != 0) {
      ++begin;
    }
    std::size_t end = token.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(token[end - 1])) != 0) {
      --end;
    }
    if (end > begin) {
      std::string cleaned = token.substr(begin, end - begin);
      if (!cleaned.empty() && (cleaned.front() == '\'' || cleaned.front() == '"') &&
          cleaned.back() == cleaned.front() && cleaned.size() >= 2) {
        cleaned = cleaned.substr(1, cleaned.size() - 2);
      }
      if (!cleaned.empty()) {
        out.push_back(std::move(cleaned));
      }
    }
    token.clear();
  };

  for (const char ch : css_value) {
    if ((ch == '\'' || ch == '"')) {
      if (!in_quote) {
        in_quote = true;
        quote = ch;
      } else if (quote == ch) {
        in_quote = false;
      }
      token.push_back(ch);
      continue;
    }
    if (ch == ',' && !in_quote) {
      flush();
      continue;
    }
    token.push_back(ch);
  }
  flush();
  return out;
}

NSFont* font_from_family_list(const std::string& family_list, const CGFloat size) {
  if (family_list.empty()) {
    return nil;
  }
  const auto families = split_font_family_list(family_list);
  for (const auto& family : families) {
    if (family.empty()) {
      continue;
    }
    std::string lower;
    lower.reserve(family.size());
    for (const char ch : family) {
      lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (lower == "system-ui" || lower == "sans-serif" || lower == "ui-sans-serif") {
      return [NSFont systemFontOfSize:size];
    }
    if (lower == "serif" || lower == "ui-serif") {
      return [NSFont fontWithName:@"Times New Roman" size:size];
    }
    if (lower == "monospace" || lower == "ui-monospace") {
      return [NSFont fontWithName:@"Menlo-Regular" size:size];
    }

    NSString* ns_name = [NSString stringWithUTF8String:family.c_str()];
    if (ns_name == nil) {
      continue;
    }
    NSFont* named = [NSFont fontWithName:ns_name size:size];
    if (named != nil) {
      return named;
    }
  }
  return nil;
}

} // namespace

bool render_text_command_native(PixelCanvas& canvas,
                                const TextCommand& command,
                                std::vector<RenderEvent>& events) {
  (void)events;
  if (canvas.empty() || command.text.empty() || command.pixel_size <= 0) {
    return false;
  }

  @autoreleasepool {
    NSString* text = [NSString stringWithUTF8String:command.text.c_str()];
    if (text == nil) {
      return false;
    }

    CGFloat weight = NSFontWeightRegular;
    if (command.font_weight >= 700) {
      weight = NSFontWeightSemibold;
    } else if (command.font_weight >= 600) {
      weight = NSFontWeightMedium;
    } else if (command.font_weight <= 300) {
      weight = NSFontWeightLight;
    }

    NSFont* font = nil;
    NSFont* family_font = font_from_family_list(command.font_family, static_cast<CGFloat>(command.pixel_size));
    if (family_font != nil) {
      font = family_font;
    }
    if ([NSFont respondsToSelector:@selector(systemFontOfSize:weight:)]) {
      if (font == nil) {
        font = [NSFont systemFontOfSize:static_cast<CGFloat>(command.pixel_size) weight:weight];
      } else {
        NSFontManager* mgr = [NSFontManager sharedFontManager];
        const NSInteger traits =
            (command.font_weight >= 700) ? NSBoldFontMask : ((command.font_weight <= 300) ? NSUnboldFontMask : 0);
        if (traits != 0) {
          NSFont* converted = [mgr convertFont:font toHaveTrait:traits];
          if (converted != nil) {
            font = converted;
          }
        }
      }
    }
    if (font == nil) {
      font = [NSFont systemFontOfSize:static_cast<CGFloat>(command.pixel_size)];
    }
    if (font == nil) {
      return false;
    }

    CTFontRef ct_font = CTFontCreateWithName((__bridge CFStringRef)[font fontName], static_cast<CGFloat>(command.pixel_size), nullptr);
    if (ct_font == nullptr) {
      return false;
    }

    CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
    if (color_space == nullptr) {
      CFRelease(ct_font);
      return false;
    }

    CGFloat white_components[4] = {1.0, 1.0, 1.0, 1.0};
    CGColorRef white = CGColorCreate(color_space, white_components);
    if (white == nullptr) {
      CGColorSpaceRelease(color_space);
      CFRelease(ct_font);
      return false;
    }

    NSDictionary* attrs = @{
      (id)kCTFontAttributeName : (__bridge id)ct_font,
      (id)kCTForegroundColorAttributeName : (__bridge id)white,
    };
    NSAttributedString* attributed = [[NSAttributedString alloc] initWithString:text attributes:attrs];
    if (attributed == nil) {
      CGColorRelease(white);
      CGColorSpaceRelease(color_space);
      CFRelease(ct_font);
      return false;
    }

    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)attributed);
    if (line == nullptr) {
      CGColorRelease(white);
      CGColorSpaceRelease(color_space);
      CFRelease(ct_font);
      return false;
    }

    CGFloat ascent = 0.0;
    CGFloat descent = 0.0;
    CGFloat leading = 0.0;
    const double text_width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    const int bitmap_w = std::max(1, static_cast<int>(std::ceil(text_width)) + 6);
    const int bitmap_h = std::max(1, static_cast<int>(std::ceil(ascent + descent + leading)) + 6);

    std::vector<std::uint8_t> bitmap(static_cast<std::size_t>(bitmap_w) * static_cast<std::size_t>(bitmap_h) * 4U, 0U);
    const CGBitmapInfo bitmap_info =
        static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) | static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Big);
    CGContextRef context = CGBitmapContextCreate(bitmap.data(),
                                                 static_cast<std::size_t>(bitmap_w),
                                                 static_cast<std::size_t>(bitmap_h),
                                                 8,
                                                 static_cast<std::size_t>(bitmap_w) * 4U,
                                                 color_space,
                                                 bitmap_info);
    if (context == nullptr) {
      CFRelease(line);
      CGColorRelease(white);
      CGColorSpaceRelease(color_space);
      CFRelease(ct_font);
      return false;
    }

    CGContextSetShouldAntialias(context, true);
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetInterpolationQuality(context, kCGInterpolationHigh);
    CGContextTranslateCTM(context, 0.0, static_cast<CGFloat>(bitmap_h));
    CGContextScaleCTM(context, 1.0, -1.0);
    CGContextSetTextPosition(context, 3.0, 2.0 + descent);
    CTLineDraw(line, context);
    CGContextFlush(context);

    const int bytes_per_row = bitmap_w * 4;
    const int spp = 4;
    const int top = command.anchor.y - static_cast<int>(std::ceil(ascent));
    const int left = command.anchor.x;
    for (int y = 0; y < bitmap_h; ++y) {
      const int src_y = (bitmap_h - 1) - y;
      const auto* row = bitmap.data() + static_cast<std::size_t>(src_y) * static_cast<std::size_t>(bytes_per_row);
      for (int x = 0; x < bitmap_w; ++x) {
        const int idx = x * spp;
        std::uint8_t coverage = 0;
        coverage = row[idx + 3];
        if (coverage == 0 && spp >= 3) {
          coverage = std::max(row[idx], std::max(row[idx + 1], row[idx + 2]));
        } else if (coverage == 0) {
          coverage = row[idx];
        }
        if (coverage == 0) {
          continue;
        }
        canvas.blend_pixel(PointI{left + x, top + y}, command.color, coverage);
      }
    }

    CGContextRelease(context);
    CFRelease(line);
    CGColorRelease(white);
    CGColorSpaceRelease(color_space);
    CFRelease(ct_font);
    return true;
  }
}

} // namespace pluss::view::window::detail

#endif
