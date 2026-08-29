#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::table::DataFrame;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

void expect_throws_contains(
    const std::function<void()>& fn,
    std::string_view needle,
    std::string_view message) {
  bool matched = false;
  try {
    fn();
  } catch (const std::exception& e) {
    matched = std::string(e.what()).find(std::string(needle)) != std::string::npos;
  }
  expect(matched, message);
}

bool has_feature(const DataFrame& frame, std::string_view name) {
  const auto& features = frame.features();
  return std::find(features.begin(), features.end(), std::string(name)) != features.end();
}

std::filesystem::path bi_temp_root(std::string_view name) {
  auto root = std::filesystem::temp_directory_path() / "pluss_bi_tests" / std::string(name);
  std::error_code ec;
  std::filesystem::remove_all(root, ec);
  std::filesystem::create_directories(root, ec);
  if (ec) {
    fail("failed to create BI temp directory");
  }
  return root;
}

std::uintmax_t file_size_checked(const std::filesystem::path& path) {
  std::error_code ec;
  const auto size = std::filesystem::file_size(path, ec);
  if (ec) {
    fail("failed to read output file size");
  }
  return size;
}

pluss::bundle::Nexus make_bi_nexus() {
  auto customers = DataFrame({
      {"customer_id", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})},
      {"name", pluss::make_note({"Ada", "Bo", "Cy"})},
      {"segment", pluss::make_note({"retail", "retail", "enterprise"})},
  }).title("customers");

  auto loans = DataFrame({
      {"loan_id", pluss::make_series(std::vector<double>{10.0, 11.0, 12.0, 13.0})},
      {"customer_id", pluss::make_series(std::vector<double>{1.0, 2.0, 2.0, 3.0})},
      {"principal", pluss::make_series(std::vector<double>{1000.0, 1200.0, 900.0, 1500.0})},
      {"risk_band", pluss::make_note({"A", "B", "B", "A"})},
      {"opened_at", pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
      })},
  }).title("loans");

  auto payments = DataFrame({
      {"payment_id", pluss::make_series(std::vector<double>{100.0, 101.0, 102.0, 103.0, 104.0})},
      {"loan_id", pluss::make_series(std::vector<double>{10.0, 10.0, 11.0, 12.0, 13.0})},
      {"amount", pluss::make_series(std::vector<double>{100.0, 50.0, 70.0, 90.0, 120.0})},
  }).title("payments");

  using namespace pluss::bundle;
  using namespace pluss::bundle::nexus;

  auto nexus = Nexus({
      {"customers", customers},
      {"loans", loans},
      {"payments", payments},
  });

  nexus = nexus.add_relationship(Relationship{
      .name = "loan_customer",
      .left_table = "loans",
      .right_table = "customers",
      .keys = {{"customer_id", "customer_id"}},
      .cardinality = RelationshipCardinality::ManyToOne,
  });

  nexus = nexus.add_relationship(Relationship{
      .name = "loan_payment",
      .left_table = "loans",
      .right_table = "payments",
      .keys = {{"loan_id", "loan_id"}},
      .cardinality = RelationshipCardinality::OneToMany,
  });

  nexus = nexus.store("loans_payments", nexus.plan("loans", "payments"));
  return nexus;
}

void test_session_and_controls() {
  using namespace pluss::bi;

  Session session;
  expect(session.empty(), "new BI session should start empty");

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment");
  apply(session, segment, std::vector<std::string>{"retail"});
  expect(session.has_filter("segment_filter"), "categorical control should register a session filter");
  expect(session.version() == 1, "setting a filter should bump session version");

  auto principal = range("principal_range");
  principal.feature("principal").label("Principal");
  apply(session, principal, 950.0, 1300.0);
  expect(session.has_filter("principal_range"), "numeric control should register a range filter");
  expect(session.version() == 2, "setting a second filter should bump session version");

  clear(session, segment);
  expect(!session.has_filter("segment_filter"), "clearing a control should remove its filter");
  expect(session.version() == 3, "clearing a filter should bump session version");
}

void test_source_query_and_projection_after_filters() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("credit_studio");
  studio.source(nexus);

  auto loans_view = table("loan_table");
  loans_view
      .query(from("loans")
                 .where(Filter::eq("risk_band", "A"))
                 .select({"loan_id", "principal"}))
      .title("Loans")
      .place(0, 0);

  auto& board = studio.board("credit");
  board.grid(1, 1);
  board.view(loans_view);

  const auto render = studio.render();
  expect(render.boards.size() == 1, "BI studio should render one board");
  expect(render.boards[0].tiles.size() == 1, "BI board should render one tile");
  expect(render.boards[0].tiles[0].table != nullptr, "detail-table tile should materialize a DataFrame");
  expect(render.boards[0].tiles[0].table->features() == std::vector<std::string>({"loan_id", "principal"}),
         "projection should happen after filtering and preserve only requested features");
  expect(render.boards[0].tiles[0].table->ncases() == 2,
         "local BI filters should narrow the source frame before projection");
}

void test_scoped_session_filters_skip_or_throw_correctly() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("scoped");
  studio.source(nexus);
  studio.set_filter(Filter::eq("segment", "retail").scoped("customers_scope"));

  auto customers_view = table("customers_view");
  customers_view
      .query(from("customers").scope_as("customers_scope"))
      .title("Customers")
      .place(0, 0);

  auto loans_view = table("loans_view");
  loans_view
      .query(from("loans").scope_as("loans_scope"))
      .title("Loans")
      .place(0, 1);

  auto& board = studio.board("b");
  board.grid(1, 2);
  board.view(customers_view);
  board.view(loans_view);

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].table->ncases() == 2,
         "scoped session filters should apply when the query scope matches");
  expect(render.boards[0].tiles[1].table->ncases() == 4,
         "scoped session filters should skip unrelated query scopes");

  Studio bad("scoped_missing");
  bad.source(nexus);
  bad.set_filter(Filter::eq("segment", "retail").scoped("loans_scope"));

  auto bad_view = table("bad_view");
  bad_view
      .query(from("loans").scope_as("loans_scope"))
      .title("Loans")
      .place(0, 0);

  auto& bad_board = bad.board("bad");
  bad_board.grid(1, 1);
  bad_board.view(bad_view);

  expect_throws_contains(
      [&]() { (void)bad.render(); },
      "missing feature 'segment'",
      "scoped session filters should throw when they target a missing feature on a matching query scope");
}

void test_unscoped_session_filters_skip_missing_features() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("unscoped_skip");
  studio.source(nexus);
  studio.set_filter(Filter::eq("segment", "retail"));

  auto loans_view = table("loans_view");
  loans_view
      .query(from("loans"))
      .title("Loans")
      .place(0, 0);

  auto& board = studio.board("loans_board");
  board.grid(1, 1);
  board.view(loans_view);

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].table->ncases() == 4,
         "unscoped session filters should skip views that do not carry the target feature");
}

void test_temporal_filters_and_query_windowing() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("temporal");
  studio.source(nexus);

  auto opened_range = time_range("opened_range");
  opened_range
      .feature("opened_at")
      .label("Opened")
      .scope("loans_scope");

  Session session;
  apply_time(
      session,
      opened_range,
      pluss::make_timeline({"2026-01-02T00:00:00Z"}),
      pluss::make_timeline({"2026-01-03T00:00:00Z"}));
  studio.session(session);

  auto window_view = table("window_view");
  window_view
      .query(from("loans")
                 .scope_as("loans_scope")
                 .drop({"customer_id"})
                 .head(2)
                 .named("Loan Window"))
      .title("Loan Window")
      .place(0, 0);

  auto& board = studio.board("window");
  board.grid(1, 1);
  board.view(window_view);

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].table->title() == "Loan Window",
         "named() should retitle the materialized query result");
  expect(!has_feature(*render.boards[0].tiles[0].table, "customer_id"),
         "drop() should remove unwanted features after filtering");
  expect(render.boards[0].tiles[0].table->ncases() == 2,
         "temporal filters followed by head() should keep the requested window size");
}

void test_query_arrange_orders_results_before_windowing() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("arrange");
  studio.source(nexus);

  auto top_loans = table("view");
  top_loans
      .query(from("loans")
                 .arrange("principal", false)
                 .head(2)
                 .select({"loan_id", "principal"}))
      .title("Top Loans")
      .place(0, 0);

  auto& board = studio.board("board");
  board.grid(1, 1);
  board.view(top_loans);

  const auto render = studio.render();
  const auto top = *render.boards[0].tiles[0].table;
  const auto principal = top.features("principal").as_series().to_vec<double>();
  expect(top.ncases() == 2,
         "arrange() combined with head() should keep only the requested top rows");
  expect(std::fabs(principal[0] - 1500.0) < 1e-12,
         "arrange(descending) should place the largest numeric value first");
  expect(std::fabs(principal[1] - 1200.0) < 1e-12,
         "arrange(descending) should preserve the correct second-ranked row");
}

void test_query_group_pivot_and_resample_plans() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("phase6");
  studio.source(nexus);

  auto grouped = table("loan_grouped");
  grouped
      .query(from("loans")
                 .group({"risk_band"})
                 .agg({
                     {"loan_count", "", pluss::table::GroupOp::Count},
                     {"principal_sum", "principal", pluss::table::GroupOp::Sum},
                 })
                 .arrange("risk_band"))
      .title("Grouped Loans")
      .place(0, 0);

  auto pivoted = table("loan_pivoted");
  pivoted
      .query(from("loans")
                 .pivot({"risk_band"}, {"customer_id"}, {"principal"},
                        pluss::table::GroupOp::Sum, true, true))
      .title("Pivoted Loans")
      .place(0, 1);

  auto sampled = table("loan_sampled");
  sampled
      .query(from("loans")
                 .resample("opened_at", {"principal"}, "2D",
                           pluss::subsp::stats::ts::ResampleOp::Sum))
      .title("Sampled Loans")
      .place(1, 0, 1, 2);

  auto& board = studio.board("overview");
  board.grid(2, 2);
  board.view(grouped);
  board.view(pivoted);
  board.view(sampled);

  const auto render = studio.render();
  const auto& grouped_frame = *render.boards[0].tiles[0].table;
  expect(grouped_frame.ncases() == 2,
         "query.group(...).agg(...) should produce one row per grouped key");
  expect(has_feature(grouped_frame, "risk_band"),
         "grouped BI query should preserve its grouping key");
  expect(has_feature(grouped_frame, "loan_count"),
         "grouped BI query should materialize aggregate outputs");
  expect(has_feature(grouped_frame, "principal_sum"),
         "grouped BI query should materialize multiple aggregate outputs");

  const auto group_a = grouped_frame.filter(grouped_frame.features("risk_band") == "A");
  const auto group_b = grouped_frame.filter(grouped_frame.features("risk_band") == "B");
  expect(group_a.ncases() == 1 && group_b.ncases() == 1,
         "grouped BI query should contain both grouped risk bands");
  expect(group_a.features("principal_sum").as_series().item<double>() == 2500.0,
         "grouped BI query should sum principal for risk band A");
  expect(group_b.features("principal_sum").as_series().item<double>() == 2100.0,
         "grouped BI query should sum principal for risk band B");

  const auto& pivoted_frame = *render.boards[0].tiles[1].table;
  expect(pivoted_frame.ncases() == 2,
         "query.pivot(...) should preserve one row per pivoted index combination");
  expect(pivoted_frame.nfeatures() >= 2,
         "query.pivot(...) should materialize reshaped feature columns");

  const auto& sampled_frame = *render.boards[0].tiles[2].table;
  expect(sampled_frame.ncases() == 2,
         "query.resample(...) should reduce the temporal axis into 2-day buckets");
  expect(has_feature(sampled_frame, "principal"),
         "query.resample(...) should preserve the sampled value feature");
  expect(sampled_frame.features("principal").as_series().sum().item<double>() == 4600.0,
         "query.resample(...) should aggregate principal across all resampled buckets");
}

void test_control_tiles_materialize_active_state() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment").place(0, 0);

  auto customers_view = table("customers_view");
  customers_view
      .query(from("customers"))
      .title("Customers")
      .place(0, 1);

  Studio studio("controls");
  studio.source(nexus);
  studio.set_filter(Filter::eq("segment", "retail"));

  auto& board = studio.board("board");
  board.grid(1, 2);
  board.control(segment);
  board.view(customers_view);

  const auto render = studio.render();
  expect(render.boards[0].tiles.size() == 2,
         "render() should include both control tiles and view tiles");
  expect(render.boards[0].tiles[0].is_control,
         "control tiles should be materialized explicitly in render output");
  expect(render.boards[0].tiles[0].control.has_value(),
         "control tile render output should expose the control definition");
  expect(render.boards[0].tiles[0].active_filter.has_value(),
         "control tile render output should expose the active session filter");
  expect(render.boards[0].tiles[0].active_filter->feature == "segment",
         "control tile render output should preserve the active filter feature");
  expect(render.boards[0].tiles[1].table->ncases() == 2,
         "view tiles should continue to honor the shared session filter");
}

void test_query_filters_throw_on_missing_features() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("bad_query");
  studio.source(nexus);

  auto bad_view = table("view");
  bad_view
      .query(from("loans").where(Filter::eq("segment", "retail")))
      .title("Loans")
      .place(0, 0);

  auto& board = studio.board("board");
  board.grid(1, 1);
  board.view(bad_view);

  expect_throws_contains(
      [&]() { (void)studio.render(); },
      "missing feature 'segment'",
      "query-local filters should throw when they target missing features");
}

void test_kpi_and_plot_tiles() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto mean_principal = kpi("mean_principal");
  mean_principal
      .query(from("loans"))
      .feature("principal")
      .op(pluss::table::GroupOp::Mean)
      .title("Mean Principal")
      .place(0, 0);

  auto principal_curve = line("principal_curve");
  principal_curve
      .query(from("loans").select({"opened_at", "principal"}))
      .x("opened_at")
      .y("principal")
      .title("Principal Curve")
      .place(0, 1);

  auto risk_counts = bar("risk_counts");
  risk_counts
      .query(from("loans").select({"risk_band"}))
      .feature("risk_band")
      .title("Risk Counts")
      .place(0, 2);

  Studio studio("plots");
  studio.source(nexus);

  auto& board = studio.board("perf");
  board.grid(1, 3);
  board.view(mean_principal);
  board.view(principal_curve);
  board.view(risk_counts);

  const auto render = studio.render();
  expect(render.boards[0].tiles.size() == 3, "all BI tiles should render");
  expect(render.boards[0].tiles[0].scalar.has_value(), "KPI tile should return a scalar");
  expect(std::fabs(pluss::aux::as_f64(render.boards[0].tiles[0].scalar->as_series()) - 1150.0) < 1e-12,
         "KPI reducer should compute the correct mean");
  expect(render.boards[0].tiles[1].plot != nullptr, "line tile should build a Flash studio");
  expect(render.boards[0].tiles[1].plot->pane_count() == 1, "line tile should create exactly one pane");
  expect(render.boards[0].tiles[2].plot != nullptr, "bar tile should build a Flash studio");
}

void test_pivot_tile() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto pivot_view = pivot("pivot");
  pivot_view
      .query(from("loans"))
      .pivot_index({"risk_band"})
      .pivot_columns({"customer_id"})
      .pivot_values({"principal"})
      .op(pluss::table::GroupOp::Sum)
      .dropna(true)
      .sort(true)
      .title("Principal Pivot")
      .place(0, 0);

  Studio studio("pivot");
  studio.source(nexus);

  auto& board = studio.board("board");
  board.grid(1, 1);
  board.view(pivot_view);

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].table != nullptr, "pivot tile should materialize a pivoted DataFrame");
  const auto pivoted = *render.boards[0].tiles[0].table;
  expect(pivoted.ncases() >= 2, "pivot tile should produce grouped rows");
  expect(pivoted.nfeatures() >= 2, "pivot tile should produce grouped columns");
}

void test_phase7_scatter_heatmap_and_corr_tiles() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto scatter_view = scatter("principal_scatter");
  scatter_view
      .query(from("loans").select({"customer_id", "principal"}))
      .x("customer_id")
      .y("principal")
      .title("Principal vs Customer")
      .place(0, 0);

  auto heatmap_view = heatmap("loan_heatmap");
  heatmap_view
      .query(from("loans").select({"loan_id", "customer_id", "principal"}))
      .x("loan_id")
      .y("customer_id")
      .z("principal")
      .title("Loan Heatmap")
      .place(0, 1);

  auto corr_view = corr("loan_corr");
  corr_view
      .query(from("loans"))
      .features({"loan_id", "customer_id", "principal"})
      .title("Loan Correlation")
      .place(1, 0, 1, 2);

  auto grouped_bar = bar("risk_mix");
  grouped_bar
      .query(from("loans")
                 .group({"risk_band"})
                 .agg({
                     {"loan_count", "", pluss::table::GroupOp::Count},
                 })
                 .arrange("loan_count", false))
      .x("risk_band")
      .y("loan_count")
      .title("Risk Mix")
      .place(2, 0, 1, 2);

  Studio studio("phase7");
  studio.source(nexus);

  auto& board = studio.board("overview");
  board.grid(3, 2);
  board.view(scatter_view);
  board.view(heatmap_view);
  board.view(corr_view);
  board.view(grouped_bar);

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].plot != nullptr,
         "phase-7 scatter BI tile should materialize a plot");
  expect(render.boards[0].tiles[1].plot != nullptr,
         "phase-7 heatmap BI tile should materialize a plot");
  expect(render.boards[0].tiles[2].plot != nullptr,
         "phase-7 corr BI tile should materialize a plot");
  expect(render.boards[0].tiles[3].plot != nullptr,
         "grouped categorical BI bar tile should materialize a plot");
  expect(render.boards[0].tiles[2].plot->pane_count() == 1,
         "phase-7 corr BI tile should render as a single heatmap pane");
  expect(render.boards[0].native != nullptr,
         "phase-7 BI board should compose into the native board runtime");
}

void test_stored_plan_sql_and_traverse_queries() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto stored_view = table("stored");
  stored_view
      .query(stored("loans_payments"))
      .title("Stored")
      .place(0, 0);

  auto sql_view = table("sql");
  sql_view
      .query(sql(
          "SELECT loans.loan_id, customers.name "
          "FROM loans "
          "INNER JOIN customers ON loans.customer_id = customers.customer_id"))
      .title("Sql")
      .place(0, 1);

  auto traverse_view = table("traverse");
  traverse_view
      .query(traverse_from("loans").join("customers"))
      .title("Traverse")
      .place(0, 2);

  Studio studio("queries_studio");
  studio.source(nexus);

  auto& board = studio.board("queries");
  board.grid(1, 3);
  board.view(stored_view);
  board.view(sql_view);
  board.view(traverse_view);

  const auto render = studio.render();
  expect(has_feature(*render.boards[0].tiles[0].table, "loan_id"),
         "stored-plan query should keep left-table features");
  expect(has_feature(*render.boards[0].tiles[0].table, "amount"),
         "stored-plan query should expose joined right-table features");
  expect(render.boards[0].tiles[0].table->ncases() == 5,
         "stored-plan query should materialize the joined plan");
  expect(render.boards[0].tiles[1].table->nfeatures() == 2,
         "SQL query should resolve through Nexus::query().execute");
  expect(render.boards[0].tiles[1].table->ncases() == 4,
         "SQL query should preserve the loan-side row count");
  expect(has_feature(*render.boards[0].tiles[2].table, "loan_id"),
         "traverse query should keep left-table features");
  expect(has_feature(*render.boards[0].tiles[2].table, "name"),
         "traverse query should expose joined relationship features");
  expect(render.boards[0].tiles[2].table->ncases() == 4,
         "traverse query should preserve the left-side row count for the direct relationship");
}

void test_board_validation() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  auto good_view = table("tile");
  good_view
      .query(from("loans"))
      .title("Loans")
      .place(0, 0);

  bool duplicate_threw = false;
  try {
    Studio studio("dup");
    studio.source(nexus);

    auto first = good_view;
    auto second = table("tile");
    second.query(from("loans")).title("Loans 2").place(0, 1);

    auto& board = studio.board("board");
    board.grid(1, 2);
    board.view(first);
    board.view(second);
    (void)studio.render();
  } catch (const std::exception& e) {
    duplicate_threw = std::string(e.what()).find("duplicate tile key") != std::string::npos;
  }
  expect(duplicate_threw, "duplicate BI tile keys should be rejected");

  bool bounds_threw = false;
  try {
    Studio studio("bounds");
    studio.source(nexus);

    auto bad_view = table("tile");
    bad_view
        .query(from("loans"))
        .title("Loans")
        .place(0, 0, 1, 2);

    auto& board = studio.board("board");
    board.grid(1, 1);
    board.view(bad_view);
    (void)studio.render();
  } catch (const std::exception& e) {
    bounds_threw = std::string(e.what()).find("column span exceeds board shape") != std::string::npos;
  }
  expect(bounds_threw, "out-of-bounds BI tiles should be rejected");
}

void test_native_runtime_and_cache_flow() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();

  Studio studio("runtime");
  studio.source(nexus);

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment").place(0, 0);

  auto customers_view = table("customers_table");
  customers_view
      .query(from("customers"))
      .title("Customers")
      .place(0, 1);

  auto customer_count = kpi("customer_count");
  customer_count
      .query(from("customers"))
      .feature("customer_id")
      .op(pluss::table::GroupOp::Count)
      .title("Customer Count")
      .place(0, 2);

  auto loan_curve = line("loan_curve");
  loan_curve
      .query(from("loans").select({"opened_at", "principal"}))
      .x("opened_at")
      .y("principal")
      .title("Principal Curve")
      .place(1, 0, 1, 2);

  auto& board = studio.board("overview");
  board.title("Overview");
  board.grid(2, 3);
  board.control(segment);
  board.view(customers_view);
  board.view(customer_count);
  board.view(loan_curve);

  studio.set_filter(Filter::eq("segment", "retail"));

  const auto first = studio.render();
  expect(first.native != nullptr, "studio render should materialize a native board studio");
  expect(first.boards[0].native != nullptr, "board render should materialize a native board runtime");
  expect(first.cache.query_hits > 0,
         "first render should reuse central BI query results across tiles with identical queries");
  expect(first.boards[0].native->pane_count() == 4,
         "board native runtime should compose one pane per control/view tile");
  expect(first.dependencies.count("segment") == 1,
         "render should expose dependency bookkeeping for active filters");
  expect(!first.dependencies.at("segment").empty(),
         "active filter dependency bookkeeping should name affected tiles");

  const auto second = studio.render();
  expect(second.cache.tile_hits > 0, "second render should hit the BI tile cache");
  expect(second.cache.board_hits > 0, "second render should hit the BI board-native cache");

  studio.set_filter(Filter::eq("segment", "enterprise"));
  const auto third = studio.render();
  expect(third.cache.query_misses > 0,
         "changing an active filter should invalidate BI query results for a fresh render");
  expect(third.cache.tile_hits > 0 && third.cache.tile_misses > 0,
         "filter changes should selectively reuse unaffected BI tiles while rebuilding affected ones");
  expect(third.boards[0].native != nullptr,
         "native board runtime should rebuild cleanly after filter invalidation");

  studio.refresh();
  const auto fourth = studio.render();
  expect(fourth.cache.query_misses > 0, "refresh() should clear the BI query cache");
  expect(fourth.cache.tile_misses > 0, "refresh() should clear the BI tile cache");
  expect(fourth.cache.board_misses > 0, "refresh() should clear the BI board-native cache");

  expect(studio.show("BI runtime", 960, 640, false),
         "Studio::show(..., launch=false) should still render the native BI surface");
}

void test_phase8_save_export_and_restore() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();
  const auto root = bi_temp_root("phase8");
  const auto bundle_path = root / "credit_bundle";
  const auto html_path = root / "credit_board";
  const auto svg_path = root / "credit_board.svg";
  const auto pdf_path = root / "credit_board.pdf";

  Studio studio("credit");
  studio.source(nexus);

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment").scope("customers_scope").place(0, 0);

  Session session;
  apply(session, segment, std::vector<std::string>{"retail"});
  session.set_param("dpi", 180.0);
  session.set_param("theme", "paper");
  studio.session(session);

  auto grouped_bar = bar("risk_mix");
  grouped_bar
      .query(from("loans")
                 .group({"risk_band"})
                 .agg({
                     {"loan_count", "", pluss::table::GroupOp::Count},
                     {"principal_sum", "principal", pluss::table::GroupOp::Sum},
                 })
                 .arrange("loan_count", false))
      .x("risk_band")
      .y("loan_count")
      .title("Risk Mix")
      .place(0, 1);

  auto stored_view = table("stored_view");
  stored_view
      .query(stored("loans_payments").head(3))
      .title("Stored Payments")
      .place(1, 0);

  auto sql_view = table("sql_view");
  sql_view
      .query(sql(
          "SELECT loans.loan_id, customers.name "
          "FROM loans "
          "INNER JOIN customers ON loans.customer_id = customers.customer_id"))
      .title("SQL Join")
      .place(1, 1);

  auto traverse_view = table("traverse_view");
  traverse_view
      .query(traverse_from("loans").join("customers"))
      .title("Traverse Join")
      .place(2, 0);

  auto sampled_table = table("sampled_table");
  sampled_table
      .query(from("loans")
                 .resample("opened_at", {"principal"}, "2D",
                           pluss::subsp::stats::ts::ResampleOp::Sum))
      .title("Sampled Principal")
      .place(2, 1);

  auto& board = studio.board("overview");
  board.title("Credit Overview");
  board.grid(3, 2);
  board.control(segment);
  board.view(grouped_bar);
  board.view(stored_view);
  board.view(sql_view);
  board.view(traverse_view);
  board.view(sampled_table);

  studio.save(bundle_path.string());
  studio.export_html(html_path.string(), "Credit Board", 960, 640);
  studio.export_svg(svg_path.string());
  studio.export_pdf(pdf_path.string());

  expect(std::filesystem::exists(bundle_path.string() + ".plussbi"),
         "phase-8 save should write a BI bundle file");
  expect(std::filesystem::exists(html_path.string() + ".html"),
         "phase-8 export_html should write an HTML artifact");
  expect(std::filesystem::exists(svg_path),
         "phase-8 export_svg should write an SVG artifact");
  expect(std::filesystem::exists(pdf_path),
         "phase-8 export_pdf should write a PDF artifact");
  expect(file_size_checked(bundle_path.string() + ".plussbi") > 0,
         "saved BI bundle should not be empty");
  expect(file_size_checked(html_path.string() + ".html") > 0,
         "exported BI HTML should not be empty");
  expect(file_size_checked(svg_path) > 0,
         "exported BI SVG should not be empty");
  expect(file_size_checked(pdf_path) > 0,
         "exported BI PDF should not be empty");

  const auto detached = Studio::load(bundle_path.string());
  expect(detached.key() == "credit", "detached Studio::load should restore the studio key");
  expect(detached.session().has_filter("segment_filter"),
         "detached Studio::load should restore saved session filters");
  expect(detached.session().has_param("dpi"),
         "detached Studio::load should restore saved session params");
  expect_throws_contains(
      [&]() { (void)detached.render(); },
      "source Nexus has not been set",
      "detached Studio::load should require source rebinding before render");

  const auto rebound = Studio::load(bundle_path.string(), nexus);
  const auto render = rebound.render();
  expect(render.boards.size() == 1, "rebound Studio::load should restore board structure");
  expect(render.boards[0].tiles.size() == 6,
         "rebound Studio::load should restore every saved tile");
  expect(render.boards[0].tiles[1].plot != nullptr,
         "rebound Studio::load should restore grouped plot views");
  expect(render.boards[0].tiles[2].table != nullptr,
         "rebound Studio::load should restore stored-plan table views");
  expect(render.boards[0].tiles[3].table != nullptr,
         "rebound Studio::load should restore SQL table views");
  expect(render.boards[0].tiles[4].table != nullptr,
         "rebound Studio::load should restore traverse table views");
  expect(render.boards[0].tiles[5].table != nullptr,
         "rebound Studio::load should restore resampled table views");
  expect(render.boards[0].tiles[2].table->ncases() == 3,
         "saved head() windows should survive load round-trips");
  expect(render.boards[0].tiles[5].table->ncases() == 2,
         "saved resample queries should survive load round-trips");

  Studio session_only("session_only");
  session_only.restore_session(bundle_path.string());
  expect(session_only.session().has_filter("segment_filter"),
         "restore_session should restore saved filters without replacing boards");
  expect(session_only.session().has_param("theme"),
         "restore_session should restore saved params without replacing boards");
}

void test_phase9_hardening_and_roundtrip_cache() {
  using namespace pluss::bi;

  const auto nexus = make_bi_nexus();
  const auto root = bi_temp_root("phase9");
  const auto bad_header = root / "bad_header.plussbi";
  const auto duplicate_board = root / "duplicate_board.plussbi";

  {
    std::ofstream out(bad_header, std::ios::out | std::ios::trunc | std::ios::binary);
    out << "NOTPLUSS|1\n";
  }
  expect_throws_contains(
      [&]() { (void)Studio::load(bad_header.string()); },
      "malformed bundle header",
      "phase-9 hardening should reject malformed BI bundle headers");

  {
    std::ofstream out(duplicate_board, std::ios::out | std::ios::trunc | std::ios::binary);
    out << "PLUSSBI|1\n";
    out << "studio|dup\n";
    out << "board|overview||1|1\n";
    out << "board|overview||1|1\n";
  }
  expect_throws_contains(
      [&]() { (void)Studio::load(duplicate_board.string()); },
      "duplicate board key",
      "phase-9 hardening should reject duplicate board keys in saved bundles");

  Studio invalid("invalid");
  invalid.source(nexus);
  auto missing_feature = kpi("missing_feature");
  missing_feature
      .query(from("loans"))
      .title("Broken KPI")
      .place(0, 0);
  auto& invalid_board = invalid.board("overview");
  invalid_board.grid(1, 1);
  invalid_board.view(missing_feature);
  expect_throws_contains(
      [&]() { invalid.save((root / "invalid_bundle").string()); },
      "KPI view requires feature",
      "phase-9 hardening should reject malformed KPI bindings during save");

  Studio studio("cache_roundtrip");
  studio.source(nexus);

  auto line_view = line("curve");
  line_view
      .query(from("loans").select({"opened_at", "principal"}))
      .x("opened_at")
      .y("principal")
      .title("Curve")
      .place(0, 0);

  auto count_view = kpi("count");
  count_view
      .query(from("loans"))
      .feature("loan_id")
      .op(pluss::table::GroupOp::Count)
      .title("Loan Count")
      .place(0, 1);

  auto& board = studio.board("overview");
  board.grid(1, 2);
  board.view(line_view);
  board.view(count_view);

  const auto bundle_path = root / "cache_bundle";
  studio.save(bundle_path.string());
  auto loaded = Studio::load(bundle_path.string(), nexus);
  const auto first = loaded.render();
  const auto second = loaded.render();
  expect(first.cache.query_misses > 0,
         "first render after round-trip load should materialize BI query results");
  expect(second.cache.tile_hits > 0,
         "second render after round-trip load should reuse BI tile cache");
  expect(second.cache.board_hits > 0,
         "second render after round-trip load should reuse BI board cache");
}

} // namespace

int main() {
  (void)::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 1);
  test_session_and_controls();
  test_source_query_and_projection_after_filters();
  test_scoped_session_filters_skip_or_throw_correctly();
  test_unscoped_session_filters_skip_missing_features();
  test_temporal_filters_and_query_windowing();
  test_query_arrange_orders_results_before_windowing();
  test_query_group_pivot_and_resample_plans();
  test_control_tiles_materialize_active_state();
  test_query_filters_throw_on_missing_features();
  test_kpi_and_plot_tiles();
  test_pivot_tile();
  test_phase7_scatter_heatmap_and_corr_tiles();
  test_stored_plan_sql_and_traverse_queries();
  test_board_validation();
  test_native_runtime_and_cache_flow();
  test_phase8_save_export_and_restore();
  test_phase9_hardening_and_roundtrip_cache();
  return 0;
}
