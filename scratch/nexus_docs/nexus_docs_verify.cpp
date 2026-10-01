// Verification harness for the NexusPull / NexusPullAgg / NexusQueryPrepared
// API reference pages (Phase 1). Every block below is labelled and printed
// standalone so its real output can be pasted into the corresponding .md
// page's ### Output fence.
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <set>
#include <vector>

namespace {

using namespace pluss::table;
using namespace pluss::bundle;
using namespace pluss::bundle::nexus;

void print_section(std::string_view title) {
  std::cout << "\n===== " << title << " =====\n";
}

// DataFrame::features() returns a bare std::vector<std::string> (no
// operator<<), so every example needs its own join -- this is that join.
void print_features(const DataFrame& frame) {
  const auto& features = frame.features();
  for (std::size_t i = 0; i < features.size(); ++i) {
    if (i > 0) std::cout << " ";
    std::cout << features[i];
  }
  std::cout << "\n";
}

// Shared dataset, matching pull.md's own example exactly (same table/field
// names) so the NexusPull/NexusPullAgg pages read as a continuation of it,
// not a competing example universe.
Nexus make_workspace() {
  auto customers = DataFrame({
      {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
      {"region_id",   Thread(pluss::make_series({10.0, 20.0, 10.0}))},
  });
  auto regions = DataFrame({
      {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
      {"zone",      Thread(pluss::make_note({"west", "east"}))},
  });
  auto events = DataFrame({
      {"event_id",    Thread(pluss::make_series({100.0, 101.0, 102.0, 103.0, 104.0}))},
      {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0, 2.0, 2.0}))},
      {"occurred_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-20T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-25T00:00:00Z",
          "2026-02-10T00:00:00Z",
      }))},
      {"spend", Thread(pluss::make_series({10.0, 5.0, 2.0, 8.0, 4.0}))},
  });

  TableMeta customer_meta;
  customer_meta.primary_key = {"customer_id"};

  TableMeta event_meta;
  event_meta.time_field = "occurred_at";

  Relationship customer_region;
  customer_region.name = "customer_region";
  customer_region.left_table = "customers";
  customer_region.right_table = "regions";
  customer_region.keys = {{"region_id", "region_id"}};
  customer_region.cardinality = RelationshipCardinality::ManyToOne;

  Relationship customer_events;
  customer_events.name = "customer_events";
  customer_events.left_table = "customers";
  customer_events.right_table = "events";
  customer_events.keys = {{"customer_id", "customer_id"}};
  customer_events.cardinality = RelationshipCardinality::OneToMany;

  return Nexus({
      {"customers", customers},
      {"regions", regions},
      {"events", events},
  }).meta({
      {"customers", customer_meta},
      {"events", event_meta},
  }).links({customer_region, customer_events});
}

} // namespace

int main() {
  auto nx = make_workspace();

  // ---- NexusPull::add ----
  print_section("add: single spec, then a batch of specs");
  {
    auto pulled = nx.pull("customers")
        .add(take("customer_id"))
        .add({take("region_id"), sum("events.spend", "total_spend")})
        .collect();
    print_features(pulled);
    std::cout << pulled.to_string();
  }

  // ---- NexusPull::take (root fields, 3 forms of the same call) ----
  print_section("take: brace literal (initializer_list)");
  {
    auto pulled = nx.pull("customers").take({"customer_id", "region_id"}).collect();
    print_features(pulled);
  }
  print_section("take: named std::vector<std::string_view> (RangeView)");
  {
    std::vector<std::string_view> fields = {"customer_id", "region_id"};
    auto pulled = nx.pull("customers").take(fields).collect();
    print_features(pulled);
  }
  print_section("take: named std::vector<std::string> (templated NameRange)");
  {
    std::vector<std::string> fields = {"customer_id", "region_id"};
    auto pulled = nx.pull("customers").take(fields).collect();
    print_features(pulled);
  }
  print_section("take: single field with explicit alias");
  {
    auto pulled = nx.pull("customers").take("customer_id", "cust_id").collect();
    print_features(pulled);
  }

  // ---- NexusPull::take (unscoped root rename pairs) ----
  // NOTE: a literal double-brace list here -- .take({{"a","b"},{"c","d"}}) --
  // is AMBIGUOUS: the outer braces could target either
  // take(initializer_list<string_view>) or
  // take(initializer_list<pair<string_view,string_view>>), and the compiler
  // rejects it rather than guessing. Unlike the table-scoped take(table,
  // {{...}}) form (unambiguous, since scoping picks a distinct 2-argument
  // overload), the unscoped rename form must be passed as a named
  // container instead of a literal brace list.
  print_section("take: unscoped pairs via named std::vector<std::pair<string_view,string_view>>");
  {
    std::vector<std::pair<std::string_view, std::string_view>> fields = {
        {"customer_id", "cust_id"}, {"region_id", "reg_id"}};
    auto pulled = nx.pull("customers").take(fields).collect();
    print_features(pulled);
  }
  print_section("take: unscoped pairs via std::map<std::string, std::string>");
  {
    std::map<std::string, std::string> fields = {
        {"customer_id", "cust_id"}, {"region_id", "reg_id"}};
    auto pulled = nx.pull("customers").take(fields).collect();
    print_features(pulled);
  }
  print_section("take: unscoped pairs via std::unordered_map<std::string, std::string>");
  {
    std::unordered_map<std::string, std::string> fields = {
        {"customer_id", "cust_id"}};
    auto pulled = nx.pull("customers").take(fields).collect();
    print_features(pulled);
  }

  // ---- NexusPull::take (table-scoped related-field pairs) ----
  print_section("take(table, pairs): brace literal");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .take("regions", {{"zone", "zone"}})
        .collect();
    print_features(pulled);
    std::cout << pulled.to_string();
  }
  print_section("take(table, pairs): named std::vector<std::pair<string_view,string_view>>");
  {
    std::vector<std::pair<std::string_view, std::string_view>> fields = {{"zone", "region_zone"}};
    auto pulled = nx.pull("customers").take("regions", fields).collect();
    print_features(pulled);
  }
  print_section("take(table, pairs): std::map<std::string, std::string>");
  {
    std::map<std::string, std::string> fields = {{"zone", "region_zone"}};
    auto pulled = nx.pull("customers").take("regions", fields).collect();
    print_features(pulled);
  }
  print_section("take(table, pairs): std::unordered_map<std::string, std::string>");
  {
    std::unordered_map<std::string, std::string> fields = {{"zone", "region_zone"}};
    auto pulled = nx.pull("customers").take("regions", fields).collect();
    print_features(pulled);
  }

  // ---- NexusPull direct reducers (count/ndistinct/sum/mean/median/min/max/std/var) ----
  print_section("direct reducers on NexusPull: sum, mean, median, min, max, std, var, count, ndistinct");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .sum("events.spend", "total_spend")
        .mean("events.spend", "mean_spend")
        .median("events.spend", "median_spend")
        .min("events.spend", "min_spend")
        .max("events.spend", "max_spend")
        .std("events.spend", "std_spend")
        .var("events.spend", "var_spend")
        .count("events.event_id", "event_count")
        .ndistinct("events.event_id", "distinct_events")
        .collect();
    print_features(pulled);
    std::cout << pulled.to_string();
  }
  print_section("direct reducer without an explicit alias (auto-aliased)");
  {
    auto pulled = nx.pull("customers").take("customer_id").sum("events.spend").collect();
    print_features(pulled);
  }

  // ---- NexusPull::agg / NexusPullAgg reducers + end() ----
  print_section("agg: several reducers scoped under one table token, then end()");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .agg("events")
            .count("event_id", "event_count")
            .sum("spend", "total_spend")
            .mean("spend", "mean_spend")
            .median("spend", "median_spend")
            .min("spend", "min_spend")
            .max("spend", "max_spend")
            .std("spend", "std_spend")
            .var("spend", "var_spend")
            .ndistinct("event_id", "distinct_events")
        .end()
        .collect();
    print_features(pulled);
    std::cout << pulled.to_string();
  }
  print_section("agg: batched (feature, alias) pairs in one call, brace literal");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .agg("events")
            .sum({{"spend", "total_spend"}, {"event_id", "total_events_sum"}})
        .end()
        .collect();
    print_features(pulled);
  }
  print_section("agg: batched pairs via named std::vector<std::pair<string_view,string_view>>");
  {
    std::vector<std::pair<std::string_view, std::string_view>> fields = {{"spend", "total_spend"}};
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .agg("events").sum(fields).end()
        .collect();
    print_features(pulled);
  }
  print_section("agg: end() returns the SAME parent NexusPull -- chaining continues normally");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .agg("events").sum("spend", "total_spend").end()
        .take("region_id")
        .collect();
    print_features(pulled);
  }

  // ---- NexusPull::root / specs / collect ----
  print_section("root(): reports the anchor table");
  {
    auto builder = nx.pull("customers").take("customer_id");
    std::cout << builder.root() << "\n";
  }
  print_section("specs(): inspect the accumulated spec list before collecting");
  {
    auto builder = nx.pull("customers").take("customer_id").sum("events.spend", "total_spend");
    std::cout << builder.specs().size() << " specs\n";
    for (const auto& spec : builder.specs()) {
      std::cout << "  source=" << spec.source
                << " op=" << static_cast<int>(spec.op)
                << " alias=" << (spec.alias.has_value() ? *spec.alias : std::string("<none>"))
                << "\n";
    }
  }
  print_section("collect(): executes and materializes the DataFrame");
  {
    auto pulled = nx.pull("customers").take("customer_id").sum("events.spend", "total_spend").collect();
    std::cout << pulled.ncases() << " rows, " << pulled.features().size() << " cols\n";
  }

  // ---- Edge cases ----
  print_section("edge case: agg() on an empty table name throws");
  try {
    auto bad = nx.pull("customers").agg("");
    (void)bad;
  } catch (const std::exception& e) {
    std::string msg(e.what());
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  print_section("edge case: an aggregate on root itself (no declared unrelated table) still qualifies via dotted source, but a truly empty source throws");
  try {
    auto bad = nx.pull("customers").sum("", "bad");
    (void)bad;
  } catch (const std::exception& e) {
    std::string msg(e.what());
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  // ---- NexusQuery::execute (real output for execute.md/prepare.md retrofit) ----
  print_section("NexusQuery::execute: join across workspace tables");
  {
    auto customers2 = DataFrame({
        {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
        {"name", pluss::make_note({"Ada", "Bola", "Chen"})},
    });
    auto orders2 = DataFrame({
        {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
        {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
        {"amount", pluss::make_series({50.0, 75.0, 20.0})},
    });
    auto nx2 = Nexus({{"customers", customers2}, {"orders", orders2}});
    auto out = nx2.query().execute(
        "SELECT customers.customer_id, orders.amount "
        "FROM customers "
        "JOIN orders ON customers.customer_id = orders.customer_id "
        "ORDER BY orders.amount DESC");
    std::cout << out.to_string();
  }
  print_section("NexusQuery::execute: empty result set");
  {
    auto customers2 = DataFrame({
        {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    });
    auto nx2 = Nexus({{"customers", customers2}});
    auto out = nx2.query().execute("SELECT customer_id FROM customers WHERE customer_id > 100");
    std::cout << out.ncases() << " rows\n";
  }
  print_section("NexusQuery::execute: malformed SQL throws");
  try {
    auto customers2 = DataFrame({{"customer_id", pluss::make_series({1.0})}});
    auto nx2 = Nexus({{"customers", customers2}});
    auto out = nx2.query().execute("SELEKT customer_id FROM customers");
    (void)out;
  } catch (const std::exception& e) {
    std::string msg(e.what());
    std::cout << msg.substr(0, msg.find('\n')) << "\n";
  }
  print_section("NexusQuery::prepare + Example: value-bound reused statement");
  {
    auto orders3 = DataFrame({
        {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
        {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
        {"amount", pluss::make_series({50.0, 75.0, 20.0})},
    });
    auto nx3 = Nexus({{"orders", orders3}});
    auto stmt = nx3.query().prepare(
        "SELECT orders.amount "
        "FROM orders "
        "WHERE orders.amount >= {min_amount} "
        "ORDER BY orders.amount DESC");
    auto out = stmt.execute({{"min_amount", 50.0}});
    std::cout << out.to_string();
  }

  // ---- NexusQueryPrepared::execute ----
  print_section("NexusQueryPrepared::execute(): no-arg re-execution");
  {
    auto stmt = nx.query().prepare("SELECT customer_id FROM customers ORDER BY customer_id");
    auto out = stmt.execute();
    print_features(out);
    std::cout << out.to_string();
  }
  print_section("NexusQueryPrepared::execute(params): value substitution, run twice with different params");
  {
    auto stmt = nx.query().prepare(
        "SELECT customer_id, region_id FROM customers WHERE region_id = {rid} ORDER BY customer_id");
    auto west = stmt.execute({{"rid", 10.0}});
    auto east = stmt.execute({{"rid", 20.0}});
    std::cout << "region 10: " << west.ncases() << " rows\n";
    std::cout << "region 20: " << east.ncases() << " rows\n";
    std::cout << west.to_string();
  }
  print_section("NexusQueryPrepared::execute(params, identifier_params): identifier substitution");
  {
    auto stmt = nx.query().prepare("SELECT {@col} FROM customers ORDER BY customer_id");
    auto out = stmt.execute({}, {{"col", "customer_id"}});
    print_features(out);
  }

  // ==================== Phase 2 retrofit examples ====================
  // Small self-contained 4-table workspace matching select.md/drop.md's
  // existing Example 1 (customers/orders/payments/refunds), reused for
  // the RangeView/NameRange sibling-overload retrofits.
  auto make_ordering_workspace = []() {
    auto customers = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    });
    auto payments = DataFrame({
        {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
        {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
    });
    auto refunds = DataFrame({
        {"refund_id", Thread(pluss::make_series({77.0}))},
        {"order_id",  Thread(pluss::make_series({102.0}))},
    });
    return Nexus({
        {"customers", customers},
        {"orders", orders},
        {"payments", payments},
        {"refunds", refunds},
    });
  };

  auto print_keys = [](const Nexus& biz) {
    for (std::int64_t i = 0; i < biz.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << biz.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  };

  print_section("Retrofit: select via named std::vector<std::string_view> (RangeView)");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> keep = {"payments", "customers"};
    auto out = biz.select(keep);
    print_keys(out);
  }
  print_section("Retrofit: select via named std::vector<std::string> (templated NameRange)");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string> keep = {"payments", "customers"};
    auto out = biz.select(keep);
    print_keys(out);
  }
  print_section("Retrofit: select via std::set<std::string_view> (NameRange, sorted order)");
  {
    auto biz = make_ordering_workspace();
    std::set<std::string_view> keep = {"payments", "customers"};
    auto out = biz.select(keep);
    print_keys(out);
  }

  print_section("Retrofit: drop via named std::vector<std::string_view> (RangeView)");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> remove = {"refunds", "payments"};
    auto out = biz.drop(remove);
    print_keys(out);
  }
  print_section("Retrofit: drop via named std::vector<std::string> (templated NameRange)");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string> remove = {"refunds", "payments"};
    auto out = biz.drop(remove);
    print_keys(out);
  }

  print_section("Retrofit: validate_keys via named containers");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> as_vector = {"order_id"};
    std::vector<std::string> as_owned = {"order_id"};
    std::cout << std::boolalpha
              << biz.validate_keys("orders", as_vector) << " "
              << biz.validate_keys("orders", as_owned) << "\n";
  }

  print_section("Retrofit: detect_duplicates via named containers");
  {
    auto dup_orders = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    });
    auto biz = Nexus({{"orders", dup_orders}});
    std::vector<std::string_view> as_vector = {"order_id"};
    std::vector<std::string> as_owned = {"order_id"};
    auto a = biz.detect_duplicates("orders", as_vector);
    auto b = biz.detect_duplicates("orders", as_owned);
    std::cout << a.ncases() << " " << b.ncases() << " duplicate rows\n";
  }

  print_section("Retrofit: duplicates (alias) via named containers");
  {
    auto dup_orders = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    });
    auto biz = Nexus({{"orders", dup_orders}});
    std::vector<std::string_view> as_vector = {"order_id"};
    auto a = biz.duplicates("orders", as_vector);
    std::cout << a.ncases() << " duplicate rows\n";
  }

  print_section("Retrofit: Nexus::pull string-fields overload via named containers");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> as_vector = {"customer_id", "name"};
    std::vector<std::string> as_owned = {"customer_id", "name"};
    auto a = biz.pull("customers", as_vector);
    auto b = biz.pull("customers", as_owned);
    print_features(a);
    print_features(b);
  }

  print_section("Retrofit: to_atlas via named containers");
  {
    // to_atlas stacks frames into a cube, so every selected frame needs
    // the SAME row count AND the same column names/order -- two
    // same-shaped snapshots of "customers", not the mixed-schema
    // customers/orders/payments/refunds workspace.
    auto customers_q1 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto customers_q2 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto biz = Nexus({{"customers_q1", customers_q1}, {"customers_q2", customers_q2}});

    std::vector<std::string_view> as_vector = {"customers_q1", "customers_q2"};
    std::vector<std::string> as_owned = {"customers_q1", "customers_q2"};
    auto a = biz.to_atlas(as_vector);
    auto b = biz.to_atlas(as_owned);
    std::cout << a.depth() << " " << b.depth() << " frames\n";
  }

  print_section("Retrofit: NexusTraverse::pick via named containers");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> as_vector = {"customer_id", "name"};
    std::vector<std::string> as_owned = {"customer_id", "name"};
    auto a = biz.from("customers").pick(as_vector).collect();
    auto b = biz.from("customers").pick(as_owned).collect();
    print_features(a);
    print_features(b);
  }

  print_section("Retrofit: NexusTraverse::drop via named containers");
  {
    auto biz = make_ordering_workspace();
    std::vector<std::string_view> as_vector = {"name"};
    std::vector<std::string> as_owned = {"name"};
    auto a = biz.from("customers").drop(as_vector).collect();
    auto b = biz.from("customers").drop(as_owned).collect();
    print_features(a);
    print_features(b);
  }

  // ---- pull.md retrofit: verify the 3 examples with a broken print call ----
  print_section("pull.md Fluent Builder Example (windowed add + agg chain)");
  {
    auto pulled = nx.pull("customers")
        .take("customer_id")
        .take("regions", {{"zone", "zone"}})
        .agg("events")
            .count("event_id", "event_count")
            .sum("spend", "total_spend")
            .median("spend", "median_spend")
        .end()
        .add({sum("events.spend", "recent_spend").windowed(30, "d")})
        .collect();
    print_features(pulled);
  }
  print_section("pull.md Batched Maps Example");
  {
    auto customers2 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
        {"region_id",   Thread(pluss::make_series({10.0, 20.0}))},
    });
    auto regions2 = DataFrame({
        {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
        {"zone",      Thread(pluss::make_note({"west", "east"}))},
    });
    auto events2 = DataFrame({
        {"event_id",    Thread(pluss::make_series({100.0, 101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
        {"spend",       Thread(pluss::make_series({10.0, 5.0, 8.0}))},
        {"occurred_at", Thread(pluss::make_timeline({
            "2026-01-01T00:00:00Z", "2026-01-20T00:00:00Z", "2026-01-25T00:00:00Z",
        }))},
    });
    TableMeta customer_meta2;
    customer_meta2.primary_key = {"customer_id"};
    TableMeta event_meta2;
    event_meta2.time_field = "occurred_at";
    Relationship customer_region2;
    customer_region2.name = "customer_region";
    customer_region2.left_table = "customers";
    customer_region2.right_table = "regions";
    customer_region2.keys = {{"region_id", "region_id"}};
    customer_region2.cardinality = RelationshipCardinality::ManyToOne;
    Relationship customer_events2;
    customer_events2.name = "customer_events";
    customer_events2.left_table = "customers";
    customer_events2.right_table = "events";
    customer_events2.keys = {{"customer_id", "customer_id"}};
    customer_events2.cardinality = RelationshipCardinality::OneToMany;

    std::unordered_map<std::string, TableMeta> meta_batch;
    meta_batch.emplace("customers", customer_meta2);
    meta_batch.emplace("events", event_meta2);

    auto nx2 = Nexus({
        {"customers", customers2}, {"regions", regions2}, {"events", events2},
    }).meta(meta_batch).links({customer_region2, customer_events2});

    auto pulled = nx2.pull("customers")
        .take(std::map<std::string, std::string>{{"customer_id", "customer_id"}})
        .take("regions", std::map<std::string, std::string>{{"zone", "zone"}})
        .agg("events")
            .count(std::map<std::string, std::string>{
                {"event_id", "event_count"},
                {"occurred_at", "dated_event_count"},
            })
            .sum(std::unordered_map<std::string, std::string>{{"spend", "total_spend"}})
        .end()
        .collect();
    print_features(pulled);
  }
  print_section("pull.md Range-Friendly Root Lists Example");
  {
    auto customers3 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
        {"region_id",   Thread(pluss::make_series({10.0, 20.0}))},
    });
    auto regions3 = DataFrame({
        {"region_id", Thread(pluss::make_series({10.0, 20.0}))},
        {"zone",      Thread(pluss::make_note({"west", "east"}))},
    });
    Relationship rel3;
    rel3.name = "customer_region";
    rel3.left_table = "customers";
    rel3.right_table = "regions";
    rel3.keys = {{"region_id", "region_id"}};
    rel3.cardinality = RelationshipCardinality::ManyToOne;

    std::vector<std::string_view> root_fields = {"customer_id", "region_id"};
    auto nx3 = Nexus({{"customers", customers3}, {"regions", regions3}}).links({rel3});
    auto pulled = nx3.pull("customers")
        .take(root_fields)
        .take("regions", {{"zone", "zone"}})
        .collect();
    print_features(pulled);
  }

  // ---- pull.md retrofit: RangeView/NameRange forms of pull(root, fields) ----
  print_section("pull.md Example 11: pull(root, fields) via named containers");
  {
    auto customers4 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders4 = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0, 104.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0, 1.0}))},
        {"amount",      Thread(pluss::make_series({50.0, 75.0, 20.0, 30.0}))},
    });
    auto biz4 = Nexus({{"customers", customers4}, {"orders", orders4}});
    TableMeta customers_meta4;
    customers_meta4.primary_key = {"customer_id"};
    auto tagged4 = biz4.set_table_meta("customers", customers_meta4);
    Relationship rel4;
    rel4.name = "orders_to_customers";
    rel4.left_table = "orders";
    rel4.right_table = "customers";
    rel4.keys = {{"customer_id", "customer_id"}};
    rel4.cardinality = RelationshipCardinality::ManyToOne;
    auto linked4 = tagged4.add_relationship(rel4);

    std::vector<std::string_view> as_view = {"name"};
    std::vector<std::string> as_owned = {"name"};
    auto p_view = linked4.pull("customers", as_view);
    auto p_owned = linked4.pull("customers", as_owned);
    print_features(p_view);
    print_features(p_owned);
  }

  // ---- NexusTraverse pick.md/drop.md retrofit ----
  print_section("pick.md/drop.md retrofit dataset via named containers");
  {
    auto customers5 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders5 = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
        {"amount",      Thread(pluss::make_series({50.0, 75.0, 20.0}))},
    });
    auto biz5 = Nexus({{"customers", customers5}, {"orders", orders5}});
    Relationship rel5;
    rel5.name = "orders_to_customers";
    rel5.left_table = "orders";
    rel5.right_table = "customers";
    rel5.keys = {{"customer_id", "customer_id"}};
    auto linked5 = biz5.add_relationship(rel5);

    std::vector<std::string_view> pick_view = {"order_id", "name"};
    std::vector<std::string> pick_owned = {"order_id", "name"};
    auto r1 = linked5.from("orders").join("customers").pick(pick_view).collect();
    auto r2 = linked5.from("orders").join("customers").pick(pick_owned).collect();
    print_features(r1);
    print_features(r2);

    std::vector<std::string_view> drop_view = {"customer_id"};
    std::vector<std::string> drop_owned = {"customer_id"};
    auto r3 = linked5.from("orders").join("customers").drop(drop_view).collect();
    auto r4 = linked5.from("orders").join("customers").drop(drop_owned).collect();
    print_features(r3);
    print_features(r4);
  }

  // ---- Verify the 7 pre-existing print-bug fixes' claimed output ----
  print_section("head.md Example 3: head(99) on a 4-table workspace");
  {
    auto customers6 = DataFrame({{"customer_id", Thread(pluss::make_series({1.0}))}});
    auto orders6 = DataFrame({{"order_id", Thread(pluss::make_series({1.0}))}});
    auto payments6 = DataFrame({{"payment_id", Thread(pluss::make_series({1.0}))}});
    auto refunds6 = DataFrame({{"refund_id", Thread(pluss::make_series({1.0}))}});
    auto biz6 = Nexus({
        {"customers", customers6}, {"orders", orders6},
        {"payments", payments6}, {"refunds", refunds6},
    });
    auto out = biz6.head(99);
    for (std::int64_t i = 0; i < out.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << out.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  }

  print_section("tail.md Example 3: tail(99) on the same 4-table workspace");
  {
    auto customers6 = DataFrame({{"customer_id", Thread(pluss::make_series({1.0}))}});
    auto orders6 = DataFrame({{"order_id", Thread(pluss::make_series({1.0}))}});
    auto payments6 = DataFrame({{"payment_id", Thread(pluss::make_series({1.0}))}});
    auto refunds6 = DataFrame({{"refund_id", Thread(pluss::make_series({1.0}))}});
    auto biz6 = Nexus({
        {"customers", customers6}, {"orders", orders6},
        {"payments", payments6}, {"refunds", refunds6},
    });
    auto out = biz6.tail(99);
    for (std::int64_t i = 0; i < out.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << out.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  }

  print_section("inject.md Example 2: inject several named tables");
  {
    auto customers7 = DataFrame({{"customer_id", Thread(pluss::make_series({1.0}))}});
    auto orders7 = DataFrame({{"order_id", Thread(pluss::make_series({1.0}))}});
    auto payments7 = DataFrame({{"payment_id", Thread(pluss::make_series({1.0}))}});
    auto refunds7 = DataFrame({{"refund_id", Thread(pluss::make_series({1.0}))}});
    auto biz7 = Nexus({{"customers", customers7}});
    auto out = biz7.inject({
        {"orders", orders7}, {"payments", payments7}, {"refunds", refunds7},
    });
    for (std::int64_t i = 0; i < out.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << out.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  }

  print_section("load.md Example 5: invalid mixed load/ignore throws");
  try {
    auto opts = LoadOptions{};
    opts.load = {"account"};
    opts.ignore = {"loan"};
    auto out = Nexus::load("/tmp/bank_raw_verify_real", opts);
    (void)out;
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
  }

  print_section("rename.md Example 2: rename then inject unnamed table");
  {
    auto customers8 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders8 = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    });
    auto payments8 = DataFrame({
        {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
        {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
    });
    auto refunds8 = DataFrame({
        {"refund_id", Thread(pluss::make_series({77.0}))},
        {"order_id",  Thread(pluss::make_series({102.0}))},
    });
    auto biz8 = Nexus({
        {"customers", customers8}, {"orders", orders8},
        {"payments", payments8}, {"refunds", refunds8},
    });
    auto shipments = DataFrame({
        {"shipment_id", Thread(pluss::make_series({501.0, 502.0}))},
        {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    });
    auto out = biz8.rename("orders", "table_9").inject(shipments);
    for (std::int64_t i = 0; i < out.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << out.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  }

  print_section("show.md Example 1: biz.show() direct stdout write");
  {
    auto customers9 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto biz9 = Nexus({{"customers", customers9}});
    biz9.show();
  }

  print_section("flow.md Example 2: flow() composing rename+head");
  {
    auto customers10 = DataFrame({{"customer_id", Thread(pluss::make_series({1.0}))}});
    auto orders10 = DataFrame({{"order_id", Thread(pluss::make_series({1.0}))}});
    auto payments10 = DataFrame({{"payment_id", Thread(pluss::make_series({1.0}))}});
    auto biz10 = Nexus({
        {"customers", customers10}, {"orders", orders10}, {"payments", payments10},
    });
    auto out = biz10.flow([](const Nexus& value) {
      return value.rename("orders", "purchases").head(3);
    });
    for (std::int64_t i = 0; i < out.ntables(); ++i) {
      if (i > 0) std::cout << ", ";
      std::cout << out.keys()[static_cast<std::size_t>(i)];
    }
    std::cout << "\n";
  }

  // ---- apply_.md content ----
  print_section("apply_ in-place mutation");
  {
    auto customers11 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders11 = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    });
    auto biz11 = Nexus({{"customers", customers11}, {"orders", orders11}});
    std::cout << "before: " << biz11.frame("orders").ncases() << " rows\n";
    biz11.apply_("orders", [](const DataFrame& frame) {
      return frame.head(1, false);
    });
    std::cout << "after: " << biz11.frame("orders").ncases() << " rows\n";
  }

  // ---- add_relationship.md: add_relationships_ (plural, mutating) ----
  print_section("add_relationships_ (plural, mutating)");
  {
    auto customers12 = DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
    });
    auto orders12 = DataFrame({
        {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    });
    auto biz12 = Nexus({{"customers", customers12}, {"orders", orders12}});
    Relationship rel12;
    rel12.name = "orders_to_customers";
    rel12.left_table = "orders";
    rel12.right_table = "customers";
    rel12.keys = {{"customer_id", "customer_id"}};
    Relationship second12;
    second12.name = "orders_to_customers_2";
    second12.left_table = "orders";
    second12.right_table = "customers";
    second12.keys = {{"customer_id", "customer_id"}};

    auto mutable_biz = biz12;
    mutable_biz.add_relationships_({rel12, second12});
    std::cout << biz12.nrelationships() << " " << mutable_biz.nrelationships();
  }

  // ---- to_string.md: operator<< vs output_render vs show(os) ----
  print_section("operator<< / output_render / show(os) equivalence");
  {
    auto customers13 = DataFrame({{"customer_id", Thread(pluss::make_series({1.0, 2.0}))}});
    auto biz13 = Nexus({{"customers", customers13}});
    std::ostringstream a, b, c;
    a << biz13;
    pluss::bundle::output_render(b, biz13);
    biz13.show(c);
    std::cout << std::boolalpha
              << (a.str() == b.str()) << " " << (b.str() == c.str()) << "\n";
  }

  // ==================== load.md: self-contained fixture examples ====================
  namespace fs = std::filesystem;

  print_section("load.md Example 1: load every supported top-level file");
  {
    fs::remove_all("/tmp/pluss_nexus_load_ex1");
    fs::create_directories("/tmp/pluss_nexus_load_ex1");

    DataFrame({
        {"account_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"balance",    Thread(pluss::make_series({100.0, 250.0, 75.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex1/account.csv");

    DataFrame({
        {"district_id", Thread(pluss::make_series({10.0, 20.0}))},
        {"name",        Thread(pluss::make_note({"North", "South"}))},
    }).save_json("/tmp/pluss_nexus_load_ex1/district.json");

    DataFrame({
        {"trans_id",   Thread(pluss::make_series({1000.0, 1001.0}))},
        {"account_id", Thread(pluss::make_series({1.0, 2.0}))},
        {"amount",     Thread(pluss::make_series({50.0, -20.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex1/trans.csv");

    auto nx = Nexus::load("/tmp/pluss_nexus_load_ex1");
    std::cout << nx.ntables() << "\n";
    std::cout << std::boolalpha << nx.has("account") << " "
              << nx.has("district") << " "
              << nx.has("trans") << "\n";
  }

  print_section("load.md Example 2: load only the files you name");
  {
    fs::remove_all("/tmp/pluss_nexus_load_ex2");
    fs::create_directories("/tmp/pluss_nexus_load_ex2");

    DataFrame({
        {"account_id", Thread(pluss::make_series({1.0, 2.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex2/account.csv");

    DataFrame({
        {"district_id", Thread(pluss::make_series({10.0}))},
    }).save_json("/tmp/pluss_nexus_load_ex2/district.json");

    DataFrame({
        {"loan_id",    Thread(pluss::make_series({500.0}))},
        {"account_id", Thread(pluss::make_series({1.0}))},
    }).save_json("/tmp/pluss_nexus_load_ex2/loan.json");

    LoadOptions options;
    options.load = {"account", "loan.json"};
    options.formats = {"csv", "json"};

    auto nx = Nexus::load("/tmp/pluss_nexus_load_ex2", options);
    std::cout << nx.ntables() << "\n";
    std::cout << nx.keys().at(0) << " " << nx.keys().at(1) << "\n";
  }

  print_section("load.md Example 3: ignore a few tables but load the rest");
  {
    fs::remove_all("/tmp/pluss_nexus_load_ex3");
    fs::create_directories("/tmp/pluss_nexus_load_ex3");

    DataFrame({
        {"account_id", Thread(pluss::make_series({1.0, 2.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex3/account.csv");

    DataFrame({
        {"trans_id", Thread(pluss::make_series({1.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex3/trans.csv");

    DataFrame({
        {"order_id", Thread(pluss::make_series({1.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex3/order.csv");

    LoadOptions options;
    options.ignore = {"trans", "order"};

    auto nx = Nexus::load("/tmp/pluss_nexus_load_ex3", options);
    std::cout << nx.ntables() << "\n";
    std::cout << std::boolalpha << nx.has("trans") << " " << nx.has("account") << "\n";
  }

  print_section("load.md Example 4: recursive scan plus stable file-name keys");
  {
    fs::remove_all("/tmp/pluss_nexus_load_ex4");
    fs::create_directories("/tmp/pluss_nexus_load_ex4/raw");

    DataFrame({
        {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
        {"name",        Thread(pluss::make_note({"Ada", "Ben"}))},
    }).save_csv("/tmp/pluss_nexus_load_ex4/raw/customers.csv");

    DataFrame({
        {"event_id",    Thread(pluss::make_series({1.0, 2.0, 3.0}))},
        {"customer_id", Thread(pluss::make_series({1.0, 1.0, 2.0}))},
    }).save_parquet("/tmp/pluss_nexus_load_ex4/raw/events.parquet");

    LoadOptions options;
    options.recursive = true;
    options.keep_extension = true;

    auto nx = Nexus::load("/tmp/pluss_nexus_load_ex4", options);
    std::cout << std::boolalpha << nx.has("customers.csv") << " "
              << nx.has("events.parquet") << "\n";
  }

  print_section("load.md Example 5: invalid mixed selection mode");
  {
    fs::remove_all("/tmp/pluss_nexus_load_ex5");
    fs::create_directories("/tmp/pluss_nexus_load_ex5");
    DataFrame({
        {"account_id", Thread(pluss::make_series({1.0}))},
    }).save_csv("/tmp/pluss_nexus_load_ex5/account.csv");

    LoadOptions options;
    options.load = {"account"};
    options.ignore = {"loan"};

    try {
      auto nx = Nexus::load("/tmp/pluss_nexus_load_ex5", options);
      (void)nx;
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }

  return 0;
}
