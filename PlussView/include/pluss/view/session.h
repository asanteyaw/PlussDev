#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "pluss/view/html_window.h"

namespace pluss::view {

enum class SessionState : std::uint8_t {
  Idle = 0,
  Validating = 1,
  SelectingBackend = 2,
  Launching = 3,
  Active = 4,
  Closed = 5,
  Failed = 6,
};

enum class SessionBackend : std::uint8_t {
  None = 0,
  GtkWebKit = 1,
  MacWebKit = 2,
  WinWebView2 = 3,
  PlussWebView = 4,
};

enum class SessionEventKind : std::uint8_t {
  Transition = 0,
};

struct HostCapabilities {
  SessionBackend backend{SessionBackend::None};
  bool available{false};
  bool supports_native_window{false};
  bool supports_png_snapshot{false};
  bool supports_svg_snapshot{false};
  bool supports_modal_loop{false};
};

struct SessionEvent {
  SessionEventKind kind{SessionEventKind::Transition};
  SessionState from{SessionState::Idle};
  SessionState to{SessionState::Idle};
  SessionBackend backend{SessionBackend::None};
  std::string note{};
};

using SessionEventSink = std::function<void(const SessionEvent&)>;

class SessionController {
 public:
  explicit SessionController(HtmlWindowOptions options = {}, SessionEventSink sink = {});

  [[nodiscard]] bool open(const std::string& html);
  void close() noexcept;

  [[nodiscard]] SessionState state() const noexcept;
  [[nodiscard]] SessionBackend backend() const noexcept;
  [[nodiscard]] const std::vector<SessionEvent>& events() const noexcept;

 private:
  void transition_(SessionState to, std::string note = {});
  [[nodiscard]] SessionBackend select_backend_() const noexcept;
  [[nodiscard]] bool launch_on_backend_(SessionBackend backend,
                                        const std::string& html,
                                        const std::string& title,
                                        std::int64_t width,
                                        std::int64_t height,
                                        const std::string& snapshot_name,
                                        const std::string& svg_payload,
                                        const std::string& svg_snapshot_name) const;

  HtmlWindowOptions options_{};
  SessionEventSink sink_{};
  SessionState state_{SessionState::Idle};
  SessionBackend backend_{SessionBackend::None};
  std::vector<SessionEvent> events_{};
};

[[nodiscard]] SessionController make_session(HtmlWindowOptions options = {}, SessionEventSink sink = {});
[[nodiscard]] SessionBackend detect_native_backend() noexcept;
[[nodiscard]] HostCapabilities query_host_capabilities(SessionBackend backend) noexcept;
[[nodiscard]] HostCapabilities native_host_capabilities() noexcept;

} // namespace pluss::view
