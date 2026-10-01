#pragma once

// Phase 2 of PlussBI: a genuinely live dashboard, served over HTTP so filter
// controls re-query and redraw in the browser via `POST /api/filter`
// instead of the static snapshot `Studio::show()`/`export_html()` produce.
//
// This header is deliberately NOT included by `pluss/bi/bi.h` (and so not by
// `pluss/tils.h`), because it pulls in the vendored cpp-httplib, which most
// consumers of `pluss::bi::Studio` never need. Only code that actually calls
// `Studio::start_serving()`/`serve()`, or names `ServeHandle`, should
// include this header directly.

#include "pluss/bi/studio.h"

#include "BI_tools/json_lite.h"

#include <httplib.h>

#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

// cpp-httplib is vendored third-party code (third_party/httplib, MIT
// license, yhirose/cpp-httplib) used as-is, under its own upstream
// `::httplib` namespace -- not `pluss::`. It isn't PlussBI's own work, so
// it isn't relabeled as if it were; every `httplib::...` reference below
// is that unmodified third-party namespace.

namespace pluss::bi::detail {

// A 1 MiB cap on `/api/filter` request bodies. The wire format is a flat
// JSON object of a handful of short string fields (control id, filter
// kind, one or two bound values) -- there is no legitimate reason for a
// real client to send anything close to this, so it exists purely to stop
// a hostile or buggy client from making the embedded server allocate an
// unbounded amount of memory per request.
inline constexpr std::size_t kMaxFilterPayloadBytes = 1 << 20;

// Everything a running server's request handlers need, heap-allocated and
// shared (via `shared_ptr`) between the handlers themselves and the
// background thread that runs `httplib::Server::listen_after_bind()`.
// Request handlers are stored inside the `httplib::Server` as
// `std::function`s and capture this by value (a `shared_ptr` copy, never a
// reference to a stack local) -- `start_serving()` returns before the
// server stops, so nothing here may live on `start_serving()`'s own stack
// frame.
struct ServeContext {
  Studio* studio{};
  const pluss::bundle::Nexus* nexus{};
  std::string page_title{};
  std::int64_t width{};
  std::int64_t height{};
  DashboardChrome chrome{DashboardChrome::Sidebar};
  bool show_all{};
  std::mutex mutex{};
};

// Finds the `Control` named by a live `/api/filter` request across every
// board/tile in `studio`, matching by `Control::key()`. Returns nullopt when
// no tile carries a control with that key (a stale or forged request).
[[nodiscard]] inline std::optional<Control> find_control_(
    const Studio& studio,
    std::string_view control_key) {
  for (const auto& board : studio.boards()) {
    for (const auto& tile : board.tiles()) {
      if (tile.control().has_value() && tile.control()->key() == control_key) {
        return tile.control();
      }
    }
  }
  return std::nullopt;
}

// Applies one decoded `/api/filter` request body to `studio`'s session,
// dispatching on the control's `ControlKind` the same way the free
// `pluss::bi::apply`/`apply_time`/`clear` helpers do -- except through
// `Studio::set_filter`/`clear_filter` so the studio's render caches are
// invalidated correctly. Malformed or incomplete payloads (an empty
// category value, a range with one bound left blank) are treated as "clear
// this filter" rather than an error, matching what a user clearing a form
// field intends. Anything that can't be parsed as a valid filter (an
// unparseable numeric or temporal bound) throws `pluss::Error`, caught by
// the caller and reported as a 400 -- never left to crash the server.
inline void apply_filter_payload_(
    Studio& studio,
    const Control& control,
    const std::map<std::string, std::string>& payload) {
  const auto field = [&](const char* name) -> std::string {
    const auto it = payload.find(name);
    return it != payload.end() ? it->second : std::string();
  };
  const std::string kind = field("kind");

  if (kind == "clear") {
    studio.clear_filter(control.key());
    return;
  }

  if (kind == "category") {
    const auto value = field("value");
    if (value.empty()) {
      studio.clear_filter(control.key());
      return;
    }
    PLUSS_CHECK(control.kind() == ControlKind::Category,
                "pluss::bi::Studio::serve: control is not categorical");
    auto filter = Filter::eq(control.feature(), value);
    if (control.scope().has_value()) {
      filter = filter.scoped(*control.scope());
    }
    filter.key = control.key();
    studio.set_filter(std::move(filter));
    return;
  }

  if (kind == "range") {
    const auto low = field("low");
    const auto high = field("high");
    if (low.empty() || high.empty()) {
      studio.clear_filter(control.key());
      return;
    }

    if (control.kind() == ControlKind::TemporalRange) {
      std::optional<pluss::Timeline> low_time;
      std::optional<pluss::Timeline> high_time;
      try {
        low_time = pluss::make_timeline(std::vector<std::string>{low});
        high_time = pluss::make_timeline(std::vector<std::string>{high});
      } catch (const std::exception&) {
        PLUSS_FAIL("pluss::bi::Studio::serve: invalid temporal bound in filter request");
      }
      auto filter = Filter::between_time(control.feature(), *low_time, *high_time);
      if (control.scope().has_value()) {
        filter = filter.scoped(*control.scope());
      }
      filter.key = control.key();
      studio.set_filter(std::move(filter));
      return;
    }

    PLUSS_CHECK(control.kind() == ControlKind::NumericRange,
                "pluss::bi::Studio::serve: control is not a range control");
    const auto low_value = parse_double_(low, "pluss::bi::Studio::serve");
    const auto high_value = parse_double_(high, "pluss::bi::Studio::serve");
    auto filter = Filter::between(control.feature(), low_value, high_value);
    if (control.scope().has_value()) {
      filter = filter.scoped(*control.scope());
    }
    filter.key = control.key();
    studio.set_filter(std::move(filter));
    return;
  }

  PLUSS_FAIL("pluss::bi::Studio::serve: unknown filter kind '" + kind + "'");
}

} // namespace pluss::bi::detail

namespace pluss::bi {

// A running `Studio::start_serving()` server. Move-only (like
// `std::thread`): copying would leave two handles racing to stop and join
// the same server. Destroying a handle that's still running stops and
// joins it automatically, so a `ServeHandle` going out of scope never
// leaks a background thread or a bound socket.
class ServeHandle {
public:
  ServeHandle() = default;

  ServeHandle(const ServeHandle&) = delete;
  ServeHandle& operator=(const ServeHandle&) = delete;

  ServeHandle(ServeHandle&&) noexcept = default;

  ServeHandle& operator=(ServeHandle&& other) noexcept {
    if (this != &other) {
      stop_and_join_();
      server_ = std::move(other.server_);
      thread_ = std::move(other.thread_);
      port_ = other.port_;
      url_ = std::move(other.url_);
      other.port_ = 0;
    }
    return *this;
  }

  ~ServeHandle() {
    stop_and_join_();
  }

  // Signals the server to stop accepting new connections and finish up.
  // Safe to call from any thread, including while another thread is
  // blocked in `wait()`. A no-op if the server was already stopped or this
  // handle never held a running server (e.g. after being moved from).
  void stop() {
    if (server_) {
      server_->stop();
    }
  }

  // Blocks the calling thread until the server stops (via `stop()`, or
  // externally). Returns immediately if it already has.
  void wait() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  [[nodiscard]] bool running() const {
    return server_ != nullptr && server_->is_running();
  }

  [[nodiscard]] int port() const noexcept {
    return port_;
  }

  [[nodiscard]] const std::string& url() const noexcept {
    return url_;
  }

private:
  friend ServeHandle Studio::start_serving(
      std::string_view, std::int64_t, std::int64_t, std::int64_t, bool, DashboardChrome);

  std::shared_ptr<httplib::Server> server_{};
  std::thread thread_{};
  int port_{0};
  std::string url_{};

  void stop_and_join_() {
    stop();
    wait();
  }
};

inline ServeHandle Studio::start_serving(
    std::string_view title,
    std::int64_t width,
    std::int64_t height,
    std::int64_t port,
    bool open_browser,
    DashboardChrome chrome) {
  PLUSS_CHECK(width > 0 && height > 0,
              "pluss::bi::Studio::serve: width and height must be positive");
  PLUSS_CHECK(nexus_ != nullptr,
              "pluss::bi::Studio::serve: source Nexus has not been set");

  auto ctx = std::make_shared<detail::ServeContext>();
  ctx->studio = this;
  ctx->nexus = nexus_;
  ctx->page_title = title.empty() ? key_ : std::string(title);
  ctx->width = width;
  ctx->height = height;
  ctx->chrome = chrome;
  ctx->show_all = (chrome == DashboardChrome::None);

  auto svr = std::make_shared<httplib::Server>();
  svr->set_payload_max_length(detail::kMaxFilterPayloadBytes);

  // Serve the CSS/JS as genuinely separate, cacheable static assets over
  // HTTP -- the modular counterpart to `show()`/`export_html()` inlining
  // the same byte-identical content into a single portable file.
  svr->Get("/assets/dashboard.css", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(pluss::bi::assets::kDashboardCss, "text/css; charset=utf-8");
  });
  svr->Get("/assets/dashboard.js", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(pluss::bi::assets::kDashboardJs, "application/javascript; charset=utf-8");
  });

  svr->Get("/", [ctx](const httplib::Request&, httplib::Response& res) {
    std::string html;
    {
      const std::lock_guard<std::mutex> lock(ctx->mutex);
      const auto rendered = ctx->studio->render();
      PLUSS_CHECK(rendered.native != nullptr,
                  "pluss::bi::Studio::serve: native board studio was not created");
      html = detail::dashboard_html_(
          rendered,
          *ctx->nexus,
          ctx->page_title,
          ctx->width,
          ctx->height,
          rendered.native->to_svg(),
          /*live=*/true,
          ctx->chrome,
          /*inline_assets=*/false);
    }
    res.set_content(html, "text/html; charset=utf-8");
  });

  svr->Post("/api/filter", [ctx](const httplib::Request& req, httplib::Response& res) {
    try {
      const auto payload = pluss::bi_tools::parse_flat_json_object(req.body);
      const auto control_it = payload.find("control");
      PLUSS_CHECK(control_it != payload.end(),
                  "pluss::bi::Studio::serve: /api/filter request is missing 'control'");

      std::string content_html;
      std::string chips_html;
      {
        const std::lock_guard<std::mutex> lock(ctx->mutex);
        const auto control = detail::find_control_(*ctx->studio, control_it->second);
        PLUSS_CHECK(control.has_value(),
                    "pluss::bi::Studio::serve: unknown control id '" + control_it->second + "'");
        detail::apply_filter_payload_(*ctx->studio, *control, payload);

        const auto rendered = ctx->studio->render();
        PLUSS_CHECK(rendered.native != nullptr,
                    "pluss::bi::Studio::serve: native board studio was not created");
        content_html = detail::render_main_content_html_(rendered, *ctx->nexus, ctx->show_all);
        chips_html = detail::render_session_chips_html_(rendered.session);
      }

      res.set_content(
          pluss::bi_tools::json_object({
              {"content_html", content_html},
              {"chips_html", chips_html},
          }),
          "application/json");
    } catch (const std::exception& error) {
      res.status = 400;
      res.set_content(
          pluss::bi_tools::json_object({{"error", error.what()}}),
          "application/json");
    }
  });

  int bound_port = static_cast<int>(port);
  if (port > 0) {
    PLUSS_CHECK(svr->bind_to_port("127.0.0.1", bound_port),
                "pluss::bi::Studio::serve: failed to bind to 127.0.0.1:" +
                    std::to_string(port));
  } else {
    bound_port = svr->bind_to_any_port("127.0.0.1");
    PLUSS_CHECK(bound_port > 0,
                "pluss::bi::Studio::serve: failed to bind to 127.0.0.1 on an ephemeral port");
  }

  ServeHandle handle;
  handle.port_ = bound_port;
  handle.url_ = "http://127.0.0.1:" + std::to_string(bound_port) + "/";
  handle.server_ = svr;

  std::cout << "pluss::bi::Studio::serve: " << ctx->page_title << " is live at " << handle.url_
            << " (press Ctrl+C to stop)" << std::endl;

  if (open_browser && !pluss::bi_tools::open_path_in_browser(handle.url_)) {
    std::cout << "pluss::bi::Studio::serve: could not auto-open a browser; "
                 "open the URL above manually\n";
  }

  // `svr` (a shared_ptr copy) is captured by value so the Server -- and
  // therefore every handler above that captured `ctx` -- stays alive for
  // as long as this thread runs, independent of `start_serving()`'s own
  // stack frame, which is gone by the time the caller gets `handle` back.
  handle.thread_ = std::thread([svr]() { svr->listen_after_bind(); });
  // `listen_after_bind()` only flips `is_running()` true once its accept
  // loop actually starts, which happens asynchronously on the thread just
  // spawned above -- without waiting here, a caller that immediately
  // checks `handle.running()` (or fires a request) races the server's own
  // startup.
  svr->wait_until_ready();
  return handle;
}

inline void Studio::serve(
    std::string_view title,
    std::int64_t width,
    std::int64_t height,
    std::int64_t port,
    bool open_browser,
    DashboardChrome chrome) {
  auto handle = start_serving(title, width, height, port, open_browser, chrome);
  handle.wait();
}

} // namespace pluss::bi
