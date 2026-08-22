#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/tils/view/window/canvas.h"

namespace pluss::view::window {

enum class ShellEventKind : std::uint8_t {
  Opened = 0,
  Presented = 1,
  Closed = 2,
  ResizeRequested = 3,
  ShortcutCommand = 4,
  PointerScroll = 5,
  PointerDrag = 6,
  PointerDown = 7,
  TextInput = 8,
  Error = 9,
  PointerUp = 10,
};

enum class ShellBackend : std::uint8_t {
  None = 0,
  Null = 1,
  Cocoa = 2,
  X11 = 3,
  Win32 = 4,
};

enum class ShellSelectionPolicy : std::uint8_t {
  PortableOnly = 0,
  PortablePreferred = 1,
  NativePreferred = 2,
  NativeOnly = 3,
};

struct ShellCapabilities {
  bool supports_live_present{false};
  bool supports_resize_events{false};
  bool supports_shortcut_events{false};
  bool supports_pointer_scroll{false};
  bool supports_pointer_drag{false};
  bool supports_pointer_click{false};
  bool supports_text_input{false};
};

struct ShellEvent {
  ShellEventKind kind{ShellEventKind::Opened};
  SizeI size{};
  std::string note{};
  double delta_x{0.0};
  double delta_y{0.0};
  double x{0.0};
  double y{0.0};
  bool precise_delta{false};
  bool shift_modifier{false};
};

struct ShellSaveFormat {
  std::string label{};
  std::string extension{};
};

struct ShellConfig {
  std::string title{"PlussTilsView"};
  SizeI initial_size{1280, 760};
  std::vector<ShellSaveFormat> save_formats{};
};

class IWindowShell {
 public:
  virtual ~IWindowShell() = default;

  virtual bool open(const ShellConfig& config) = 0;
  virtual bool present(const PixelCanvas& canvas) = 0;
  virtual void close() noexcept = 0;
  [[nodiscard]] virtual std::vector<ShellEvent> poll_events() = 0;
  [[nodiscard]] virtual ShellBackend backend() const noexcept = 0;
  [[nodiscard]] virtual ShellCapabilities capabilities() const noexcept = 0;
  [[nodiscard]] virtual double backing_scale_factor() const noexcept {
    return 1.0;
  }
  [[nodiscard]] virtual bool presentation_frame_due() noexcept {
    return true;
  }
  virtual bool set_clipboard_text(std::string_view text) {
    (void)text;
    return false;
  }
  [[nodiscard]] virtual std::string choose_save_path(std::string_view suggested_name,
                                                     std::string_view extension) {
    (void)suggested_name;
    (void)extension;
    return {};
  }
  virtual bool save_canvas_pdf(const PixelCanvas& canvas, std::string_view path) {
    (void)canvas;
    (void)path;
    return false;
  }
};

class NullWindowShell final : public IWindowShell {
 public:
  bool open(const ShellConfig& config) override;
  bool present(const PixelCanvas& canvas) override;
  void close() noexcept override;
  [[nodiscard]] std::vector<ShellEvent> poll_events() override;
  [[nodiscard]] ShellBackend backend() const noexcept override;
  [[nodiscard]] ShellCapabilities capabilities() const noexcept override;

 private:
  bool open_{false};
  SizeI size_{};
  std::vector<ShellEvent> events_{};
};

[[nodiscard]] std::unique_ptr<IWindowShell> make_null_window_shell();
[[nodiscard]] std::unique_ptr<IWindowShell> make_window_shell(
    ShellSelectionPolicy policy = ShellSelectionPolicy::PortableOnly);
[[nodiscard]] bool is_native_backend(ShellBackend backend) noexcept;

} // namespace pluss::view::window
