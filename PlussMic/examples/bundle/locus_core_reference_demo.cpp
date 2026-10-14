// Real, compiled, run output for the remaining Locus reference pages:
// workspace editing, table metadata, policy/alignment, reports, join plans.
#include "pluss/pluss.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

using pluss::bundle::Locus;
using pluss::bundle::locus::JoinHow;
using pluss::bundle::locus::JoinPlan;
using pluss::bundle::locus::JoinPlanStep;
using pluss::bundle::locus::RelationshipCardinality;
using pluss::bundle::locus::StoreOptions;
using pluss::bundle::locus::TableMeta;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using Streams = std::vector<std::pair<std::string, Stream>>;

Stream nums(std::initializer_list<double> values) {
  std::vector<double> owned(values);
  return Stream(pluss::make_series(owned, {static_cast<std::int64_t>(owned.size())}));
}
Stream words(std::initializer_list<std::string_view> values) {
  std::vector<std::string> owned;
  owned.reserve(values.size());
  for (const auto v : values) owned.emplace_back(v);
  return Stream(pluss::make_note(owned, {static_cast<std::int64_t>(owned.size())}));
}

// customers 1..3; orders 10..13 (order 13 points at customer 9, which does
// not exist); items belong to orders.
Locus make_shop() {
  Catalog customers(Streams{{"id", nums({1, 2, 3})}, {"name", words({"Ada", "Ben", "Cy"})}});
  Catalog orders(Streams{{"order_id", nums({10, 11, 12, 13})},
                         {"cust_id", nums({1, 1, 2, 9})},
                         {"amount", nums({5, 7, 11, 13})}});
  Catalog items(Streams{{"item_order", nums({10, 10, 11, 12, 12})}, {"sku", words({"a", "b", "c", "d", "e"})}});
  return Locus({{"customers", customers}, {"orders", orders}, {"items", items}});
}

Locus make_linked_shop() {
  return make_shop()
      .link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne)
      .link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);
}

void heading(std::string_view title) { std::cout << "== " << title << " ==\n"; }

void print_keys(const Locus& l) {
  std::cout << "keys=[";
  const auto& k = l.keys();
  for (std::size_t i = 0; i < k.size(); ++i) { if (i) std::cout << ", "; std::cout << k[i]; }
  std::cout << "]\n";
}

template <class Fn>
void print_error(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const std::exception& e) {
    std::string msg(e.what());
    const auto nl = msg.find('\n');
    if (nl != std::string::npos) msg.resize(nl);
    std::cout << label << ": " << msg << "\n";
  }
}

} // namespace

int main() {
  std::cout << std::boolalpha;
  namespace locus = pluss::bundle::locus;

  // ===================== head / tail =====================
  heading("head and tail");
  const auto shop = make_shop();
  print_keys(shop.head(2));
  print_keys(shop.tail(1));
  print_keys(shop.head(10));

  // ===================== select / drop =====================
  heading("select and drop");
  print_keys(shop.select({"items", "customers"}));
  print_keys(shop.drop({"orders"}));
  print_error("unknown select", [&] { (void)shop.select({"customers", "nope"}); });

  // ===================== with / with_ =====================
  heading("with and with_");
  const auto replaced = shop.with("items", shop.catalog("customers"));
  std::cout << "replaced items has(name)=" << replaced.catalog("items").has("name")
            << " original items has(sku)=" << shop.catalog("items").has("sku") << "\n";
  const auto appended = shop.with("extra", shop.catalog("items"));
  std::cout << "appended ntables=" << appended.ntables() << "\n";
  auto mutated = shop;
  mutated.with_("orders", shop.catalog("items"));
  std::cout << "with_ changed in place: " << mutated.catalog("orders").has("sku") << "\n";

  // ===================== inject / inject_ =====================
  heading("inject and inject_");
  const auto injected = shop.inject({{"extra", shop.catalog("items")}, {"more", shop.catalog("orders")}});
  std::cout << "injected ntables=" << injected.ntables() << "\n";
  const pluss::kindle::Archive archive(std::vector<Catalog>{shop.catalog("orders"), shop.catalog("items")},
                                       words({"o", "i"}));
  const auto injected_archive = shop.inject(archive);
  std::cout << "injected archive ntables=" << injected_archive.ntables() << "\n";

  // ===================== without / without_ =====================
  heading("without and without_");
  const auto linked = make_linked_shop();
  const auto fewer = linked.without("items");
  std::cout << "without ntables=" << fewer.ntables() << " nrelationships=" << fewer.nrelationships() << "\n";
  std::cout << "original ntables=" << linked.ntables() << " nrelationships=" << linked.nrelationships() << "\n";
  print_error("unknown without", [&] { (void)shop.without("nope"); });

  // ===================== rename / rename_ =====================
  heading("rename and rename_");
  const auto people = linked.rename("customers", "people");
  std::cout << "people has(people)=" << people.has("people") << " has(customers)=" << people.has("customers") << "\n";
  std::cout << "link right_table=" << people.relationship("orders->customers")->right_table << "\n";
  print_error("rename onto existing", [&] { (void)linked.rename("customers", "orders"); });
  print_error("rename to same name", [&] { (void)linked.rename("customers", "customers"); });

  // ===================== apply / apply_ =====================
  heading("apply and apply_");
  const auto trimmed = linked.apply("orders", [](const Catalog& c) { return c.head(2); });
  std::cout << "trimmed orders rows=" << trimmed.catalog("orders").maxrows()
            << " original orders rows=" << linked.catalog("orders").maxrows() << "\n";
  print_error("empty callback", [&] { (void)linked.apply("orders", Locus::CatalogFn{}); });
  print_error("undefined result", [&] { (void)linked.apply("orders", [](const Catalog&) { return Catalog(); }); });

  // ===================== apply_all =====================
  heading("apply_all");
  const auto all_trimmed = linked.apply_all([](const Catalog& c) { return c.head(1); });
  std::cout << "items rows=" << all_trimmed.catalog("items").maxrows()
            << " orders rows=" << all_trimmed.catalog("orders").maxrows() << "\n";

  // ===================== flow =====================
  heading("flow");
  const auto flowed = linked.flow([](const Locus& l) { return l.drop({"items"}); });
  std::cout << "flowed ntables=" << flowed.ntables() << "\n";

  // ===================== table metadata =====================
  heading("table metadata");
  TableMeta meta;
  meta.grain = "one row per customer";
  meta.primary_key = {"id"};
  const auto described = linked.meta("customers", meta);
  std::cout << "described has_table_meta=" << described.has_table_meta("customers")
            << " original has_table_meta=" << linked.has_table_meta("customers") << "\n";
  std::cout << "table_meta primary_key[0]=" << described.table_meta("customers")->primary_key[0] << "\n";
  std::cout << "renamed still has meta=" << described.rename("customers", "people").has_table_meta("people") << "\n";
  std::cout << "dropped loses meta=" << described.drop({"customers"}).has_table_meta("customers") << "\n";
  print_error("meta on unknown table", [&] { (void)linked.meta("ghost", meta); });
  auto meta_inplace = linked;
  meta_inplace.meta_("customers", meta);
  std::cout << "meta_ changed in place=" << meta_inplace.has_table_meta("customers") << "\n";
  std::cout << "set_table_meta == meta: "
            << described.set_table_meta("customers", meta).has_table_meta("customers") << "\n";

  // ===================== policy and alignment =====================
  heading("policy and alignment");
  std::cout << "default_policy().align == policy().align: "
            << (Locus::default_policy().align == linked.policy().align) << "\n";
  pluss::kindle::TabelizePolicy strict;
  strict.align = pluss::kindle::Align::Strict;
  const auto strict_locus = linked.with_policy(strict);
  std::cout << "with_policy new align=Strict: " << (strict_locus.policy().align == pluss::kindle::Align::Strict) << "\n";
  std::cout << "original align unchanged: " << (linked.policy().align == pluss::kindle::Align::Outer) << "\n";
  const auto nx = linked.nexus();
  std::cout << "nexus ntables=" << nx.ntables() << " nexus relationships=" << nx.relationships().size() << "\n";

  heading("alignment_cache_stats");
  auto base = make_linked_shop();
  const auto before = base.alignment_cache_stats();
  std::cout << "before any nexus(): hits=" << before.first << " misses=" << before.second << "\n";
  (void)base.nexus();
  const auto after_first = base.alignment_cache_stats();
  std::cout << "after 1st nexus(): hits=" << after_first.first << " misses=" << after_first.second << "\n";
  (void)base.nexus();
  const auto after_second = base.alignment_cache_stats();
  std::cout << "after 2nd nexus() on the SAME Locus: hits=" << after_second.first
            << " misses=" << after_second.second << "\n";
  // A derived Locus that shares catalogs with base (with_ touches only one of
  // the three) gets its own nexus() -- two of its three catalogs are already
  // cached from base's own alignment, so those two are hits.
  auto derived = base;
  derived.with_("items", base.catalog("items"));
  (void)derived.nexus();
  const auto after_derived = derived.alignment_cache_stats();
  std::cout << "after a derived Locus's nexus(): hits=" << after_derived.first
            << " misses=" << after_derived.second << "\n";

  // ===================== reports =====================
  heading("reports");
  auto report_shop = linked;
  TableMeta rmeta;
  rmeta.primary_key = {"id"};
  report_shop.meta_("customers", rmeta);
  std::cout << "audit_report ncases=" << report_shop.audit_report().ncases() << "\n";
  std::cout << "relationship_audit ncases=" << report_shop.relationship_audit("orders->customers").ncases() << "\n";
  std::cout << "grain_audit ncases=" << report_shop.grain_audit("customers").ncases() << "\n";
  std::cout << "summary ncases=" << report_shop.summary().ncases() << "\n";
  std::cout << "profile ncases=" << report_shop.profile().ncases() << " scan ncases=" << report_shop.scan().ncases() << "\n";
  std::cout << "schema ncases=" << report_shop.schema().ncases() << " graph ncases=" << report_shop.graph().ncases() << "\n";
  std::cout << "deduce ncases=" << report_shop.deduce().ncases() << "\n";

  heading("fingerprint");
  const auto fp = report_shop.fingerprint();
  std::cout << "fingerprint stable: " << (fp == report_shop.fingerprint()) << "\n";
  auto same_shop = make_linked_shop();
  same_shop.meta_("customers", rmeta);
  std::cout << "equal workspaces share fingerprint: " << (fp == same_shop.fingerprint()) << "\n";
  std::cout << "changed data changes fingerprint: "
            << (fp != report_shop.with("customers", report_shop.catalog("orders")).fingerprint()) << "\n";

  // ===================== join plans =====================
  heading("plan and materialize");
  JoinPlan plan;
  plan.root_table = "orders";
  plan.result_name = "orders_with_customers";
  JoinPlanStep step;
  step.left_table = "orders";
  step.right_table = "customers";
  step.spec.keys = {{"cust_id", "id"}};
  step.spec.how = JoinHow::Inner;
  plan.steps.push_back(step);
  const auto direct = linked.materialize(plan);
  std::cout << "materialize rows=" << direct.get("amount")->size() << "\n";
  const auto shortest = linked.plan("items", "customers", JoinHow::Left);
  std::cout << "plan steps=" << shortest.steps.size() << " root_table=" << shortest.root_table << "\n";

  heading("store and run");
  std::cout << "has_store before=" << linked.has_store("by_customer") << "\n";
  const auto stored = linked.store("by_customer", plan);
  std::cout << "has_store after=" << stored.has_store("by_customer")
            << " original has_store=" << linked.has_store("by_customer") << "\n";
  std::cout << "stores()=[";
  const auto names = stored.stores();
  for (std::size_t i = 0; i < names.size(); ++i) { if (i) std::cout << ", "; std::cout << names[i]; }
  std::cout << "]\n";
  std::cout << "store(name) root_table=" << stored.store("by_customer")->root_table << "\n";
  std::cout << "run rows=" << stored.run("by_customer").get("amount")->size() << "\n";
  std::cout << "run with inline plan rows=" << linked.run("anonymous", plan).get("amount")->size() << "\n";
  print_error("run unknown", [&] { (void)linked.run("missing"); });
  const auto renamed_store = stored.rename("customers", "people");
  std::cout << "renamed plan right_table=" << renamed_store.store("by_customer")->steps[0].right_table << "\n";
  std::cout << "renamed plan still runs rows=" << renamed_store.run("by_customer").get("amount")->size() << "\n";
  std::cout << "dropped catalog loses store=" << stored.drop({"customers"}).has_store("by_customer") << "\n";
  auto bad_plan = plan;
  bad_plan.steps[0].right_table = "ghost";
  print_error("store unknown catalog", [&] { (void)linked.store("bad", bad_plan); });

  return 0;
}
