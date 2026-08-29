#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::make_note;
using pluss::make_series;
using pluss::make_timeline;
using pluss::table::DataFrame;
using pluss::bundle::Nexus;

const std::filesystem::path kOutDir =
    std::filesystem::temp_directory_path() / "pluss_bi_demo_suite";

void ensure_out_dir() {
  std::error_code ec;
  std::filesystem::create_directories(kOutDir, ec);
  PLUSS_CHECK(!ec, "bi_demo_suite: failed to create output directory");
}

void write_board_svgs(
    const pluss::bi::Studio& studio,
    std::string_view stem_prefix) {
  const auto render = studio.render();
  for (const auto& board : render.boards) {
    PLUSS_CHECK(board.native != nullptr,
                "bi_demo_suite: board render did not produce a native studio");
    const auto path = kOutDir /
                      (std::string(stem_prefix) + "_" + board.key + ".svg");
    board.native->save_svg(path.string());
    std::cout << "saved " << path << "\n";
  }
}

Nexus make_credit_nexus() {
  auto customers = DataFrame({
      {"customer_id", make_series(std::vector<double>{1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008})},
      {"customer_name", make_note({"Ama", "Kojo", "Lina", "Daniel", "Esi", "Mira", "Kweku", "Nora"})},
      {"segment", make_note({"Retail", "Retail", "SME", "Enterprise", "Retail", "SME", "Enterprise", "Retail"})},
      {"region", make_note({"North", "South", "South", "West", "East", "West", "North", "East"})},
      {"gender", make_note({"F", "M", "F", "M", "F", "F", "M", "F"})},
  }).title("customers");

  auto loans = DataFrame({
      {"loan_id", make_series(std::vector<double>{501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512})},
      {"customer_id", make_series(std::vector<double>{1001, 1002, 1003, 1004, 1005, 1002, 1006, 1007, 1008, 1003, 1001, 1005})},
      {"principal", make_series(std::vector<double>{1200, 900, 1500, 4200, 800, 1300, 2600, 3100, 950, 1800, 1100, 1400})},
      {"term_months", make_series(std::vector<double>{12, 6, 18, 24, 6, 12, 18, 24, 9, 12, 6, 15})},
      {"utilization", make_series(std::vector<double>{0.42, 0.58, 0.63, 0.74, 0.31, 0.55, 0.67, 0.71, 0.37, 0.62, 0.47, 0.51})},
      {"risk_band", make_note({"A", "B", "B", "C", "A", "B", "C", "C", "A", "B", "A", "B"})},
      {"opened_at", make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
          "2026-01-09T00:00:00Z",
          "2026-01-10T00:00:00Z",
          "2026-01-11T00:00:00Z",
          "2026-01-12T00:00:00Z",
      })},
  }).title("loans");

  auto payments = DataFrame({
      {"payment_id", make_series(std::vector<double>{9001, 9002, 9003, 9004, 9005, 9006, 9007, 9008, 9009, 9010})},
      {"loan_id", make_series(std::vector<double>{501, 501, 502, 503, 505, 506, 507, 509, 510, 512})},
      {"amount", make_series(std::vector<double>{220, 180, 150, 260, 120, 200, 300, 140, 250, 190})},
      {"paid_at", make_timeline({
          "2026-01-15T00:00:00Z",
          "2026-02-15T00:00:00Z",
          "2026-01-20T00:00:00Z",
          "2026-02-20T00:00:00Z",
          "2026-01-24T00:00:00Z",
          "2026-02-10T00:00:00Z",
          "2026-02-28T00:00:00Z",
          "2026-02-05T00:00:00Z",
          "2026-03-01T00:00:00Z",
          "2026-03-10T00:00:00Z",
      })},
  }).title("payments");

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

pluss::bi::Studio make_overview_studio(const Nexus& nexus) {
  using namespace pluss::bi;

  Studio studio("credit_overview");
  studio.source(nexus);

  auto segment = category("segment_filter");
  segment.feature("segment").label("Segment").place(0, 0);

  auto principal = range("principal_filter");
  principal.feature("principal").label("Principal").place(0, 1);

  auto opened = time_range("opened_filter");
  opened.feature("opened_at").label("Opened").place(0, 2);

  Session session;
  apply(session, segment, std::vector<std::string>{"Retail", "SME"});
  apply(session, principal, 850.0, 2600.0);
  apply_time(
      session,
      opened,
      make_timeline({"2026-01-02T00:00:00Z"}),
      make_timeline({"2026-01-11T00:00:00Z"}));
  studio.session(session);

  auto customer_count = kpi("customer_count");
  customer_count
      .query(from("customers"))
      .feature("customer_id")
      .op(pluss::table::GroupOp::Count)
      .title("Filtered Customers")
      .place(0, 3);

  auto region_mix = bar("region_mix");
  region_mix
      .query(from("customers")
                 .group({"region"})
                 .agg({
                     {"n_customers", "", pluss::table::GroupOp::Count},
                 })
                 .arrange("n_customers", false))
      .x("region")
      .y("n_customers")
      .title("Customers by Region")
      .place(1, 0, 1, 2);

  auto principal_curve = line("principal_curve");
  principal_curve
      .query(from("loans")
                 .select({"opened_at", "principal"}))
      .x("opened_at")
      .y("principal")
      .title("Principal Through Time")
      .place(1, 2, 1, 2);

  auto exposure_scatter = scatter("exposure_scatter");
  exposure_scatter
      .query(from("loans")
                 .select({"term_months", "utilization"}))
      .x("term_months")
      .y("utilization")
      .title("Utilization vs Term")
      .place(2, 0, 1, 2);

  auto loan_heat = heatmap("loan_heat");
  loan_heat
      .query(from("loans")
                 .select({"loan_id", "customer_id", "principal"}))
      .x("loan_id")
      .y("customer_id")
      .z("principal")
      .title("Loan Principal Grid")
      .place(2, 2, 1, 2);

  auto principal_pivot = pivot("principal_pivot");
  principal_pivot
      .query(traverse_from("loans").join("customers"))
      .pivot_index({"region"})
      .pivot_columns({"risk_band"})
      .pivot_values({"principal"})
      .op(pluss::table::GroupOp::Mean)
      .dropna(true)
      .sort(true)
      .title("Mean Principal by Region and Risk")
      .place(3, 0, 1, 2);

  auto joined_table = table("joined_table");
  joined_table
      .query(traverse_from("loans")
                 .join("customers")
                 .select({"loan_id", "customer_name", "segment", "region", "principal", "risk_band", "opened_at"})
                 .arrange("principal", false)
                 .head(8)
                 .named("Credit Slice"))
      .title("Joined Credit Slice")
      .place(3, 2, 1, 2);

  auto corr_view = corr("credit_corr");
  corr_view
      .query(from("loans"))
      .features({"principal", "term_months", "utilization"})
      .title("Numeric Correlations")
      .place(4, 0, 1, 4);

  auto& board = studio.board("overview");
  board.title("Credit Overview");
  board.grid(5, 4);
  board.control(segment);
  board.control(principal);
  board.control(opened);
  board.view(customer_count);
  board.view(region_mix);
  board.view(principal_curve);
  board.view(exposure_scatter);
  board.view(loan_heat);
  board.view(principal_pivot);
  board.view(joined_table);
  board.view(corr_view);
  return studio;
}

pluss::bi::Studio make_workbench_studio(const Nexus& nexus) {
  using namespace pluss::bi;

  Studio studio("credit_workbench");
  studio.source(nexus);

  auto stored_view = table("stored_payments");
  stored_view
      .query(stored("loans_payments")
                 .select({"loan_id", "customer_id", "principal", "amount"})
                 .head(8))
      .title("Stored Plan")
      .place(0, 0);

  auto sql_view = table("sql_view");
  sql_view
      .query(sql(
          "SELECT customers.region, loans.risk_band, AVG(loans.principal) AS avg_principal "
          "FROM loans "
          "INNER JOIN customers ON loans.customer_id = customers.customer_id "
          "GROUP BY customers.region, loans.risk_band "
          "ORDER BY customers.region, loans.risk_band"))
      .title("Nexus SQL")
      .place(0, 1);

  auto traverse_view = table("traverse_view");
  traverse_view
      .query(traverse_from("loans")
                 .join("customers")
                 .select({"loan_id", "customer_name", "region", "segment", "principal"})
                 .arrange("principal", false)
                 .head(8))
      .title("Traverse Join")
      .place(0, 2);

  auto payment_bar = bar("payment_bar");
  payment_bar
      .query(stored("loans_payments")
                 .group({"loan_id"})
                 .agg({
                     {"payment_sum", "amount", pluss::table::GroupOp::Sum},
                 })
                 .arrange("payment_sum", false))
      .x("loan_id")
      .y("payment_sum")
      .title("Payments by Loan")
      .place(1, 0, 1, 2);

  auto payment_curve = line("payment_curve");
  payment_curve
      .query(from("payments")
                 .select({"paid_at", "amount"}))
      .x("paid_at")
      .y("amount")
      .title("Payment Timing")
      .place(1, 2);

  auto& board = studio.board("workbench");
  board.title("Workbench Queries");
  board.grid(2, 3);
  board.view(stored_view);
  board.view(sql_view);
  board.view(traverse_view);
  board.view(payment_bar);
  board.view(payment_curve);
  return studio;
}

void verify_roundtrip(
    const std::filesystem::path& bundle_path,
    const Nexus& nexus) {
  auto restored = pluss::bi::Studio::load(bundle_path.string(), nexus);
  const auto render = restored.render();
  PLUSS_CHECK(!render.boards.empty(),
              "bi_demo_suite: restored studio has no boards");
  PLUSS_CHECK(render.native != nullptr,
              "bi_demo_suite: restored studio did not rebuild native output");
}

int run_suite() {
  ensure_out_dir();

  const auto nexus = make_credit_nexus();
  auto overview = make_overview_studio(nexus);
  auto workbench = make_workbench_studio(nexus);

  const auto overview_bundle = kOutDir / "credit_overview";
  const auto workbench_bundle = kOutDir / "credit_workbench";

  overview.save(overview_bundle.string());
  overview.export_html((kOutDir / "credit_overview").string(),
                       "Credit Overview", 1480, 980);
  overview.export_svg((kOutDir / "credit_overview.svg").string());
  write_board_svgs(overview, "credit_overview");
  verify_roundtrip(overview_bundle.string() + ".plussbi", nexus);

  workbench.save(workbench_bundle.string());
  workbench.export_html((kOutDir / "credit_workbench").string(),
                        "Credit Workbench", 1320, 900);
  workbench.export_svg((kOutDir / "credit_workbench.svg").string());
  write_board_svgs(workbench, "credit_workbench");
  verify_roundtrip(workbench_bundle.string() + ".plussbi", nexus);

  std::cout << "overview_cases="
            << overview.render().boards[0].tiles[9].table->ncases() << "\n";
  std::cout << "output_dir=" << kOutDir << "\n";
  return 0;
}

int show_overview() {
  ensure_out_dir();

  const auto nexus = make_credit_nexus();
  auto overview = make_overview_studio(nexus);

  pluss::view::HtmlWindowOptions options;
  options.title = "PlussBI Credit Overview";
  options.snapshot_name = (kOutDir / "plussview_credit_overview.png").string();
  options.svg_snapshot_name = (kOutDir / "plussview_credit_overview.svg").string();
  options.width = 1480;
  options.height = 980;
  options.backend_preference = pluss::view::BackendPreference::PlussWebViewOnly;

  PLUSS_CHECK(pluss::view::pluss_webview_backend_available(),
              "bi_demo_suite: built-in PlussView backend is not available");

  std::cout << "opening built-in PlussView\n";
  std::cout << "snapshot_png=" << options.snapshot_name << "\n";
  std::cout << "snapshot_svg=" << options.svg_snapshot_name << "\n";
  std::cout << "close the PlussView window when done\n";

  return overview.show(options, true) ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
  const std::string mode = argc > 1 ? argv[1] : "suite";

  if (mode == "suite") {
    return run_suite();
  }
  if (mode == "show") {
    return show_overview();
  }

  std::cerr << "usage: plussmic_bi_demo_suite [suite|show]\n";
  return 1;
}
