// Phase 3 hardening: automated coverage for pluss::bi::Studio::serve() /
// start_serving(), which previously had zero automated tests -- only
// manual verification in a browser. Exercises the embedded HTTP server
// end to end over real sockets: GET /, the /assets/*.css|.js routes,
// POST /api/filter (valid and adversarial payloads), concurrent requests,
// and the ServeHandle's stop()/wait() lifecycle.

#include <atomic>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

#include "pluss/bi/serve.h"

namespace {

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

void expect_contains(std::string_view haystack, std::string_view needle, std::string_view message) {
  expect(haystack.find(needle) != std::string_view::npos, message);
}

pluss::bundle::Nexus make_serve_test_nexus() {
  using pluss::table::DataFrame;

  auto loans = DataFrame({
      {"loan_id", pluss::make_series(std::vector<double>{10.0, 11.0, 12.0, 13.0})},
      {"principal", pluss::make_series(std::vector<double>{1000.0, 1200.0, 900.0, 1500.0})},
      {"segment", pluss::make_note({"retail", "retail", "enterprise", "retail"})},
      {"opened_at", pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
      })},
  }).title("loans");

  return pluss::bundle::Nexus({{"loans", loans}});
}

// One board carrying all three control kinds (category/range/temporal) plus
// one KPI view, so every `apply_filter_payload_` branch is reachable
// through a real HTTP request in these tests.
pluss::bi::Studio make_serve_test_studio(const pluss::bundle::Nexus& nexus) {
  using namespace pluss::bi;

  Studio studio("serve_test");
  studio.source(nexus);

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment").place(0, 0);

  auto principal = range("principal_filter");
  principal.feature("principal").label("Principal").place(0, 1);

  auto opened = time_range("opened_filter");
  opened.feature("opened_at").label("Opened").place(0, 2);

  auto count_kpi = kpi("loan_count");
  count_kpi
      .query(from("loans"))
      .feature("loan_id")
      .op(pluss::table::GroupOp::Count)
      .title("Loans")
      .place(1, 0);

  auto& board = studio.board("overview");
  board.grid(2, 3);
  board.control(segment);
  board.control(principal);
  board.control(opened);
  board.view(count_kpi);

  return studio;
}

// httplib::Client is the vendored third-party HTTP client used by the server
// library's own client, pulled in transitively by pluss/bi/serve.h.
httplib::Client make_client(const pluss::bi::ServeHandle& handle) {
  httplib::Client client("127.0.0.1", handle.port());
  client.set_connection_timeout(5, 0);
  client.set_read_timeout(5, 0);
  return client;
}

void test_serve_root_and_assets() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);

  auto handle = studio.start_serving("Serve Test", 1280, 820, /*port=*/0, /*open_browser=*/false);
  expect(handle.running(), "start_serving should return a running handle");
  expect(handle.port() > 0, "start_serving should report a bound port");

  auto client = make_client(handle);

  const auto root = client.Get("/");
  expect(static_cast<bool>(root), "GET / should succeed");
  expect(root->status == 200, "GET / should return 200");
  expect_contains(root->body, "Serve Test", "GET / should contain the page title");
  expect_contains(root->body, "Live", "a live server's GET / should render the 'Live' status");
  expect_contains(root->body, "/assets/dashboard.css",
                   "serve()'s GET / should reference the external stylesheet, not inline it");
  expect_contains(root->body, "/assets/dashboard.js",
                   "serve()'s GET / should reference the external script, not inline it");

  const auto css = client.Get("/assets/dashboard.css");
  expect(static_cast<bool>(css), "GET /assets/dashboard.css should succeed");
  expect(css->status == 200, "GET /assets/dashboard.css should return 200");
  expect(css->body == pluss::bi::assets::kDashboardCss,
         "served CSS should byte-match the embedded constant");

  const auto js = client.Get("/assets/dashboard.js");
  expect(static_cast<bool>(js), "GET /assets/dashboard.js should succeed");
  expect(js->status == 200, "GET /assets/dashboard.js should return 200");
  expect(js->body == pluss::bi::assets::kDashboardJs,
         "served JS should byte-match the embedded constant");

  handle.stop();
}

void test_serve_flat_chrome_shows_all_boards() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);

  auto handle = studio.start_serving(
      "Flat", 1280, 820, /*port=*/0, /*open_browser=*/false, pluss::bi::DashboardChrome::None);
  auto client = make_client(handle);

  const auto root = client.Get("/");
  expect(root->status == 200, "GET / should return 200 in flat chrome mode");
  expect(root->body.find("bi-sidebar") == std::string::npos,
         "chrome=None should never render the sidebar chrome");
  expect_contains(root->body, "bi-shell", "chrome=None should render the flat single-column shell");

  handle.stop();
}

void test_serve_category_filter_round_trips() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Filter", 1280, 820, 0, false);
  auto client = make_client(handle);

  const auto resp = client.Post(
      "/api/filter",
      R"({"control":"segment_filter","kind":"category","value":"retail"})",
      "application/json");
  expect(static_cast<bool>(resp), "POST /api/filter should succeed");
  expect(resp->status == 200, "a valid category filter should return 200");
  expect_contains(resp->body, "content_html", "response JSON should carry content_html");
  expect_contains(resp->body, "retail", "the response should reflect the applied filter value");

  // The mutation persists on the Studio itself -- a fresh GET / should
  // reflect it too, proving this isn't just an echoed request value.
  const auto root = client.Get("/");
  expect_contains(root->body, "retail", "a persisted filter should show up on the next GET /");

  const auto cleared = client.Post(
      "/api/filter", R"({"control":"segment_filter","kind":"clear"})", "application/json");
  expect(cleared->status == 200, "clearing a filter should return 200");
  expect_contains(cleared->body, "No active filters",
                   "clearing the only active filter should restore the empty-state chip");

  handle.stop();
}

void test_serve_range_and_temporal_filters() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Filter", 1280, 820, 0, false);
  auto client = make_client(handle);

  const auto range_resp = client.Post(
      "/api/filter",
      R"({"control":"principal_filter","kind":"range","low":"950","high":"1300"})",
      "application/json");
  expect(range_resp->status == 200, "a valid numeric-range filter should return 200");
  expect_contains(range_resp->body, "950", "the response should reflect the applied low bound");

  const auto temporal_resp = client.Post(
      "/api/filter",
      R"({"control":"opened_filter","kind":"range","low":"2026-01-02T00:00:00Z","high":"2026-01-03T00:00:00Z"})",
      "application/json");
  expect(temporal_resp->status == 200, "a valid temporal-range filter should return 200");

  handle.stop();
}

void test_serve_rejects_adversarial_payloads() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Adversarial", 1280, 820, 0, false);
  auto client = make_client(handle);

  const auto missing_control = client.Post("/api/filter", R"({"kind":"category"})", "application/json");
  expect(static_cast<bool>(missing_control), "a request missing 'control' should still get an HTTP response");
  expect(missing_control->status == 400, "a request missing 'control' should return 400, not crash");

  const auto unknown_control = client.Post(
      "/api/filter", R"({"control":"does_not_exist","kind":"clear"})", "application/json");
  expect(unknown_control->status == 400, "an unknown control id should return 400, not crash");

  const auto wrong_kind = client.Post(
      "/api/filter",
      R"({"control":"segment_filter","kind":"range","low":"1","high":"2"})",
      "application/json");
  expect(wrong_kind->status == 400,
         "a 'range' request against a categorical control should return 400, not crash");

  const auto malformed_json = client.Post("/api/filter", "not json at all {{{", "application/json");
  expect(static_cast<bool>(malformed_json), "malformed JSON should still get an HTTP response");
  expect(malformed_json->status == 400, "malformed JSON should return 400, not crash the server");

  const auto garbage_temporal = client.Post(
      "/api/filter",
      R"({"control":"opened_filter","kind":"range","low":"not-a-date","high":"also-not-a-date"})",
      "application/json");
  expect(static_cast<bool>(garbage_temporal),
         "an unparseable temporal bound should still get an HTTP response, not crash the server");
  expect(garbage_temporal->status == 400, "an unparseable temporal bound should return 400");

  const auto garbage_numeric = client.Post(
      "/api/filter",
      R"({"control":"principal_filter","kind":"range","low":"abc","high":"def"})",
      "application/json");
  expect(garbage_numeric->status == 400, "an unparseable numeric bound should return 400, not crash");

  // The server must still be healthy after all of the above.
  const auto still_alive = client.Get("/");
  expect(still_alive->status == 200, "the server should still answer requests after adversarial input");

  handle.stop();
}

void test_serve_rejects_oversized_payload() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Oversized", 1280, 820, 0, false);
  auto client = make_client(handle);

  // Comfortably over the 1 MiB cap (pluss::bi::detail::kMaxFilterPayloadBytes).
  const std::string huge_body(2 << 20, 'x');
  const auto resp = client.Post("/api/filter", huge_body, "application/json");
  expect(static_cast<bool>(resp), "an oversized request should still get an HTTP response, not a dropped connection");
  expect(resp->status == 413, "an oversized request body should be rejected with 413 Payload Too Large");

  const auto still_alive = client.Get("/");
  expect(still_alive->status == 200, "the server should still answer requests after an oversized request");

  handle.stop();
}

void test_serve_concurrent_requests_smoke() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Concurrent", 1280, 820, 0, false);

  constexpr int kRequestCount = 12;
  std::atomic<int> success_count{0};
  std::vector<std::thread> workers;
  workers.reserve(kRequestCount);

  for (int i = 0; i < kRequestCount; ++i) {
    workers.emplace_back([&handle, &success_count, i]() {
      httplib::Client client("127.0.0.1", handle.port());
      client.set_connection_timeout(5, 0);
      client.set_read_timeout(5, 0);
      const std::string value = (i % 2 == 0) ? "retail" : "enterprise";
      const auto resp = client.Post(
          "/api/filter",
          R"({"control":"segment_filter","kind":"category","value":")" + value + R"("})",
          "application/json");
      if (resp && resp->status == 200) {
        ++success_count;
      }
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }

  expect(success_count.load() == kRequestCount,
         "every concurrent request should complete successfully with no crash or deadlock");

  handle.stop();
}

void test_serve_handle_stop_actually_stops() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  auto handle = studio.start_serving("Stoppable", 1280, 820, 0, false);
  const int port = handle.port();

  {
    auto client = make_client(handle);
    const auto resp = client.Get("/");
    expect(resp->status == 200, "the server should answer while running");
  }

  handle.stop();
  handle.wait();
  expect(!handle.running(), "running() should report false after stop()+wait()");

  httplib::Client after_stop("127.0.0.1", port);
  after_stop.set_connection_timeout(1, 0);
  const auto resp = after_stop.Get("/");
  expect(!resp, "a request after stop() should fail to connect -- the server must actually be gone");
}

void test_serve_handle_destructor_stops_server() {
  const auto nexus = make_serve_test_nexus();
  auto studio = make_serve_test_studio(nexus);
  int port = 0;
  {
    auto handle = studio.start_serving("Scoped", 1280, 820, 0, false);
    port = handle.port();
    auto client = make_client(handle);
    const auto resp = client.Get("/");
    expect(resp->status == 200, "the server should answer while the handle is in scope");
  } // handle destructs here -- must stop+join, not leak a thread/socket.

  httplib::Client after_scope("127.0.0.1", port);
  after_scope.set_connection_timeout(1, 0);
  const auto resp = after_scope.Get("/");
  expect(!resp, "a request after the handle's destructor ran should fail to connect");
}

} // namespace

int main() {
  (void)::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 1);
  test_serve_root_and_assets();
  test_serve_flat_chrome_shows_all_boards();
  test_serve_category_filter_round_trips();
  test_serve_range_and_temporal_filters();
  test_serve_rejects_adversarial_payloads();
  test_serve_rejects_oversized_payload();
  test_serve_concurrent_requests_smoke();
  test_serve_handle_stop_actually_stops();
  test_serve_handle_destructor_stops_server();
  return 0;
}
