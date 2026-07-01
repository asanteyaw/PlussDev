#if defined(__APPLE__)

#import <Foundation/Foundation.h>
#include "text_raster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>

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

CTFontRef apply_font_weight(CTFontRef font, const int weight) {
  if (font == nullptr || weight < 600) {
    return font;
  }
  CTFontRef bold = CTFontCreateCopyWithSymbolicTraits(
      font, 0.0, nullptr, kCTFontBoldTrait, kCTFontBoldTrait);
  if (bold != nullptr) {
    CFRelease(font);
    return bold;
  }
  return font;
}

CTFontRef font_from_family_list(const std::string& family_list,
                                const CGFloat size,
                                const int weight) {
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
      return apply_font_weight(
          CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, size, nullptr),
          weight);
    }
    if (lower == "serif" || lower == "ui-serif") {
      return apply_font_weight(
          CTFontCreateWithName(CFSTR("Times New Roman"), size, nullptr),
          weight);
    }
    if (lower == "monospace" || lower == "ui-monospace") {
      return apply_font_weight(
          CTFontCreateWithName(CFSTR("Menlo"), size, nullptr),
          weight);
    }

    NSString* ns_name = [NSString stringWithUTF8String:family.c_str()];
    if (ns_name == nil) {
      continue;
    }
    CTFontRef named =
        CTFontCreateWithName((__bridge CFStringRef)ns_name, size, nullptr);
    if (named != nullptr) {
      return apply_font_weight(named, weight);
    }
  }
  return apply_font_weight(
      CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, size, nullptr),
      weight);
}

} // namespace

std::optional<int> measure_text_width_native(const TextCommand& command) {
  if (command.text.empty() || command.pixel_size <= 0) {
    return 0;
  }
  @autoreleasepool {
    NSString* text = [NSString stringWithUTF8String:command.text.c_str()];
    CTFontRef font =
        font_from_family_list(command.font_family,
                              static_cast<CGFloat>(command.pixel_size),
                              command.font_weight);
    if (text == nil || font == nullptr) {
      if (font != nullptr) {
        CFRelease(font);
      }
      return std::nullopt;
    }
    NSDictionary* attrs = @{
      (id)kCTFontAttributeName : (__bridge id)font,
    };
    NSAttributedString* attributed =
        [[NSAttributedString alloc] initWithString:text attributes:attrs];
    CTLineRef line =
        attributed != nil
            ? CTLineCreateWithAttributedString(
                  (__bridge CFAttributedStringRef)attributed)
            : nullptr;
    if (line == nullptr) {
      CFRelease(font);
      return std::nullopt;
    }
    const double full_advance =
        CTLineGetTypographicBounds(line, nullptr, nullptr, nullptr);
    NSUInteger ink_length = text.length;
    NSCharacterSet* whitespace =
        [NSCharacterSet whitespaceAndNewlineCharacterSet];
    while (ink_length > 0 &&
           [whitespace characterIsMember:
                           [text characterAtIndex:ink_length - 1]]) {
      --ink_length;
    }
    NSString* ink_text = [text substringToIndex:ink_length];
    double ink_advance = 0.0;
    int ink_right_exclusive = 0;
    if (ink_text != nil && ink_text.length > 0) {
      NSAttributedString* ink_attributed =
          [[NSAttributedString alloc] initWithString:ink_text attributes:attrs];
      CTLineRef ink_line =
          ink_attributed != nil
              ? CTLineCreateWithAttributedString(
                    (__bridge CFAttributedStringRef)ink_attributed)
              : nullptr;
      if (ink_line != nullptr) {
        CGFloat ascent = 0.0;
        CGFloat descent = 0.0;
        CGFloat leading = 0.0;
        ink_advance = CTLineGetTypographicBounds(
            ink_line, &ascent, &descent, &leading);
        const int bitmap_w =
            std::max(1, static_cast<int>(std::ceil(ink_advance)) + 8);
        const int bitmap_h = std::max(
            1, static_cast<int>(std::ceil(ascent + descent + leading)) + 8);
        std::vector<std::uint8_t> bitmap(
            static_cast<std::size_t>(bitmap_w) *
                static_cast<std::size_t>(bitmap_h) * 4U,
            0U);
        CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
        const CGBitmapInfo bitmap_info =
            static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedLast) |
            static_cast<CGBitmapInfo>(kCGBitmapByteOrder32Big);
        CGContextRef context =
            color_space != nullptr
                ? CGBitmapContextCreate(
                      bitmap.data(), static_cast<std::size_t>(bitmap_w),
                      static_cast<std::size_t>(bitmap_h), 8,
                      static_cast<std::size_t>(bitmap_w) * 4U, color_space,
                      bitmap_info)
                : nullptr;
        if (context != nullptr) {
          CGContextSetShouldAntialias(context, true);
          CGContextSetAllowsAntialiasing(context, true);
          CGContextTranslateCTM(context, 0.0,
                                static_cast<CGFloat>(bitmap_h));
          CGContextScaleCTM(context, 1.0, -1.0);
          CGContextSetTextPosition(context, 3.0, 2.0 + descent);
          CTLineDraw(ink_line, context);
          CGContextFlush(context);
          for (int x = bitmap_w - 1; x >= 0 && ink_right_exclusive == 0; --x) {
            for (int y = 0; y < bitmap_h; ++y) {
              const std::size_t alpha =
                  (static_cast<std::size_t>(y) *
                       static_cast<std::size_t>(bitmap_w) +
                   static_cast<std::size_t>(x)) *
                      4U +
                  3U;
              if (bitmap[alpha] != 0U) {
                ink_right_exclusive = x + 1;
                break;
              }
            }
          }
          CGContextRelease(context);
        }
        if (color_space != nullptr) {
          CGColorSpaceRelease(color_space);
        }
        CFRelease(ink_line);
      }
    }
    // This is the exact right edge of the same padded CoreText bitmap used by
    // render_text_command_native. Only explicit trailing whitespace advances
    // the caret beyond the final painted pixel.
    const double trailing_advance = std::max(0.0, full_advance - ink_advance);
    const int width =
        ink_right_exclusive > 0
            ? std::max(0, static_cast<int>(
                              std::ceil(ink_right_exclusive +
                                        trailing_advance)))
            : std::max(
                  0, static_cast<int>(std::ceil(full_advance)));
    CFRelease(line);
    CFRelease(font);
    return width;
  }
}

bool render_text_command_native(PixelCanvas& canvas,
                                const TextCommand& command,
                                const std::optional<RectI>& clip,
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

    CTFontRef ct_font =
        font_from_family_list(command.font_family,
                              static_cast<CGFloat>(command.pixel_size),
                              command.font_weight);
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
    std::size_t coverage_pixels = 0;
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
        ++coverage_pixels;
        const PointI target{left + x, top + y};
        if (clip.has_value()) {
          const RectI rect = *clip;
          if (rect.empty() || target.x < rect.x || target.y < rect.y || target.x >= (rect.x + rect.width) ||
              target.y >= (rect.y + rect.height)) {
            continue;
          }
        }
        canvas.blend_pixel(target, command.color, coverage);
      }
    }

    CGContextRelease(context);
    CFRelease(line);
    CGColorRelease(white);
    CGColorSpaceRelease(color_space);
    CFRelease(ct_font);
    const std::size_t visible_characters =
        static_cast<std::size_t>(std::count_if(
            command.text.begin(), command.text.end(),
            [](const char ch) {
              return std::isspace(static_cast<unsigned char>(ch)) == 0 &&
                     (static_cast<unsigned char>(ch) & 0xC0U) != 0x80U;
            }));
    const std::size_t minimum_coverage =
        visible_characters *
        static_cast<std::size_t>(std::max(2, command.pixel_size / 2));
    // CoreText occasionally reports a line before its glyph raster is ready
    // on a background thread. Let the deterministic software glyph path
    // render this command instead of publishing missing or partial text.
    return coverage_pixels >= minimum_coverage;
  }
}

} // namespace pluss::view::window::detail

#endif
