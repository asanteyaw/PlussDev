#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

bool has_feature(const DataFrame& frame, std::string_view name) {
  const auto& features = frame.features();
  return std::find(features.begin(), features.end(), std::string(name)) != features.end();
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
  pluss::bi::Session session;
  expect(session.empty(), "new BI session should start empty");

  const auto segment = pluss::bi::Control::category("segment_filter", "segment", "Segment");
  pluss::bi::apply(session, segment, std::vector<std::string>{"retail"});
  expect(session.has_filter("segment_filter"), "categorical control should register a session filter");
  expect(session.version() == 1, "setting a filter should bump session version");

  const auto principal = pluss::bi::Control::numeric_range("principal_range", "principal", "Principal");
  pluss::bi::apply(session, principal, 950.0, 1300.0);
  expect(session.has_filter("principal_range"), "numeric control should register a range filter");
  expect(session.version() == 2, "setting a second filter should bump session version");

  pluss::bi::clear(session, segment);
  expect(!session.has_filter("segment_filter"), "clearing a control should remove its filter");
  expect(session.version() == 3, "clearing a filter should bump session version");
}

void test_source_query_and_projection_after_filters() {
  const auto nexus = make_bi_nexus();

  const auto query =
      pluss::bi::QuerySpec::from("loans")
          .where(pluss::bi::Filter::eq("risk_band", "A"))
          .select({"loan_id", "principal"});

  const auto view = pluss::bi::View::make(
      "loan_table",
      query,
      pluss::bi::BindSpec::detail_table("Loans"));

  const auto board =
      pluss::bi::Board("credit").shape(1, 1).add(
          pluss::bi::Tile::from_view("loan_tile", pluss::bi::LayoutSpec::at(0, 0), view));

  pluss::bi::Studio studio("credit_studio");
  studio.source(nexus).add(board);

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
  const auto nexus = make_bi_nexus();

  pluss::bi::Studio studio("scoped");
  studio.source(nexus);
  studio.set_filter(pluss::bi::Filter::eq("segment", "retail").scoped("customers_scope"));

  const auto customer_view = pluss::bi::View::make(
      "customers_view",
      pluss::bi::QuerySpec::from("customers").scope_as("customers_scope"),
      pluss::bi::BindSpec::detail_table("Customers"));

  const auto loan_view = pluss::bi::View::make(
      "loans_view",
      pluss::bi::QuerySpec::from("loans").scope_as("loans_scope"),
      pluss::bi::BindSpec::detail_table("Loans"));

  auto board = pluss::bi::Board("b").shape(1, 2);
  board = board.add(pluss::bi::Tile::from_view("customers_tile", pluss::bi::LayoutSpec::at(0, 0), customer_view));
  board = board.add(pluss::bi::Tile::from_view("loans_tile", pluss::bi::LayoutSpec::at(0, 1), loan_view));

  studio.add(board);
  std::cerr << "  scoped render 1\n";
  const auto render = studio.render();
  std::cerr << "  scoped render 1 done\n";
  expect(render.boards[0].tiles[0].table->ncases() == 2,
         "scoped session filters should apply when the query scope matches");
  expect(render.boards[0].tiles[1].table->ncases() == 4,
         "scoped session filters should skip unrelated query scopes");

  pluss::bi::Studio bad("scoped_missing");
  bad.source(nexus);
  bad.set_filter(pluss::bi::Filter::eq("segment", "retail").scoped("loans_scope"));
  bad.add(pluss::bi::Board("bad").shape(1, 1).add(
      pluss::bi::Tile::from_view(
          "bad_tile",
          pluss::bi::LayoutSpec::at(0, 0),
          pluss::bi::View::make(
              "bad_view",
              pluss::bi::QuerySpec::from("loans").scope_as("loans_scope"),
              pluss::bi::BindSpec::detail_table("Loans")))));

  bool threw = false;
  try {
    std::cerr << "  scoped render 2\n";
    (void)bad.render();
  } catch (const std::exception& e) {
    std::cerr << "  scoped render 2 threw: " << e.what() << "\n";
    threw = std::string(e.what()).find("missing feature 'segment'") != std::string::npos;
  }
  expect(threw,
         "scoped session filters should throw when they target a missing feature on a matching query scope");
}

void test_kpi_and_plot_tiles() {
  const auto nexus = make_bi_nexus();

  auto board = pluss::bi::Board("perf").shape(1, 3);
  board = board.add(pluss::bi::Tile::from_view(
      "kpi",
      pluss::bi::LayoutSpec::at(0, 0),
      pluss::bi::View::make(
          "mean_principal",
          pluss::bi::QuerySpec::from("loans"),
          pluss::bi::BindSpec::kpi("principal", pluss::table::GroupOp::Mean, "Mean Principal"))));
  board = board.add(pluss::bi::Tile::from_view(
      "line",
      pluss::bi::LayoutSpec::at(0, 1),
      pluss::bi::View::make(
          "principal_curve",
          pluss::bi::QuerySpec::from("loans").select({"opened_at", "principal"}),
          pluss::bi::BindSpec::line(
              {.x_feature = "opened_at", .y_feature = "principal"},
              "Principal Curve"))));
  board = board.add(pluss::bi::Tile::from_view(
      "bar",
      pluss::bi::LayoutSpec::at(0, 2),
      pluss::bi::View::make(
          "risk_counts",
          pluss::bi::QuerySpec::from("loans").select({"risk_band"}),
          pluss::bi::BindSpec::bar(
              {.feature = "risk_band"},
              false,
              "Risk Counts"))));

  pluss::bi::Studio studio("plots");
  studio.source(nexus).add(board);

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
  const auto nexus = make_bi_nexus();

  const auto view = pluss::bi::View::make(
      "pivot",
      pluss::bi::QuerySpec::from("loans"),
      pluss::bi::BindSpec::pivot_table(
          {"risk_band"},
          {"customer_id"},
          {"principal"},
          pluss::table::GroupOp::Sum,
          true,
          true,
          "Principal Pivot"));

  pluss::bi::Studio studio("pivot");
  studio.source(nexus).add(
      pluss::bi::Board("board").shape(1, 1).add(
          pluss::bi::Tile::from_view("pivot_tile", pluss::bi::LayoutSpec::at(0, 0), view)));

  const auto render = studio.render();
  expect(render.boards[0].tiles[0].table != nullptr, "pivot tile should materialize a pivoted DataFrame");
  const auto pivoted = *render.boards[0].tiles[0].table;
  expect(pivoted.ncases() >= 2, "pivot tile should produce grouped rows");
  expect(pivoted.nfeatures() >= 2, "pivot tile should produce grouped columns");
}

void test_stored_plan_sql_and_traverse_queries() {
  const auto nexus = make_bi_nexus();

  const auto stored_view = pluss::bi::View::make(
      "stored",
      pluss::bi::QuerySpec::stored("loans_payments"),
      pluss::bi::BindSpec::detail_table("Stored"));

  const auto sql_view = pluss::bi::View::make(
      "sql",
      pluss::bi::QuerySpec::sql(
          "SELECT loans.loan_id, customers.name "
          "FROM loans "
          "INNER JOIN customers ON loans.customer_id = customers.customer_id"),
      pluss::bi::BindSpec::detail_table("Sql"));

  const auto traverse_view = pluss::bi::View::make(
      "traverse",
      pluss::bi::QuerySpec::traverse_from("loans")
          .join("customers"),
      pluss::bi::BindSpec::detail_table("Traverse"));

  auto board = pluss::bi::Board("queries").shape(1, 3);
  board = board.add(pluss::bi::Tile::from_view("stored_tile", pluss::bi::LayoutSpec::at(0, 0), stored_view));
  board = board.add(pluss::bi::Tile::from_view("sql_tile", pluss::bi::LayoutSpec::at(0, 1), sql_view));
  board = board.add(pluss::bi::Tile::from_view("traverse_tile", pluss::bi::LayoutSpec::at(0, 2), traverse_view));

  pluss::bi::Studio studio("queries_studio");
  studio.source(nexus).add(board);

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
  const auto nexus = make_bi_nexus();

  const auto view = pluss::bi::View::make(
      "view",
      pluss::bi::QuerySpec::from("loans"),
      pluss::bi::BindSpec::detail_table("Loans"));

  bool duplicate_threw = false;
  try {
    pluss::bi::Studio studio("dup");
    studio.source(nexus).add(
        pluss::bi::Board("board").shape(1, 2)
            .add(pluss::bi::Tile::from_view("tile", pluss::bi::LayoutSpec::at(0, 0), view))
            .add(pluss::bi::Tile::from_view("tile", pluss::bi::LayoutSpec::at(0, 1), view)));
    (void)studio.render();
  } catch (const std::exception& e) {
    duplicate_threw = std::string(e.what()).find("duplicate tile key") != std::string::npos;
  }
  expect(duplicate_threw, "duplicate BI tile keys should be rejected");

  bool bounds_threw = false;
  try {
    pluss::bi::Studio studio("bounds");
    studio.source(nexus).add(
        pluss::bi::Board("board").shape(1, 1).add(
            pluss::bi::Tile::from_view("tile", pluss::bi::LayoutSpec::at(0, 0, 1, 2), view)));
    (void)studio.render();
  } catch (const std::exception& e) {
    bounds_threw = std::string(e.what()).find("column span exceeds board shape") != std::string::npos;
  }
  expect(bounds_threw, "out-of-bounds BI tiles should be rejected");
}

} // namespace

int main() {
  (void)::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 1);
  std::cerr << "test_session_and_controls\n";
  test_session_and_controls();
  std::cerr << "test_source_query_and_projection_after_filters\n";
  test_source_query_and_projection_after_filters();
  std::cerr << "test_scoped_session_filters_skip_or_throw_correctly\n";
  test_scoped_session_filters_skip_or_throw_correctly();
  std::cerr << "test_kpi_and_plot_tiles\n";
  test_kpi_and_plot_tiles();
  std::cerr << "test_pivot_tile\n";
  test_pivot_tile();
  std::cerr << "test_stored_plan_sql_and_traverse_queries\n";
  test_stored_plan_sql_and_traverse_queries();
  std::cerr << "test_board_validation\n";
  test_board_validation();
  std::cerr << "done\n";
  return 0;
}
