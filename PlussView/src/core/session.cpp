#include "pluss/view/session.h"

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <utility>

#include "platform/PlatformHtmlViewer.h"

namespace pluss::view {

namespace {

constexpr auto kNativeWebViewPriority = std::to_array<SessionBackend>({
    SessionBackend::GtkWebKit,
    SessionBackend::MacWebKit,
    SessionBackend::WinWebView2,
});

constexpr auto kAutoBackendPriority = std::to_array<SessionBackend>({
    SessionBackend::GtkWebKit,
    SessionBackend::MacWebKit,
    SessionBackend::WinWebView2,
    SessionBackend::PlussWebView,
});

constexpr auto kPlussWebViewPreferredPriority = std::to_array<SessionBackend>({
    SessionBackend::PlussWebView,
    SessionBackend::GtkWebKit,
    SessionBackend::MacWebKit,
    SessionBackend::WinWebView2,
});

constexpr auto kPlussWebViewOnlyPriority = std::to_array<SessionBackend>({
    SessionBackend::PlussWebView,
});

HostCapabilities capabilities_for_backend_(const SessionBackend backend) noexcept {
  HostCapabilities caps{};
  caps.backend = backend;
  caps.supports_native_window = backend != SessionBackend::None;
  caps.supports_modal_loop = backend != SessionBackend::None;

  switch (backend) {
    case SessionBackend::GtkWebKit:
#if defined(PLUSSVIEW_HAVE_GTK_WEBKIT) && PLUSSVIEW_HAVE_GTK_WEBKIT
      caps.available = detail::gtk_html_viewer_available();
      caps.supports_png_snapshot = true;
      caps.supports_svg_snapshot = true;
#endif
      return caps;
    case SessionBackend::MacWebKit:
#if defined(PLUSSVIEW_HAVE_MAC_WEBKIT) && PLUSSVIEW_HAVE_MAC_WEBKIT
      caps.available = detail::mac_html_viewer_available();
      caps.supports_png_snapshot = true;
#endif
      return caps;
    case SessionBackend::WinWebView2:
#if defined(PLUSSVIEW_HAVE_WIN_WEBVIEW2) && PLUSSVIEW_HAVE_WIN_WEBVIEW2
      caps.available = detail::windows_html_viewer_available();
      caps.supports_png_snapshot = true;
#endif
      return caps;
    case SessionBackend::PlussWebView:
      caps.available = detail::inhouse_html_viewer_available();
      caps.supports_png_snapshot = true;
      return caps;
    case SessionBackend::None:
      return caps;
  }

  return caps;
}

[[nodiscard]] SessionBackend first_available_backend_(const std::span<const SessionBackend> priority) noexcept {
  if (!runtime_gui_environment_available()) {
    return SessionBackend::None;
  }

  for (const auto backend : priority) {
    const auto caps = capabilities_for_backend_(backend);
    if (caps.available) {
      return backend;
    }
  }
  return SessionBackend::None;
}

} // namespace

SessionController::SessionController(HtmlWindowOptions options, SessionEventSink sink)
    : options_(std::move(options)), sink_(std::move(sink)) {
  events_.reserve(16);
}

bool SessionController::open(const std::string& html) {
  if (state_ == SessionState::Active) {
    return true;
  }

  transition_(SessionState::Validating, "begin_open");

  if (html.empty()) {
    transition_(SessionState::Failed, "empty_html_payload");
    return false;
  }

  if (!runtime_gui_environment_available()) {
    transition_(SessionState::Failed, "no_runtime_gui_environment");
    return false;
  }

  transition_(SessionState::SelectingBackend, "select_backend");
  backend_ = select_backend_();
  if (backend_ == SessionBackend::None) {
    transition_(SessionState::Failed, "no_backend_available");
    return false;
  }

  const auto width = std::max<std::int64_t>(640, options_.width);
  const auto height = std::max<std::int64_t>(420, options_.height);
  const auto title = options_.title.empty() ? std::string("PlussView") : options_.title;
  const auto snapshot_name = options_.snapshot_name.empty() ? std::string("view.png") : options_.snapshot_name;
  const auto svg_snapshot_name =
      options_.svg_snapshot_name.empty() ? std::string("view.svg") : options_.svg_snapshot_name;

  transition_(SessionState::Launching, "launch_backend");
  const bool launched =
      launch_on_backend_(backend_, html, title, width, height, snapshot_name, options_.svg_payload, svg_snapshot_name);
  if (!launched) {
    transition_(SessionState::Failed, "launch_failed");
    return false;
  }

  transition_(SessionState::Active, "window_active");
  transition_(SessionState::Closed, "closed_by_host");
  return true;
}

void SessionController::close() noexcept {
  if (state_ == SessionState::Active) {
    transition_(SessionState::Closed, "close_requested");
  }
}

SessionState SessionController::state() const noexcept {
  return state_;
}

SessionBackend SessionController::backend() const noexcept {
  return backend_;
}

const std::vector<SessionEvent>& SessionController::events() const noexcept {
  return events_;
}

void SessionController::transition_(SessionState to, std::string note) {
  SessionEvent event{};
  event.kind = SessionEventKind::Transition;
  event.from = state_;
  event.to = to;
  event.backend = backend_;
  event.note = std::move(note);

  state_ = to;
  events_.push_back(event);
  if (sink_) {
    sink_(events_.back());
  }
}

SessionBackend SessionController::select_backend_() const noexcept {
  switch (options_.backend_preference) {
    case BackendPreference::NativeWebView:
      return first_available_backend_(kNativeWebViewPriority);
    case BackendPreference::PlussWebView:
      return first_available_backend_(kPlussWebViewPreferredPriority);
    case BackendPreference::PlussWebViewOnly:
      return first_available_backend_(kPlussWebViewOnlyPriority);
    case BackendPreference::Auto:
      return first_available_backend_(kAutoBackendPriority);
  }
  return SessionBackend::None;
}

SessionBackend detect_native_backend() noexcept {
  return first_available_backend_(kNativeWebViewPriority);
}

HostCapabilities query_host_capabilities(const SessionBackend backend) noexcept {
  return capabilities_for_backend_(backend);
}

HostCapabilities native_host_capabilities() noexcept {
  const auto backend = detect_native_backend();
  return query_host_capabilities(backend);
}

bool SessionController::launch_on_backend_(SessionBackend backend,
                                           const std::string& html,
                                           const std::string& title,
                                           std::int64_t width,
                                           std::int64_t height,
                                           const std::string& snapshot_name,
                                           const std::string& svg_payload,
                                           const std::string& svg_snapshot_name) const {
  switch (backend) {
    case SessionBackend::GtkWebKit:
#if defined(PLUSSVIEW_HAVE_GTK_WEBKIT) && PLUSSVIEW_HAVE_GTK_WEBKIT
      return detail::show_gtk_html_window(title,
                                          html,
                                          width,
                                          height,
                                          snapshot_name,
                                          svg_payload,
                                          svg_snapshot_name);
#else
      return false;
#endif
    case SessionBackend::MacWebKit:
#if defined(PLUSSVIEW_HAVE_MAC_WEBKIT) && PLUSSVIEW_HAVE_MAC_WEBKIT
      return detail::show_macos_html_window(title, html, width, height, snapshot_name);
#else
      return false;
#endif
    case SessionBackend::WinWebView2:
#if defined(PLUSSVIEW_HAVE_WIN_WEBVIEW2) && PLUSSVIEW_HAVE_WIN_WEBVIEW2
      return detail::show_windows_html_window(title, html, width, height, snapshot_name);
#else
      return false;
#endif
    case SessionBackend::PlussWebView:
      return detail::show_inhouse_html_window(title,
                                              html,
                                              width,
                                              height,
                                              snapshot_name,
                                              svg_payload,
                                              svg_snapshot_name);
    case SessionBackend::None:
      return false;
  }

  return false;
}

SessionController make_session(HtmlWindowOptions options, SessionEventSink sink) {
  return SessionController{std::move(options), std::move(sink)};
}

} // namespace pluss::view
