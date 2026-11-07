#if defined(_WIN32)

#include "pluss/tils/view/window/shell.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::view::window {

class Win32WindowShell;

namespace {

constexpr wchar_t kWindowClassName[] = L"PlussViewPixelWindow";
constexpr int kSaveButtonId = 4100;
constexpr int kSaveMenuBaseId = 4200;
constexpr int kToolbarHeight = 46;

LRESULT CALLBACK pluss_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

[[nodiscard]] std::wstring utf8_to_wide(const std::string_view text) {
  if (text.empty()) {
    return {};
  }
  const int count = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  if (count <= 0) {
    return {};
  }
  std::wstring output(static_cast<std::size_t>(count), L'\0');
  (void)MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), output.data(), count);
  return output;
}

[[nodiscard]] std::string wide_to_utf8(const std::wstring_view text) {
  if (text.empty()) {
    return {};
  }
  const int count =
      WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
  if (count <= 0) {
    return {};
  }
  std::string output(static_cast<std::size_t>(count), '\0');
  (void)WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), output.data(), count, nullptr,
                            nullptr);
  return output;
}

[[nodiscard]] std::string shortcut_for_key(const WPARAM key) {
  const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  if (control && (key == VK_OEM_PLUS || key == VK_ADD)) return "zoom_in";
  if (control && (key == VK_OEM_MINUS || key == VK_SUBTRACT)) return "zoom_out";
  if (control && key == '0') return "reset_view";
  if (control && key == 'S') return "save_snapshot";
  if (control && key == 'J') return "next_page";
  if (control && key == 'K') return "previous_page";
  if (key == VK_NEXT) return "page_down";
  if (key == VK_PRIOR) return "page_up";
  if (key == VK_HOME) return "home";
  if (key == VK_END) return "end";
  if (key == VK_LEFT) return "pan_left";
  if (key == VK_RIGHT) return "pan_right";
  if (key == VK_UP) return "pan_up";
  if (key == VK_DOWN) return "pan_down";
  return {};
}

[[nodiscard]] bool register_window_class() {
  static std::once_flag once;
  static bool registered = false;
  std::call_once(once, [] {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = pluss_window_proc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = kWindowClassName;
    registered = RegisterClassExW(&window_class) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
  });
  return registered;
}

class ScopedPerMonitorDpiAwareness {
 public:
  ScopedPerMonitorDpiAwareness() {
    using SetThreadDpiAwarenessContextFn = HANDLE(WINAPI*)(HANDLE);
    setter_ = reinterpret_cast<SetThreadDpiAwarenessContextFn>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
    if (setter_ != nullptr) {
      // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 is the pseudo-handle -4.
      previous_ = setter_(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)));
    }
  }

  ~ScopedPerMonitorDpiAwareness() {
    if (setter_ != nullptr && previous_ != nullptr) {
      (void)setter_(previous_);
    }
  }

 private:
  using Setter = HANDLE(WINAPI*)(HANDLE);
  Setter setter_{nullptr};
  HANDLE previous_{nullptr};
};

} // namespace

class Win32WindowShell final : public IWindowShell {
 public:
  ~Win32WindowShell() override { close(); }

  bool open(const ShellConfig& config) override {
    close();
    if (!register_window_class()) {
      events_.push_back(ShellEvent{ShellEventKind::Error, {}, "win32_class_register_failed"});
      return false;
    }

    save_formats_ = config.save_formats;
    toolbar_height_ = save_formats_.empty() ? 0 : kToolbarHeight;
    const int width = std::max(640, config.initial_size.width);
    const int height = std::max(420, config.initial_size.height);

    ScopedPerMonitorDpiAwareness dpi_awareness;
    RECT frame{0, 0, width, height + toolbar_height_};
    (void)AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
    const int window_width = frame.right - frame.left;
    const int window_height = frame.bottom - frame.top;
    const std::wstring title =
        utf8_to_wide(config.title.empty() ? std::string_view("PlussTilsView") : std::string_view(config.title));

    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kWindowClassName, title.empty() ? L"PlussTilsView" : title.c_str(),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height, nullptr,
                            nullptr, GetModuleHandleW(nullptr), this);
    if (hwnd_ == nullptr) {
      events_.push_back(ShellEvent{ShellEventKind::Error, {}, "win32_window_create_failed"});
      return false;
    }

    if (toolbar_height_ > 0) {
      save_button_ = CreateWindowExW(0, L"BUTTON", L"Save As", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 92, 30,
                                     hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSaveButtonId)),
                                     GetModuleHandleW(nullptr), nullptr);
      if (save_button_ != nullptr) {
        SendMessageW(save_button_, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
      }
    }

    open_ = true;
    size_ = SizeI{width, height};
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    SetForegroundWindow(hwnd_);
    events_.push_back(ShellEvent{ShellEventKind::Opened, size_, "win32_shell_opened"});
    return true;
  }

  bool present(const PixelCanvas& canvas) override {
    if (!open_ || hwnd_ == nullptr || canvas.empty()) {
      events_.push_back(ShellEvent{ShellEventKind::Error, size_, "win32_shell_present_failed"});
      return false;
    }
    const auto canvas_size = canvas.size();
    const auto bytes = canvas.bytes();
    bgra_.resize(bytes.size());
    for (std::size_t index = 0; index + 3U < bytes.size(); index += 4U) {
      bgra_[index] = bytes[index + 2U];
      bgra_[index + 1U] = bytes[index + 1U];
      bgra_[index + 2U] = bytes[index];
      bgra_[index + 3U] = 255U;
    }
    pixel_size_ = canvas_size;
    RECT canvas_rect{};
    GetClientRect(hwnd_, &canvas_rect);
    canvas_rect.top += toolbar_height_;
    InvalidateRect(hwnd_, &canvas_rect, FALSE);
    UpdateWindow(hwnd_);
    events_.push_back(ShellEvent{ShellEventKind::Presented, size_, "win32_shell_presented"});
    return true;
  }

  void close() noexcept override {
    const bool was_open = open_;
    open_ = false;
    dragging_ = false;
    if (hwnd_ != nullptr) {
      DestroyWindow(hwnd_);
      hwnd_ = nullptr;
    }
    save_button_ = nullptr;
    bgra_.clear();
    if (was_open) {
      events_.push_back(ShellEvent{ShellEventKind::Closed, size_, "win32_shell_closed"});
    }
  }

  [[nodiscard]] std::vector<ShellEvent> poll_events() override {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
    auto buffered = std::move(events_);
    events_.clear();
    return buffered;
  }

  [[nodiscard]] ShellBackend backend() const noexcept override { return ShellBackend::Win32; }

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

  [[nodiscard]] double backing_scale_factor() const noexcept override {
    if (hwnd_ == nullptr) {
      return 1.0;
    }
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    static const auto get_dpi_for_window =
        reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    const UINT dpi = get_dpi_for_window != nullptr ? get_dpi_for_window(hwnd_) : 96U;
    return std::clamp(static_cast<double>(dpi) / 96.0, 1.0, 2.0);
  }

  bool set_clipboard_text(const std::string_view text) override {
    if (!OpenClipboard(hwnd_)) {
      return false;
    }
    const std::wstring wide = utf8_to_wide(text);
    const std::size_t bytes = (wide.size() + 1U) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) {
      CloseClipboard();
      return false;
    }
    void* destination = GlobalLock(memory);
    if (destination == nullptr) {
      GlobalFree(memory);
      CloseClipboard();
      return false;
    }
    std::memcpy(destination, wide.c_str(), bytes);
    GlobalUnlock(memory);
    EmptyClipboard();
    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) {
      GlobalFree(memory);
      CloseClipboard();
      return false;
    }
    CloseClipboard();
    return true;
  }

  [[nodiscard]] std::string choose_save_path(const std::string_view suggested_name,
                                             const std::string_view extension) override {
    std::wstring filename = utf8_to_wide(suggested_name.empty() ? std::string_view("plussview") : suggested_name);
    const std::wstring wide_extension = utf8_to_wide(extension);
    if (!wide_extension.empty()) {
      const std::wstring suffix = L"." + wide_extension;
      if (filename.size() < suffix.size() || filename.substr(filename.size() - suffix.size()) != suffix) {
        filename += suffix;
      }
    }

    std::vector<wchar_t> path(32768U, L'\0');
    std::copy_n(filename.data(), std::min(filename.size(), path.size() - 1U), path.data());

    std::wstring description = wide_extension.empty() ? L"Files" : (L"*." + wide_extension + L" files");
    std::wstring pattern = wide_extension.empty() ? L"*.*" : (L"*." + wide_extension);
    std::vector<wchar_t> filter;
    filter.insert(filter.end(), description.begin(), description.end());
    filter.push_back(L'\0');
    filter.insert(filter.end(), pattern.begin(), pattern.end());
    filter.push_back(L'\0');
    const std::wstring all_description = L"All files";
    const std::wstring all_pattern = L"*.*";
    filter.insert(filter.end(), all_description.begin(), all_description.end());
    filter.push_back(L'\0');
    filter.insert(filter.end(), all_pattern.begin(), all_pattern.end());
    filter.push_back(L'\0');
    filter.push_back(L'\0');

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = filter.data();
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrDefExt = wide_extension.empty() ? nullptr : wide_extension.c_str();
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) {
      return {};
    }
    return wide_to_utf8(path.data());
  }

  LRESULT handle_message(HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam) {
    switch (message) {
      case WM_ERASEBKGND: return 1;
      case WM_CLOSE: DestroyWindow(hwnd); return 0;
      case WM_DESTROY:
        if (hwnd_ == hwnd) {
          hwnd_ = nullptr;
        }
        if (open_) {
          open_ = false;
          events_.push_back(ShellEvent{ShellEventKind::Closed, size_, "win32_shell_closed_by_user"});
        }
        return 0;
      case WM_SIZE: layout_children(LOWORD(lparam), HIWORD(lparam)); return 0;
      case WM_DPICHANGED: {
        const auto* suggested = reinterpret_cast<const RECT*>(lparam);
        if (suggested != nullptr) {
          SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                       suggested->bottom - suggested->top, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        return 0;
      }
      case WM_PAINT: paint(hwnd); return 0;
      case WM_COMMAND:
        if (LOWORD(wparam) == kSaveButtonId) {
          show_save_menu();
          return 0;
        }
        if (LOWORD(wparam) >= kSaveMenuBaseId &&
            LOWORD(wparam) < kSaveMenuBaseId + static_cast<int>(save_formats_.size())) {
          const auto index = static_cast<std::size_t>(LOWORD(wparam) - kSaveMenuBaseId);
          events_.push_back(ShellEvent{
              ShellEventKind::ShortcutCommand,
              size_,
              "save_as:" + save_formats_[index].extension,
          });
          return 0;
        }
        break;
      case WM_KEYDOWN: {
        const std::string shortcut = shortcut_for_key(wparam);
        if (!shortcut.empty()) {
          if (shortcut == "save_snapshot" && !save_formats_.empty()) {
            show_save_menu();
          } else {
            events_.push_back(ShellEvent{ShellEventKind::ShortcutCommand, size_, shortcut});
          }
          return 0;
        }
        break;
      }
      case WM_CHAR: handle_character(static_cast<wchar_t>(wparam)); return 0;
      case WM_LBUTTONDOWN: {
        const PointI point = canvas_point(lparam);
        if (point.y >= 0) {
          dragging_ = true;
          last_pointer_ = point;
          SetCapture(hwnd);
          push_pointer(ShellEventKind::PointerDown, "win32_pointer_down", point);
        }
        return 0;
      }
      case WM_LBUTTONUP: {
        const PointI point = canvas_point(lparam);
        if (dragging_) {
          dragging_ = false;
          ReleaseCapture();
          push_pointer(ShellEventKind::PointerUp, "win32_pointer_up", point);
        }
        return 0;
      }
      case WM_MOUSEMOVE:
        if (dragging_ && (wparam & MK_LBUTTON) != 0U) {
          const PointI point = canvas_point(lparam);
          events_.push_back(ShellEvent{
              ShellEventKind::PointerDrag,
              size_,
              "win32_drag",
              static_cast<double>(point.x - last_pointer_.x),
              static_cast<double>(point.y - last_pointer_.y),
              static_cast<double>(point.x),
              static_cast<double>(point.y),
          });
          last_pointer_ = point;
        }
        return 0;
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL: handle_wheel(message, wparam, lparam); return 0;
      default: break;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
  }

 private:
  void layout_children(const int client_width, const int client_height) {
    if (save_button_ != nullptr) {
      MoveWindow(save_button_, std::max(12, client_width - 104), 8, 92, 30, TRUE);
    }
    const SizeI next{
        std::max(1, client_width),
        std::max(1, client_height - toolbar_height_),
    };
    if (open_ && (next.width != size_.width || next.height != size_.height)) {
      size_ = next;
      events_.push_back(ShellEvent{ShellEventKind::ResizeRequested, size_, "win32_shell_resized"});
    } else {
      size_ = next;
    }
  }

  void paint(HWND hwnd) {
    PAINTSTRUCT paint_struct{};
    HDC dc = BeginPaint(hwnd, &paint_struct);
    if (dc == nullptr) {
      return;
    }
    RECT client{};
    GetClientRect(hwnd, &client);
    RECT canvas_rect = client;
    canvas_rect.top += toolbar_height_;
    FillRect(dc, &canvas_rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
    if (!bgra_.empty() && pixel_size_.width > 0 && pixel_size_.height > 0) {
      BITMAPINFO bitmap{};
      bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bitmap.bmiHeader.biWidth = pixel_size_.width;
      bitmap.bmiHeader.biHeight = -pixel_size_.height;
      bitmap.bmiHeader.biPlanes = 1;
      bitmap.bmiHeader.biBitCount = 32;
      bitmap.bmiHeader.biCompression = BI_RGB;
      SetStretchBltMode(dc, COLORONCOLOR);
      StretchDIBits(dc, canvas_rect.left, canvas_rect.top, canvas_rect.right - canvas_rect.left,
                    canvas_rect.bottom - canvas_rect.top, 0, 0, pixel_size_.width, pixel_size_.height, bgra_.data(),
                    &bitmap, DIB_RGB_COLORS, SRCCOPY);
    }
    EndPaint(hwnd, &paint_struct);
  }

  [[nodiscard]] PointI canvas_point(const LPARAM lparam) const {
    return PointI{
        std::clamp(GET_X_LPARAM(lparam), 0, size_.width),
        std::clamp(GET_Y_LPARAM(lparam) - toolbar_height_, -1, size_.height),
    };
  }

  void push_pointer(const ShellEventKind kind, const char* note, const PointI point) {
    events_.push_back(ShellEvent{
        kind,
        size_,
        note,
        0.0,
        0.0,
        std::clamp(static_cast<double>(point.x), 0.0, static_cast<double>(size_.width)),
        std::clamp(static_cast<double>(point.y), 0.0, static_cast<double>(size_.height)),
    });
  }

  void handle_wheel(const UINT message, const WPARAM wparam, const LPARAM lparam) {
    POINT point{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    ScreenToClient(hwnd_, &point);
    point.y -= toolbar_height_;
    if (point.y < 0 || point.y > size_.height) {
      return;
    }
    const int raw_delta = GET_WHEEL_DELTA_WPARAM(wparam);
    const bool precise = std::abs(raw_delta) < WHEEL_DELTA;
    const double delta = precise
                             ? static_cast<double>(raw_delta) / 3.0
                             : static_cast<double>(raw_delta) / static_cast<double>(WHEEL_DELTA);
    const bool horizontal = message == WM_MOUSEHWHEEL;
    events_.push_back(ShellEvent{
        ShellEventKind::PointerScroll,
        size_,
        horizontal ? "win32_horizontal_scroll" : "win32_vertical_scroll",
        horizontal ? delta : 0.0,
        horizontal ? 0.0 : delta,
        std::clamp(static_cast<double>(point.x), 0.0, static_cast<double>(size_.width)),
        std::clamp(static_cast<double>(point.y), 0.0, static_cast<double>(size_.height)),
        precise,
        (GET_KEYSTATE_WPARAM(wparam) & MK_SHIFT) != 0U,
    });
  }

  void handle_character(const wchar_t character) {
    if (character == L'\b' || character == 0x7F) {
      events_.push_back(ShellEvent{ShellEventKind::TextInput, size_, "__backspace__"});
      return;
    }
    if (character < 0x20 || character == L'\t' || character == L'\r' || character == L'\n') {
      return;
    }
    if (character >= 0xD800 && character <= 0xDBFF) {
      pending_high_surrogate_ = character;
      return;
    }
    std::wstring text;
    if (pending_high_surrogate_ != 0 && character >= 0xDC00 && character <= 0xDFFF) {
      text.push_back(pending_high_surrogate_);
      text.push_back(character);
    } else {
      text.push_back(character);
    }
    pending_high_surrogate_ = 0;
    const std::string utf8 = wide_to_utf8(text);
    if (!utf8.empty()) {
      events_.push_back(ShellEvent{ShellEventKind::TextInput, size_, utf8});
    }
  }

  void show_save_menu() {
    if (hwnd_ == nullptr || save_formats_.empty()) {
      return;
    }
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
      return;
    }
    for (std::size_t index = 0; index < save_formats_.size(); ++index) {
      const std::wstring label = utf8_to_wide(save_formats_[index].label);
      AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(kSaveMenuBaseId + static_cast<int>(index)),
                  label.empty() ? L"Save" : label.c_str());
    }
    RECT button_rect{};
    GetWindowRect(save_button_, &button_rect);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, button_rect.left, button_rect.bottom, 0, hwnd_, nullptr);
    DestroyMenu(menu);
  }

  bool open_{false};
  bool dragging_{false};
  HWND hwnd_{nullptr};
  HWND save_button_{nullptr};
  int toolbar_height_{0};
  SizeI size_{};
  SizeI pixel_size_{};
  PointI last_pointer_{};
  wchar_t pending_high_surrogate_{0};
  std::vector<std::uint8_t> bgra_{};
  std::vector<ShellSaveFormat> save_formats_{};
  std::vector<ShellEvent> events_{};
};

namespace {

LRESULT CALLBACK pluss_window_proc(HWND hwnd, const UINT message, const WPARAM wparam, const LPARAM lparam) {
  Win32WindowShell* shell = nullptr;
  if (message == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
    shell = static_cast<Win32WindowShell*>(create->lpCreateParams);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(shell));
  } else {
    shell = reinterpret_cast<Win32WindowShell*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  }
  if (shell != nullptr) {
    return shell->handle_message(hwnd, message, wparam, lparam);
  }
  return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

std::unique_ptr<IWindowShell> make_win32_window_shell() { return std::make_unique<Win32WindowShell>(); }

} // namespace pluss::view::window

#endif
