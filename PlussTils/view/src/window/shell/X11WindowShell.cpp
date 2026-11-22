#if defined(PLUSSTILS_VIEW_HAVE_X11_SHELL) && PLUSSTILS_VIEW_HAVE_X11_SHELL

#include "pluss/tils/view/window/shell.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
#include <X11/extensions/XInput2.h>
#endif

#include <algorithm>
#include <clocale>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::view::window {

namespace {

[[nodiscard]] unsigned long pack_component(const std::uint8_t value, const unsigned long mask) noexcept {
  if (mask == 0) {
    return 0;
  }
  unsigned int shift = 0;
  unsigned long shifted_mask = mask;
  while ((shifted_mask & 1UL) == 0UL) {
    shifted_mask >>= 1U;
    ++shift;
  }
  const unsigned long scaled = (static_cast<unsigned long>(value) * shifted_mask + 127UL) / 255UL;
  return (scaled << shift) & mask;
}

[[nodiscard]] std::string shortcut_for_key(const KeySym key, const unsigned int state) {
  const bool control = (state & ControlMask) != 0U;
  if (control && (key == XK_plus || key == XK_equal)) return "zoom_in";
  if (control && (key == XK_minus || key == XK_underscore)) return "zoom_out";
  if (control && key == XK_0) return "reset_view";
  if (control && (key == XK_s || key == XK_S)) return "save_snapshot";
  if (control && (key == XK_j || key == XK_J)) return "next_page";
  if (control && (key == XK_k || key == XK_K)) return "previous_page";
  if (key == XK_Page_Down) return "page_down";
  if (key == XK_Page_Up) return "page_up";
  if (key == XK_Home) return "home";
  if (key == XK_End) return "end";
  if (key == XK_Left) return "pan_left";
  if (key == XK_Right) return "pan_right";
  if (key == XK_Up) return "pan_up";
  if (key == XK_Down) return "pan_down";
  return {};
}

[[nodiscard]] bool is_text_modifier(const unsigned int state) noexcept {
  return (state & (ControlMask | Mod1Mask | Mod4Mask)) == 0U;
}

// Toolbar and menu geometry, in pixels. Every button is as wide as its own label needs.
constexpr int kToolbarHeightPx = 46;
constexpr int kToolbarMarginPx = 12;      // gap to the window edge
constexpr int kToolbarGapPx = 8;          // gap between neighbouring buttons
constexpr int kToolbarButtonHeightPx = 30;
constexpr int kToolbarButtonPadPx = 28;   // padding left and right of a label, together
constexpr int kArrowPx = 8;               // the pull-down arrow beside a label
constexpr int kPopupRowHeightPx = 28;
constexpr int kPopupPadPx = 14;           // left and right padding of a menu row
constexpr int kPopupIndentPx = 16;        // extra left padding per nesting level
constexpr int kPopupCheckPx = 18;         // room kept for a check mark
constexpr int kPopupMinWidthPx = 140;

struct Rgb {
  std::uint8_t r, g, b;
};
constexpr Rgb kToolbarFill{238, 240, 244};
constexpr Rgb kButtonFill{255, 255, 255};
constexpr Rgb kButtonLine{188, 194, 204};
constexpr Rgb kInk{30, 34, 43};
constexpr Rgb kMutedInk{110, 118, 132};
constexpr Rgb kHoverFill{205, 222, 246};

} // namespace

class X11WindowShell final : public IWindowShell {
 public:
  ~X11WindowShell() override { close(); }

  bool open(const ShellConfig& config) override {
    close();
    std::setlocale(LC_CTYPE, "");
    XSetLocaleModifiers("");

    display_ = XOpenDisplay(nullptr);
    if (display_ == nullptr) {
      events_.push_back(ShellEvent{ShellEventKind::Error, {}, "x11_display_open_failed"});
      return false;
    }

    screen_ = DefaultScreen(display_);
    visual_ = DefaultVisual(display_, screen_);
    depth_ = DefaultDepth(display_, screen_);
    const int width = std::max(640, config.initial_size.width);
    const int height = std::max(420, config.initial_size.height);
    menus_ = config.menus;
    toolbar_height_ = (config.save_formats.empty() && config.menus.empty()) ? 0 : kToolbarHeightPx;
    window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 0, 0, static_cast<unsigned int>(width),
                                  static_cast<unsigned int>(height + toolbar_height_), 0, BlackPixel(display_, screen_),
                                  WhitePixel(display_, screen_));
    if (window_ == 0) {
      events_.push_back(ShellEvent{ShellEventKind::Error, {}, "x11_window_create_failed"});
      close();
      return false;
    }

    const long event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
                            PointerMotionMask | FocusChangeMask | PropertyChangeMask;
    XSelectInput(display_, window_, event_mask);

    wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    (void)XSetWMProtocols(display_, window_, &wm_delete_, 1);
    clipboard_atom_ = XInternAtom(display_, "CLIPBOARD", False);
    utf8_atom_ = XInternAtom(display_, "UTF8_STRING", False);
    targets_atom_ = XInternAtom(display_, "TARGETS", False);

    const std::string title = config.title.empty() ? std::string("PlussTilsView") : config.title;
    XStoreName(display_, window_, title.c_str());
    XClassHint class_hint{};
    class_hint.res_name = const_cast<char*>("plussview");
    class_hint.res_class = const_cast<char*>("PlussView");
    XSetClassHint(display_, window_, &class_hint);

    XSizeHints size_hints{};
    size_hints.flags = PMinSize;
    size_hints.min_width = 640;
    size_hints.min_height = 420;
    XSetWMNormalHints(display_, window_, &size_hints);

    gc_ = XCreateGC(display_, window_, 0, nullptr);
    if (gc_ == nullptr) {
      events_.push_back(ShellEvent{ShellEventKind::Error, {}, "x11_gc_create_failed"});
      close();
      return false;
    }

    input_method_ = XOpenIM(display_, nullptr, nullptr, nullptr);
    if (input_method_ != nullptr) {
      input_context_ = XCreateIC(input_method_, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                                 window_, XNFocusWindow, window_, nullptr);
    }

#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
    initialize_smooth_scrolling();
#endif

    save_formats_ = config.save_formats;
    size_ = SizeI{width, height};
    font_ = XLoadQueryFont(display_, "-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1");
    if (font_ == nullptr) {
      font_ = XLoadQueryFont(display_, "fixed");
    }
    layout_toolbar(width);
    XMapRaised(display_, window_);
    XFlush(display_);
    open_ = true;
    events_.push_back(ShellEvent{ShellEventKind::Opened, size_, "x11_shell_opened"});
    return true;
  }

  bool present(const PixelCanvas& canvas) override {
    if (!open_ || display_ == nullptr || window_ == 0 || gc_ == nullptr || canvas.empty()) {
      events_.push_back(ShellEvent{ShellEventKind::Error, size_, "x11_shell_present_failed"});
      return false;
    }

    const auto canvas_size = canvas.size();
    const auto bytes = canvas.bytes();
    pixels_.resize(static_cast<std::size_t>(canvas_size.width) * static_cast<std::size_t>(canvas_size.height));
    for (std::size_t index = 0; index < pixels_.size(); ++index) {
      const std::size_t source = index * 4U;
      pixels_[index] = pack_component(bytes[source], visual_->red_mask) |
                       pack_component(bytes[source + 1U], visual_->green_mask) |
                       pack_component(bytes[source + 2U], visual_->blue_mask);
    }
    pixel_size_ = canvas_size;
    blit();
    events_.push_back(ShellEvent{ShellEventKind::Presented, size_, "x11_shell_presented"});
    return true;
  }

  void close() noexcept override {
    const bool was_open = open_;
    open_ = false;
    if (input_context_ != nullptr) {
      XDestroyIC(input_context_);
      input_context_ = nullptr;
    }
    if (input_method_ != nullptr) {
      XCloseIM(input_method_);
      input_method_ = nullptr;
    }
    close_popup();
    if (display_ != nullptr && font_ != nullptr) {
      XFreeFont(display_, font_);
      font_ = nullptr;
    }
    if (display_ != nullptr && gc_ != nullptr) {
      XFreeGC(display_, gc_);
      gc_ = nullptr;
    }
    if (display_ != nullptr && window_ != 0) {
      XDestroyWindow(display_, window_);
      window_ = 0;
    }
    if (display_ != nullptr) {
      XCloseDisplay(display_);
      display_ = nullptr;
    }
    pixels_.clear();
    clipboard_.clear();
    if (was_open) {
      events_.push_back(ShellEvent{ShellEventKind::Closed, size_, "x11_shell_closed"});
    }
  }

  std::vector<ShellEvent> poll_events() override {
    if (display_ != nullptr) {
      while (XPending(display_) > 0) {
        XEvent event{};
        XNextEvent(display_, &event);
        handle_event(event);
      }
    }
    auto buffered = std::move(events_);
    events_.clear();
    return buffered;
  }

  [[nodiscard]] bool supports_menus() const noexcept override { return true; }

  void update_menus(const std::vector<ShellMenu>& menus) override {
    menus_ = menus;
  }

  [[nodiscard]] ShellBackend backend() const noexcept override { return ShellBackend::X11; }

  [[nodiscard]] ShellCapabilities capabilities() const noexcept override {
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

  bool set_clipboard_text(const std::string_view text) override {
    if (!open_ || display_ == nullptr || window_ == 0) {
      return false;
    }
    clipboard_.assign(text);
    XSetSelectionOwner(display_, clipboard_atom_, window_, CurrentTime);
    XSetSelectionOwner(display_, XA_PRIMARY, window_, CurrentTime);
    XFlush(display_);
    return XGetSelectionOwner(display_, clipboard_atom_) == window_;
  }

  // Asks for a file name with the desktop's own dialog (zenity or kdialog) when one is installed,
  // and otherwise saves into the current folder under the suggested name.
  [[nodiscard]] std::string choose_save_path(const std::string_view suggested_name,
                                             const std::string_view extension) override {
    std::string filename = suggested_name.empty() ? std::string("plussview") : std::string(suggested_name);
    if (!extension.empty()) {
      const std::string suffix = "." + std::string(extension);
      if (filename.size() < suffix.size() || filename.substr(filename.size() - suffix.size()) != suffix) {
        filename += suffix;
      }
    }
    std::error_code error;
    const auto directory = std::filesystem::current_path(error);
    const std::string fallback = error ? filename : (directory / filename).string();
    const std::string start = fallback;
    const std::string quoted = shell_quote(start);
    const std::string commands[] = {
        "zenity --file-selection --save --confirm-overwrite --filename=" + quoted + " 2>/dev/null",
        "kdialog --getsavefilename " + quoted + " 2>/dev/null",
    };
    for (const auto& command : commands) {
      const std::string tool = command.substr(0, command.find(' '));
      if (std::system(("command -v " + tool + " >/dev/null 2>&1").c_str()) != 0) {
        continue;
      }
      std::string chosen;
      if (FILE* pipe = popen(command.c_str(), "r")) {
        char buffer[512];
        while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          chosen += buffer;
        }
        const int status = pclose(pipe);
        while (!chosen.empty() && (chosen.back() == '\n' || chosen.back() == '\r')) {
          chosen.pop_back();
        }
        if (status != 0 || chosen.empty()) {
          return {};  // the user cancelled
        }
        return chosen;
      }
    }
    return fallback;
  }

 private:
  [[nodiscard]] static std::string shell_quote(const std::string& text) {
    std::string quoted = "'";
    for (const char ch : text) {
      if (ch == '\'') {
        quoted += "'\\''";
      } else {
        quoted += ch;
      }
    }
    return quoted + "'";
  }

  [[nodiscard]] unsigned long pixel_of(const Rgb color) const noexcept {
    return pack_component(color.r, visual_->red_mask) | pack_component(color.g, visual_->green_mask) |
           pack_component(color.b, visual_->blue_mask);
  }

  [[nodiscard]] int text_width(const std::string& text) const {
    return font_ != nullptr ? XTextWidth(font_, text.c_str(), static_cast<int>(text.size())) : 8 * static_cast<int>(text.size());
  }

  void draw_text(const Drawable target, const int x, const int center_y, const std::string& text, const Rgb color) {
    if (font_ == nullptr) {
      return;
    }
    XSetForeground(display_, gc_, pixel_of(color));
    XSetFont(display_, gc_, font_->fid);
    const int baseline = center_y + (font_->ascent - font_->descent) / 2;
    XDrawString(display_, target, gc_, x, baseline, text.c_str(), static_cast<int>(text.size()));
  }

  void draw_arrow(const Drawable target, const int x, const int center_y, const Rgb color) {
    XSetForeground(display_, gc_, pixel_of(color));
    XPoint points[3] = {{static_cast<short>(x), static_cast<short>(center_y - kArrowPx / 4)},
                        {static_cast<short>(x + kArrowPx), static_cast<short>(center_y - kArrowPx / 4)},
                        {static_cast<short>(x + kArrowPx / 2), static_cast<short>(center_y + kArrowPx / 4 + 1)}};
    XFillPolygon(display_, target, gc_, points, 3, Convex, CoordModeOrigin);
  }

  // Buttons run right to left from the window edge: "Save As" first, then the menus, the last
  // menu closest to it.
  void layout_toolbar(const int client_width) {
    toolbar_buttons_.clear();
    if (toolbar_height_ <= 0) {
      return;
    }
    const int y = (toolbar_height_ - kToolbarButtonHeightPx) / 2;
    int right_edge = client_width - kToolbarMarginPx;
    const auto place = [&](const std::string& label, const int menu_index) {
      const int width = text_width(label) + kArrowPx + kToolbarButtonPadPx;
      right_edge -= width;
      toolbar_buttons_.push_back(ToolbarButton{std::max(kToolbarMarginPx, right_edge), y, width,
                                               kToolbarButtonHeightPx, label, menu_index});
      right_edge -= kToolbarGapPx;
    };
    if (!save_formats_.empty()) {
      place("Save As", -1);
    }
    for (std::size_t index = menus_.size(); index-- > 0;) {
      place(menus_[index].title.empty() ? std::string("Menu") : menus_[index].title, static_cast<int>(index));
    }
  }

  void draw_toolbar() {
    if (toolbar_height_ <= 0 || display_ == nullptr || gc_ == nullptr) {
      return;
    }
    XSetForeground(display_, gc_, pixel_of(kToolbarFill));
    XFillRectangle(display_, window_, gc_, 0, 0, static_cast<unsigned int>(std::max(1, size_.width)),
                   static_cast<unsigned int>(toolbar_height_));
    for (const auto& button : toolbar_buttons_) {
      XSetForeground(display_, gc_, pixel_of(kButtonFill));
      XFillRectangle(display_, window_, gc_, button.x, button.y, static_cast<unsigned int>(button.w),
                     static_cast<unsigned int>(button.h));
      XSetForeground(display_, gc_, pixel_of(kButtonLine));
      XDrawRectangle(display_, window_, gc_, button.x, button.y, static_cast<unsigned int>(button.w - 1),
                     static_cast<unsigned int>(button.h - 1));
      const int center_y = button.y + button.h / 2;
      draw_text(window_, button.x + kToolbarButtonPadPx / 2, center_y, button.label, kInk);
      draw_arrow(window_, button.x + button.w - kToolbarButtonPadPx / 2 - kArrowPx, center_y, kMutedInk);
    }
    XFlush(display_);
  }

  // Flattens a menu into rows. A cascading entry becomes a heading with its children indented
  // beneath it, so one popup shows everything. A choice's note is "<prefix><id>".
  void append_rows(const std::vector<ShellMenuOption>& options, const std::string& prefix, const int level) {
    for (const auto& option : options) {
      if (!option.children.empty()) {
        popup_rows_.push_back(PopupRow{option.label, {}, false, true, level});
        append_rows(option.children, option.id + ":", level + 1);
      } else {
        popup_rows_.push_back(PopupRow{option.label, prefix + option.id, option.checked, false, level});
      }
    }
  }

  void open_popup(const int button_index) {
    close_popup();
    const ToolbarButton& button = toolbar_buttons_[static_cast<std::size_t>(button_index)];
    popup_rows_.clear();
    if (button.menu_index < 0) {
      for (const auto& format : save_formats_) {
        popup_rows_.push_back(PopupRow{format.label, "save_as:" + format.extension, false, false, 0});
      }
    } else {
      const auto& menu = menus_[static_cast<std::size_t>(button.menu_index)];
      append_rows(menu.options, menu.id + ":", 0);
    }
    if (popup_rows_.empty()) {
      return;
    }
    int width = kPopupMinWidthPx;
    for (const auto& row : popup_rows_) {
      width = std::max(width, text_width(row.label) + 2 * kPopupPadPx + kPopupCheckPx + row.level * kPopupIndentPx);
    }
    const int height = static_cast<int>(popup_rows_.size()) * kPopupRowHeightPx;
    Window child = 0;
    int root_x = 0;
    int root_y = 0;
    XTranslateCoordinates(display_, window_, RootWindow(display_, screen_), button.x, button.y + button.h, &root_x,
                          &root_y, &child);
    XSetWindowAttributes attributes{};
    attributes.override_redirect = True;
    attributes.background_pixel = pixel_of(kButtonFill);
    attributes.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask |
                            LeaveWindowMask;
    popup_window_ = XCreateWindow(display_, RootWindow(display_, screen_), root_x, root_y,
                                  static_cast<unsigned int>(width), static_cast<unsigned int>(height), 1, depth_,
                                  InputOutput, visual_, CWOverrideRedirect | CWBackPixel | CWEventMask, &attributes);
    popup_size_ = SizeI{width, height};
    popup_hover_ = -1;
    XMapRaised(display_, popup_window_);
    XGrabPointer(display_, popup_window_, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync,
                 GrabModeAsync, None, None, CurrentTime);
    XGrabKeyboard(display_, popup_window_, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    draw_popup();
  }

  void close_popup() {
    if (display_ == nullptr || popup_window_ == 0) {
      popup_window_ = 0;
      return;
    }
    XUngrabPointer(display_, CurrentTime);
    XUngrabKeyboard(display_, CurrentTime);
    XDestroyWindow(display_, popup_window_);
    popup_window_ = 0;
    popup_rows_.clear();
    XFlush(display_);
  }

  void draw_popup() {
    if (popup_window_ == 0) {
      return;
    }
    for (std::size_t index = 0; index < popup_rows_.size(); ++index) {
      const auto& row = popup_rows_[index];
      const int y = static_cast<int>(index) * kPopupRowHeightPx;
      const bool hovered = static_cast<int>(index) == popup_hover_ && !row.heading;
      XSetForeground(display_, gc_, pixel_of(hovered ? kHoverFill : kButtonFill));
      XFillRectangle(display_, popup_window_, gc_, 0, y, static_cast<unsigned int>(popup_size_.width),
                     static_cast<unsigned int>(kPopupRowHeightPx));
      const int center_y = y + kPopupRowHeightPx / 2;
      if (row.checked) {
        // A check mark: a small filled square, since core fonts have no tick glyph.
        XSetForeground(display_, gc_, pixel_of(kInk));
        XFillRectangle(display_, popup_window_, gc_, kPopupPadPx / 2 + row.level * kPopupIndentPx, center_y - 3, 6, 6);
      }
      draw_text(popup_window_, kPopupPadPx + kPopupCheckPx / 2 + row.level * kPopupIndentPx, center_y, row.label,
                row.heading ? kMutedInk : kInk);
    }
    XFlush(display_);
  }

  [[nodiscard]] int popup_row_at(const int y) const noexcept {
    if (y < 0) {
      return -1;
    }
    const int index = y / kPopupRowHeightPx;
    return index < static_cast<int>(popup_rows_.size()) ? index : -1;
  }

  void handle_popup_event(XEvent& event) {
    switch (event.type) {
      case Expose: draw_popup(); break;
      case MotionNotify: {
        const int row = popup_row_at(event.xmotion.y);
        if (row != popup_hover_) {
          popup_hover_ = row;
          draw_popup();
        }
        break;
      }
      case ButtonPress: {
        const bool inside = event.xbutton.x >= 0 && event.xbutton.y >= 0 && event.xbutton.x < popup_size_.width &&
                            event.xbutton.y < popup_size_.height;
        const int row = inside ? popup_row_at(event.xbutton.y) : -1;
        if (row >= 0 && !popup_rows_[static_cast<std::size_t>(row)].heading) {
          events_.push_back(ShellEvent{ShellEventKind::ShortcutCommand, size_,
                                       popup_rows_[static_cast<std::size_t>(row)].note});
          close_popup();
        } else if (!inside) {
          close_popup();
        }
        break;
      }
      case KeyPress: {
        char buffer[8]{};
        KeySym key = NoSymbol;
        XLookupString(&event.xkey, buffer, sizeof(buffer), &key, nullptr);
        if (key == XK_Escape) {
          close_popup();
        }
        break;
      }
      default: break;
    }
  }

  // A press on the toolbar opens that button's menu. Returns true when the press was the toolbar's.
  bool press_toolbar(const int x, const int y) {
    if (y >= toolbar_height_) {
      return false;
    }
    for (std::size_t index = 0; index < toolbar_buttons_.size(); ++index) {
      const auto& button = toolbar_buttons_[index];
      if (x >= button.x && x < button.x + button.w && y >= button.y && y < button.y + button.h) {
        open_popup(static_cast<int>(index));
        break;
      }
    }
    return true;
  }

  void blit() {
    if (display_ == nullptr || window_ == 0 || gc_ == nullptr || pixels_.empty() || pixel_size_.width <= 0 ||
        pixel_size_.height <= 0) {
      return;
    }
    XImage* image = XCreateImage(display_, visual_, static_cast<unsigned int>(depth_), ZPixmap, 0,
                                 reinterpret_cast<char*>(pixels_.data()), static_cast<unsigned int>(pixel_size_.width),
                                 static_cast<unsigned int>(pixel_size_.height), 32,
                                 static_cast<int>(static_cast<std::size_t>(pixel_size_.width) * sizeof(std::uint32_t)));
    if (image == nullptr) {
      return;
    }
    const int width = std::min(size_.width, pixel_size_.width);
    const int height = std::min(size_.height, pixel_size_.height);
    XPutImage(display_, window_, gc_, image, 0, 0, 0, toolbar_height_, static_cast<unsigned int>(std::max(0, width)),
              static_cast<unsigned int>(std::max(0, height)));
    image->data = nullptr;
    XDestroyImage(image);
    draw_toolbar();
    XFlush(display_);
  }

  void push_pointer_event(const ShellEventKind kind, const char* note, const double x, const double y,
                          const double dx = 0.0, const double dy = 0.0) {
    events_.push_back(ShellEvent{
        kind,
        size_,
        note,
        dx,
        dy,
        std::clamp(x, 0.0, static_cast<double>(size_.width)),
        std::clamp(y, 0.0, static_cast<double>(size_.height)),
    });
  }

  void handle_key(XKeyEvent& key_event) {
    KeySym key = NoSymbol;
    char buffer[128]{};
    int length = 0;
    if (input_context_ != nullptr) {
      Status status = 0;
      length = Xutf8LookupString(input_context_, &key_event, buffer, sizeof(buffer) - 1, &key, &status);
      if (status == XBufferOverflow) {
        length = 0;
      }
    } else {
      length = XLookupString(&key_event, buffer, sizeof(buffer) - 1, &key, nullptr);
    }

    const std::string shortcut = shortcut_for_key(key, key_event.state);
    if (!shortcut.empty()) {
      if (shortcut == "save_snapshot" && !save_formats_.empty()) {
        for (std::size_t index = 0; index < toolbar_buttons_.size(); ++index) {
          if (toolbar_buttons_[index].menu_index < 0) {
            open_popup(static_cast<int>(index));
            break;
          }
        }
        return;
      }
      events_.push_back(ShellEvent{ShellEventKind::ShortcutCommand, size_, shortcut});
      return;
    }
    if (!is_text_modifier(key_event.state)) {
      return;
    }
    if (key == XK_BackSpace || key == XK_Delete) {
      events_.push_back(ShellEvent{ShellEventKind::TextInput, size_, "__backspace__"});
      return;
    }
    if (length > 0) {
      buffer[std::min<int>(length, static_cast<int>(sizeof(buffer) - 1))] = '\0';
      std::string text(buffer, static_cast<std::size_t>(length));
      text.erase(std::remove_if(text.begin(), text.end(),
                                [](const char ch) {
                                  const auto value = static_cast<unsigned char>(ch);
                                  return value < 0x20U || value == 0x7FU;
                                }),
                 text.end());
      if (!text.empty()) {
        events_.push_back(ShellEvent{ShellEventKind::TextInput, size_, std::move(text)});
      }
    }
  }

  void respond_to_selection(const XSelectionRequestEvent& request) {
    XSelectionEvent response{};
    response.type = SelectionNotify;
    response.display = request.display;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.time = request.time;
    response.property = None;

    const Atom property = request.property == None ? request.target : request.property;
    if (request.target == targets_atom_) {
      const Atom targets[] = {utf8_atom_, XA_STRING, targets_atom_};
      XChangeProperty(display_, request.requestor, property, XA_ATOM, 32, PropModeReplace,
                      reinterpret_cast<const unsigned char*>(targets), 3);
      response.property = property;
    } else if (request.target == utf8_atom_ || request.target == XA_STRING) {
      XChangeProperty(display_, request.requestor, property, request.target, 8, PropModeReplace,
                      reinterpret_cast<const unsigned char*>(clipboard_.data()), static_cast<int>(clipboard_.size()));
      response.property = property;
    }
    XSendEvent(display_, request.requestor, False, 0, reinterpret_cast<XEvent*>(&response));
    XFlush(display_);
  }

  void handle_event(XEvent& event) {
    if (popup_window_ != 0 && event.xany.window == popup_window_) {
      handle_popup_event(event);
      return;
    }
    switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          blit();
        }
        break;
      case ConfigureNotify: {
        const SizeI next{
            std::max(1, event.xconfigure.width),
            std::max(1, event.xconfigure.height - toolbar_height_),
        };
        layout_toolbar(event.xconfigure.width);
        if (next.width != size_.width || next.height != size_.height) {
          size_ = next;
          events_.push_back(ShellEvent{ShellEventKind::ResizeRequested, size_, "x11_shell_resized"});
        }
        break;
      }
      case ClientMessage:
        if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_) {
          open_ = false;
          events_.push_back(ShellEvent{ShellEventKind::Closed, size_, "x11_shell_closed_by_user"});
        }
        break;
      case FocusIn:
        if (input_context_ != nullptr) XSetICFocus(input_context_);
        break;
      case FocusOut:
        if (input_context_ != nullptr) XUnsetICFocus(input_context_);
        break;
      case KeyPress: handle_key(event.xkey); break;
      case ButtonPress:
        if (event.xbutton.button == Button1) {
          if (press_toolbar(event.xbutton.x, event.xbutton.y)) {
            break;
          }
          dragging_ = true;
          last_pointer_ = PointI{event.xbutton.x, event.xbutton.y - toolbar_height_};
          push_pointer_event(ShellEventKind::PointerDown, "x11_pointer_down", event.xbutton.x,
                             event.xbutton.y - toolbar_height_);
        } else if (event.xbutton.button == Button4 || event.xbutton.button == Button5 || event.xbutton.button == 6 ||
                   event.xbutton.button == 7) {
#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
          if (last_smooth_time_ != 0 && event.xbutton.time >= last_smooth_time_ &&
              event.xbutton.time - last_smooth_time_ <= 8) {
            break;
          }
#endif
          double dx = 0.0;
          double dy = 0.0;
          if (event.xbutton.button == Button4) dy = 1.0;
          if (event.xbutton.button == Button5) dy = -1.0;
          if (event.xbutton.button == 6) dx = 1.0;
          if (event.xbutton.button == 7) dx = -1.0;
          events_.push_back(ShellEvent{
              ShellEventKind::PointerScroll,
              size_,
              "x11_scroll",
              dx,
              dy,
              static_cast<double>(event.xbutton.x),
              static_cast<double>(event.xbutton.y - toolbar_height_),
              false,
              (event.xbutton.state & ShiftMask) != 0U,
          });
        }
        break;
      case ButtonRelease:
        if (event.xbutton.button == Button1) {
          dragging_ = false;
          push_pointer_event(ShellEventKind::PointerUp, "x11_pointer_up", event.xbutton.x,
                             event.xbutton.y - toolbar_height_);
        }
        break;
      case MotionNotify:
        if (dragging_ || (event.xmotion.state & Button1Mask) != 0U) {
          const PointI next{event.xmotion.x, event.xmotion.y - toolbar_height_};
          push_pointer_event(ShellEventKind::PointerDrag, "x11_drag", next.x, next.y, next.x - last_pointer_.x,
                             next.y - last_pointer_.y);
          last_pointer_ = next;
        }
        break;
      case SelectionRequest: respond_to_selection(event.xselectionrequest); break;
      case SelectionClear: clipboard_.clear(); break;
#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
      case GenericEvent:
        if (event.xcookie.extension == xi_opcode_ && XGetEventData(display_, &event.xcookie)) {
          if (event.xcookie.evtype == XI_Motion) {
            handle_smooth_scroll(*static_cast<XIDeviceEvent*>(event.xcookie.data));
          }
          XFreeEventData(display_, &event.xcookie);
        }
        break;
#endif
      default: break;
    }
  }

#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
  struct SmoothScrollAxis {
    int device_id{0};
    int valuator{0};
    int scroll_type{0};
    double increment{1.0};
    double previous{0.0};
    bool initialized{false};
  };

  void initialize_smooth_scrolling() {
    int event_base = 0;
    int error_base = 0;
    if (!XQueryExtension(display_, "XInputExtension", &xi_opcode_, &event_base, &error_base)) {
      xi_opcode_ = -1;
      return;
    }
    int major = 2;
    int minor = 1;
    if (XIQueryVersion(display_, &major, &minor) == BadRequest) {
      xi_opcode_ = -1;
      return;
    }

    unsigned char mask_bits[XIMaskLen(XI_LASTEVENT)]{};
    XIEventMask mask{};
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = sizeof(mask_bits);
    mask.mask = mask_bits;
    XISetMask(mask.mask, XI_Motion);
    XISelectEvents(display_, window_, &mask, 1);

    int device_count = 0;
    XIDeviceInfo* devices = XIQueryDevice(display_, XIAllDevices, &device_count);
    if (devices == nullptr) {
      return;
    }
    for (int index = 0; index < device_count; ++index) {
      const XIDeviceInfo& device = devices[index];
      for (int class_index = 0; class_index < device.num_classes; ++class_index) {
        const XIAnyClassInfo* any = device.classes[class_index];
        if (any == nullptr || any->type != XIScrollClass) {
          continue;
        }
        const auto* scroll = reinterpret_cast<const XIScrollClassInfo*>(any);
        if (!std::isfinite(scroll->increment) || std::abs(scroll->increment) < 1e-9) {
          continue;
        }
        smooth_axes_.push_back(SmoothScrollAxis{
            .device_id = device.deviceid,
            .valuator = scroll->number,
            .scroll_type = scroll->scroll_type,
            .increment = scroll->increment,
        });
      }
    }
    XIFreeDeviceInfo(devices);
    XFlush(display_);
  }

  void handle_smooth_scroll(const XIDeviceEvent& event) {
    if (event.valuators.mask == nullptr || event.valuators.values == nullptr) {
      return;
    }
    const double* value = event.valuators.values;
    double dx = 0.0;
    double dy = 0.0;
    for (int valuator = 0; valuator < event.valuators.mask_len * 8; ++valuator) {
      if (!XIMaskIsSet(event.valuators.mask, valuator)) {
        continue;
      }
      const double current = *value++;
      const auto axis =
          std::find_if(smooth_axes_.begin(), smooth_axes_.end(), [&](const SmoothScrollAxis& candidate) {
            return candidate.valuator == valuator &&
                   (candidate.device_id == event.deviceid || candidate.device_id == event.sourceid);
          });
      if (axis == smooth_axes_.end()) {
        continue;
      }
      if (axis->initialized) {
        const double pixels = (axis->previous - current) * 40.0 / axis->increment;
        if (axis->scroll_type == XIScrollTypeHorizontal) {
          dx += pixels;
        } else if (axis->scroll_type == XIScrollTypeVertical) {
          dy += pixels;
        }
      }
      axis->previous = current;
      axis->initialized = true;
    }
    if (std::abs(dx) < 1e-6 && std::abs(dy) < 1e-6) {
      return;
    }
    last_smooth_time_ = event.time;
    events_.push_back(ShellEvent{
        ShellEventKind::PointerScroll,
        size_,
        "xinput2_smooth_scroll",
        dx,
        dy,
        std::clamp(event.event_x, 0.0, static_cast<double>(size_.width)),
        std::clamp(event.event_y - toolbar_height_, 0.0, static_cast<double>(size_.height)),
        true,
        (event.mods.effective & ShiftMask) != 0U,
    });
  }
#endif

  bool open_{false};
  bool dragging_{false};
  Display* display_{nullptr};
  int screen_{0};
  int depth_{0};
  Visual* visual_{nullptr};
  Window window_{0};
  GC gc_{nullptr};
  XIM input_method_{nullptr};
  XIC input_context_{nullptr};
  Atom wm_delete_{None};
  Atom clipboard_atom_{None};
  Atom utf8_atom_{None};
  Atom targets_atom_{None};
  SizeI size_{};
  SizeI pixel_size_{};
  PointI last_pointer_{};
  std::vector<std::uint32_t> pixels_{};
  std::vector<ShellSaveFormat> save_formats_{};
  std::vector<ShellMenu> menus_{};
  struct ToolbarButton {
    int x{0}, y{0}, w{0}, h{0};
    std::string label{};
    int menu_index{-1};  // -1 is "Save As"
  };
  struct PopupRow {
    std::string label{};
    std::string note{};
    bool checked{false};
    bool heading{false};
    int level{0};
  };
  std::vector<ToolbarButton> toolbar_buttons_{};
  std::vector<PopupRow> popup_rows_{};
  Window popup_window_{0};
  SizeI popup_size_{};
  int popup_hover_{-1};
  int toolbar_height_{0};
  XFontStruct* font_{nullptr};
  std::vector<ShellEvent> events_{};
  std::string clipboard_{};
#if defined(PLUSSTILS_VIEW_HAVE_XINPUT2) && PLUSSTILS_VIEW_HAVE_XINPUT2
  int xi_opcode_{-1};
  Time last_smooth_time_{0};
  std::vector<SmoothScrollAxis> smooth_axes_{};
#endif
};

std::unique_ptr<IWindowShell> make_x11_window_shell() { return std::make_unique<X11WindowShell>(); }

} // namespace pluss::view::window

#endif
