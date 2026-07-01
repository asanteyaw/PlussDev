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
    window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen_), 0, 0, static_cast<unsigned int>(width),
                                  static_cast<unsigned int>(height), 0, BlackPixel(display_, screen_),
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
    return error ? filename : (directory / filename).string();
  }

 private:
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
    XPutImage(display_, window_, gc_, image, 0, 0, 0, 0, static_cast<unsigned int>(std::max(0, width)),
              static_cast<unsigned int>(std::max(0, height)));
    image->data = nullptr;
    XDestroyImage(image);
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
      std::string note = shortcut;
      if (shortcut == "save_snapshot" && !save_formats_.empty()) {
        note = "save_as:" + save_formats_.front().extension;
      }
      events_.push_back(ShellEvent{ShellEventKind::ShortcutCommand, size_, std::move(note)});
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
    switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          blit();
        }
        break;
      case ConfigureNotify: {
        const SizeI next{
            std::max(1, event.xconfigure.width),
            std::max(1, event.xconfigure.height),
        };
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
          dragging_ = true;
          last_pointer_ = PointI{event.xbutton.x, event.xbutton.y};
          push_pointer_event(ShellEventKind::PointerDown, "x11_pointer_down", event.xbutton.x, event.xbutton.y);
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
              static_cast<double>(event.xbutton.y),
              false,
              (event.xbutton.state & ShiftMask) != 0U,
          });
        }
        break;
      case ButtonRelease:
        if (event.xbutton.button == Button1) {
          dragging_ = false;
          push_pointer_event(ShellEventKind::PointerUp, "x11_pointer_up", event.xbutton.x, event.xbutton.y);
        }
        break;
      case MotionNotify:
        if (dragging_ || (event.xmotion.state & Button1Mask) != 0U) {
          const PointI next{event.xmotion.x, event.xmotion.y};
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
        std::clamp(event.event_y, 0.0, static_cast<double>(size_.height)),
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
