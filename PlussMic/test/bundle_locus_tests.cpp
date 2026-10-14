#include <cmath>
#include <cstdint>
#include <exception>
#include <atomic>
#include <climits>
#include <cmath>
#include <filesystem>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::bundle::Locus;
using pluss::bundle::locus::JoinHow;
using pluss::bundle::locus::JoinPath;
using pluss::bundle::locus::JoinSpec;
using pluss::bundle::locus::Relationship;
using pluss::bundle::locus::RelationshipCardinality;
using pluss::bundle::locus::RelationshipValidationStatus;
using pluss::kindle::Align;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using pluss::kindle::TabelizePolicy;

[[noreturn]] void fail(std::string_view msg) { throw std::runtime_error(std::string(msg)); }

void expect(bool condition, std::string_view msg) {
  if (!condition) fail(msg);
}

template <class Fn>
void expect_throw(Fn&& fn, std::string_view msg) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  if (!threw) fail(msg);
}

using Streams = std::vector<std::pair<std::string, Stream>>;

Stream nums(const std::vector<double>& v) {
  return Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
}

Stream words(const std::vector<std::string>& v) {
  return Stream(pluss::make_note(v, {static_cast<std::int64_t>(v.size())}));
}

std::vector<double> values_of(const Catalog& c, std::string_view key) {
  const auto s = c.get(key);
  expect(s.has_value(), "result catalog should contain the requested stream");
  return s->as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
}

bool has_key(const Catalog& c, std::string_view key) { return c.has(key); }

// customers 1..3; orders 10..13 (order 13 points at a customer that does not exist);
// items belong to orders.
Locus make_shop() {
  Catalog customers(Streams{
      {"id", nums({1.0, 2.0, 3.0})},
      {"name", words({"Ada", "Ben", "Cy"})},
  });
  Catalog orders(Streams{
      {"order_id", nums({10.0, 11.0, 12.0, 13.0})},
      {"cust_id", nums({1.0, 1.0, 2.0, 9.0})},
      {"amount", nums({5.0, 7.0, 11.0, 13.0})},
  });
  Catalog items(Streams{
      {"item_order", nums({10.0, 10.0, 11.0, 12.0, 12.0})},
      {"sku", words({"a", "b", "c", "d", "e"})},
  });
  return Locus({{"customers", customers}, {"orders", orders}, {"items", items}});
}

void test_construction_and_selection() {
  const auto shop = make_shop();
  expect(shop.defined() && !shop.empty(), "a built Locus is defined and non-empty");
  expect(shop.ntables() == 3, "ntables should count catalogs");
  expect(shop.nrelationships() == 0, "a new Locus has no links");
  expect(shop.shape() == std::make_pair<std::int64_t, std::int64_t>(3, 0), "shape is {tables, links}");
  expect(shop.keys() == std::vector<std::string>({"customers", "orders", "items"}),
         "keys keep construction order");
  expect(shop.has("orders") && !shop.has("nope"), "has() looks catalogs up by name");
  expect(shop.catalog("orders").nkeys() == 3, "catalog(name) returns the member");
  expect(shop.catalog(-1).has("sku") && shop.catalog(0).has("name"), "catalog(index) supports negatives");
  expect(shop.get("items").has_value() && !shop.get("nope").has_value(), "get() is the non-throwing lookup");
  expect_throw([&] { (void)shop.catalog("nope"); }, "unknown catalog name should throw");
  expect_throw([&] { (void)shop.catalog(3); }, "out-of-range catalog index should throw");

  expect(!Locus().defined() && Locus().empty() == true, "default Locus is undefined");
  expect_throw([&] { (void)Locus().ntables(); }, "undefined Locus should refuse use");
  expect_throw([&] { Locus l({{"a", Catalog()}}); (void)l; }, "undefined member catalog should throw");
  expect_throw([&] { Locus l({{"a", shop.catalog("orders")}, {"a", shop.catalog("items")}}); (void)l; },
               "duplicate catalog names should throw");

  expect(shop.head(2).keys() == std::vector<std::string>({"customers", "orders"}), "head keeps the first tables");
  expect(shop.tail(1).keys() == std::vector<std::string>({"items"}), "tail keeps the last tables");
  expect(shop.select({"items", "customers"}).keys() == std::vector<std::string>({"items", "customers"}),
         "select reorders");
  expect_throw([&] { (void)shop.select({"customers", "nope"}); }, "select of an unknown name should throw");

  const auto replaced = shop.with("items", shop.catalog("customers"));
  expect(replaced.catalog("items").has("name") && shop.catalog("items").has("sku"),
         "with() replaces without touching the original");
  expect(shop.with("extra", shop.catalog("items")).ntables() == 4, "with() appends a new name");
  const auto smaller = shop.without("items");
  expect(smaller.ntables() == 2 && shop.ntables() == 3, "without() returns a new Locus");
  expect_throw([&] { (void)shop.without("nope"); }, "without of an unknown name should throw");
}

void test_links_and_paths() {
  auto shop = make_shop();
  shop = shop.link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);
  shop = shop.link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);
  expect(shop.nrelationships() == 2, "two links declared");
  expect(shop.has_relationship("orders->customers") && shop.has_relationship("items->orders"),
         "unnamed links are named left->right");
  expect(shop.relationship("items->orders")->cardinality == RelationshipCardinality::ManyToOne,
         "relationship() returns the stored declaration");
  expect(shop.neighbors("orders") == std::vector<std::string>({"customers", "items"}),
         "neighbors lists both directions in declaration order");

  // Same pair twice gets a distinct auto-name; explicit duplicates are refused.
  const auto twice = shop.link("orders", "cust_id", "customers", "id");
  expect(twice.has_relationship("orders->customers#2"), "auto names stay unique");
  Relationship dup;
  dup.name = "orders->customers";
  dup.left_table = "orders";
  dup.right_table = "customers";
  dup.keys = {{"cust_id", "id"}};
  expect_throw([&] { (void)shop.link(dup); }, "an explicit duplicate relationship name should throw");
  expect_throw([&] { (void)shop.link("orders", "nope", "customers", "id"); }, "missing left key should throw");
  expect_throw([&] { (void)shop.link("orders", "cust_id", "customers", "nope"); }, "missing right key should throw");
  expect_throw([&] { (void)shop.link("orders", "cust_id", "ghost", "id"); }, "unknown table should throw");

  expect(!shop.unlink("items->orders").has_relationship("items->orders"), "unlink removes a link");
  expect(shop.without("orders").nrelationships() == 0, "removing a catalog removes the links touching it");

  const auto to_customers = shop.path("items", "customers");
  expect(to_customers.has_value() && to_customers->steps.size() == 2, "items reaches customers in two hops");
  expect(to_customers->steps[0].from_table == "items" && to_customers->steps[0].to_table == "orders" &&
             !to_customers->steps[0].reversed,
         "first hop follows the declared direction");
  const auto back = shop.path("customers", "items");
  expect(back.has_value() && back->steps.size() == 2 && back->steps[0].reversed && back->steps[1].reversed,
         "walking links backwards is marked reversed");
  expect(shop.path("orders", "orders")->steps.empty(), "a table reaches itself in zero hops");
  const auto isolated = shop.with("island", shop.catalog("customers"));
  expect(!isolated.path("island", "orders").has_value(), "an unlinked table has no path");
  expect_throw([&] { (void)shop.path("orders", "ghost"); }, "path to an unknown table should throw");
  expect(shop.to_string().find("orders->customers") != std::string::npos, "to_string lists links");
}

void test_audit_and_validate() {
  auto shop = make_shop().link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);
  const auto audits = shop.audit();
  expect(audits.size() == 1, "one link, one audit");
  const auto& a = audits[0];
  expect(a.right_unique && !a.left_unique, "customers.id is unique, orders.cust_id repeats");
  expect(a.left_rows == 4 && a.right_rows == 3, "audit counts aligned rows");
  expect(a.left_orphans == 1, "order 13 has no customer");
  expect(a.right_orphans == 1, "customer 3 has no orders");
  expect(a.status == RelationshipValidationStatus::Warning, "orphans on a required side are a warning");
  expect(!a.message.empty(), "a warning carries a message");
  shop.validate();  // warnings do not throw

  Relationship tolerant;
  tolerant.left_table = "orders";
  tolerant.right_table = "customers";
  tolerant.keys = {{"cust_id", "id"}};
  tolerant.name = "tolerant";
  tolerant.left_optional = true;
  tolerant.right_optional = true;
  const auto ok = make_shop().link(tolerant).audit("tolerant");
  expect(ok.status == RelationshipValidationStatus::Validated, "declared-optional orphans validate cleanly");

  // Declaring the wrong cardinality is Invalid and validate() throws.
  const auto wrong = make_shop().link("customers", "id", "orders", "cust_id", RelationshipCardinality::OneToOne);
  expect(wrong.audit()[0].status == RelationshipValidationStatus::Invalid, "one-to-one with repeating keys is invalid");
  expect_throw([&] { wrong.validate(); }, "validate() should throw on an invalid link");
  const auto one_to_many = make_shop().link("customers", "id", "orders", "cust_id", RelationshipCardinality::OneToMany);
  expect(one_to_many.audit()[0].status != RelationshipValidationStatus::Invalid,
         "one-to-many with a unique left side is fine");
  expect_throw([&] { (void)shop.audit("nope"); }, "auditing an unknown link should throw");
}

void test_join_aligned_catalogs() {
  const auto shop = make_shop()
                        .link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne)
                        .link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);

  // The declared link supplies the keys; inner join drops the orphan order.
  const auto joined = shop.join("orders", "customers");
  expect(has_key(joined, "amount") && has_key(joined, "name"), "join keeps both sides' streams");
  expect(values_of(joined, "amount") == std::vector<double>({5.0, 7.0, 11.0}), "inner join keeps matched orders");
  expect(joined.get("name")->as_note().vector() == std::vector<std::string_view>({"Ada", "Ada", "Ben"}),
         "customer names follow their orders");

  // Joining from the other side flips the cardinality that is enforced.
  const auto reverse = shop.join("customers", "orders");
  expect(values_of(reverse, "amount") == std::vector<double>({5.0, 7.0, 11.0}), "reverse join sees the same matches");

  JoinSpec left_join;
  left_join.how = JoinHow::Left;
  const auto kept = shop.join("orders", "customers", left_join);
  expect(values_of(kept, "amount").size() == 4, "a left join keeps the orphan order");

  // Explicit keys need no declared link.
  JoinSpec explicit_keys;
  explicit_keys.keys = {{"item_order", "order_id"}};
  const auto manual = make_shop().join("items", "orders", explicit_keys);
  expect(values_of(manual, "amount").size() == 5, "explicit keys join without a link");

  expect_throw([&] { (void)make_shop().join("items", "orders"); }, "no keys and no link should throw");
  JoinSpec bad_keys;
  bad_keys.keys = {{"nope", "order_id"}};
  expect_throw([&] { (void)make_shop().join("items", "orders", bad_keys); }, "unknown join key should throw");
  expect_throw([&] { (void)shop.join("orders", "ghost"); }, "unknown table should throw");

  // The declared cardinality is enforced at join time.
  const auto one_to_one = make_shop().link("customers", "id", "orders", "cust_id", RelationshipCardinality::OneToOne);
  expect_throw([&] { (void)one_to_one.join("customers", "orders"); },
               "a violated declared cardinality should fail the join");

  JoinSpec cross;
  cross.how = JoinHow::Cross;
  expect(values_of(shop.join("customers", "items", cross), "id").size() == 15, "cross join multiplies rows");

  // Chain: items -> orders -> customers.
  const auto chain = shop.join(*shop.path("items", "customers"), JoinHow::Inner);
  expect(has_key(chain, "sku") && has_key(chain, "name") && has_key(chain, "amount"),
         "a path join accumulates every table's streams");
  expect(values_of(chain, "amount").size() == 5, "all five items trace back to a customer");
  expect(shop.join(JoinPath{"orders", "orders", {}}).get("amount").has_value(), "an empty path returns the table");
}

Locus make_linked_shop() {
  return make_shop()
      .link("orders", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne)
      .link("items", "item_order", "orders", "order_id", RelationshipCardinality::ManyToOne);
}

bool has_validity(const Catalog& c, std::string_view key) {
  return pluss::kindle::detail::has_validity_mask(*c.get(key));
}

std::vector<bool> valid_of(const Catalog& c, std::string_view key) {
  const auto s = *c.get(key);
  if (!pluss::kindle::detail::has_validity_mask(s)) return std::vector<bool>(static_cast<std::size_t>(s.size()), true);
  return pluss::kindle::detail::validity_mask(s)->to_vec<bool>();
}

std::int64_t records_of(const Catalog& c, std::string_view key) { return c.get(key)->size(); }

Stream with_nulls(Stream s, const std::vector<bool>& valid) {
  std::vector<double> mask;
  for (const bool v : valid) mask.push_back(v ? 1.0 : 0.0);
  return pluss::kindle::detail::with_validity_mask(
      s, pluss::make_series(mask, {static_cast<std::int64_t>(mask.size())}).to(pluss::codem::ScalarType::Bool));
}

void test_join_ragged_catalogs() {
  // Ragged on purpose: 'id' has 3 records, 'tag' only 2.
  Catalog ragged(Streams{
      {"id", nums({1.0, 2.0, 3.0})},
      {"tag", words({"x", "y"})},
  });
  Catalog scores(Streams{
      {"id", nums({1.0, 2.0, 3.0})},
      {"score", nums({10.0, 20.0, 30.0})},
  });
  const Locus locus({{"ragged", ragged}, {"scores", scores}});

  JoinSpec on_id;
  on_id.keys = {{"id", "id"}};

  // Default (Preserve): nothing dropped, nothing padded, the ragged stream stays ragged.
  const auto kept = locus.join_detailed("ragged", "scores", on_id);
  expect(values_of(kept.catalog, "id").size() == 3, "all three records survive the join");
  expect(values_of(kept.catalog, "score") == std::vector<double>({10.0, 20.0, 30.0}), "all three scores join");
  expect(records_of(kept.catalog, "tag") == 2, "the short stream stays short: no padding is invented");
  expect(kept.report.output_records == 3 && kept.report.match.left_dropped == 0 &&
             kept.report.match.right_dropped == 0 && kept.report.match.left_unkeyed == 0,
         "the report accounts for every record");

  // Pad: rectangular, with an explicit null where the stream had no record.
  pluss::bundle::locus::JoinOptions pad;
  pad.ragged.materialize = pluss::bundle::locus::RaggedMaterialize::Pad;
  const auto padded = locus.join("ragged", "scores", on_id, pad);
  expect(records_of(padded, "tag") == 3 && valid_of(padded, "tag") == std::vector<bool>({true, true, false}),
         "materialize=Pad fills the missing record with a null");

  // Trim: cut to the shortest stream; lossy, and the report says exactly how lossy.
  pluss::bundle::locus::JoinOptions trim;
  trim.ragged.domain = pluss::bundle::locus::RaggedDomain::Shortest;
  const auto trimmed = locus.join_detailed("ragged", "scores", on_id, trim);
  expect(values_of(trimmed.catalog, "id").size() == 2, "domain=Shortest drops the record the short stream lacks");
  expect(trimmed.report.trimmed_left == 1 && trimmed.report.match.right_dropped == 1,
         "the report counts the trimmed and omitted records");

  // Strict: refuse ragged input outright.
  pluss::bundle::locus::JoinOptions strict;
  strict.ragged.domain = pluss::bundle::locus::RaggedDomain::Exact;
  expect_throw([&] { (void)locus.join("ragged", "scores", on_id, strict); }, "domain=Exact should reject ragged input");

  // Audit and validate use the same engine and options.
  const auto linked = locus.link("ragged", "id", "scores", "id", RelationshipCardinality::OneToOne);
  const auto audit = linked.audit()[0];
  expect(audit.left_rows == 3 && audit.left_unique && audit.right_unique, "audit sees all three records");
  expect(audit.status == RelationshipValidationStatus::Validated, "a clean ragged link validates");
  expect(linked.audit(strict)[0].status == RelationshipValidationStatus::Invalid,
         "auditing under domain=Exact reports the ragged catalog as invalid instead of throwing");
}

void test_join_unkeyed_and_null_keys() {
  // 'id' covers 2 of the 4 records in this catalog: records 2 and 3 have no key at all.
  Catalog sparse(Streams{{"id", nums({1.0, 2.0})}, {"v", nums({10.0, 20.0, 30.0, 40.0})}});
  Catalog other(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"w", nums({100.0, 200.0, 300.0})}});
  const Locus locus({{"sparse", sparse}, {"other", other}});
  JoinSpec on_id;
  on_id.keys = {{"id", "id"}};

  JoinSpec left = on_id;
  left.how = JoinHow::Left;
  const auto kept = locus.join_detailed("sparse", "other", left);
  expect(values_of(kept.catalog, "v") == std::vector<double>({10.0, 20.0, 30.0, 40.0}),
         "a left join keeps records that have no key");
  expect(records_of(kept.catalog, "w") == 4 && valid_of(kept.catalog, "w") == std::vector<bool>({true, true, false, false}),
         "keyless records get explicit nulls on the other side, not silence");
  expect(kept.report.match.left_unkeyed == 2 && kept.report.match.left_unmatched == 2,
         "the report names the keyless records");

  const auto inner = locus.join_detailed("sparse", "other", on_id);
  expect(values_of(inner.catalog, "v") == std::vector<double>({10.0, 20.0}), "an inner join drops them");
  expect(inner.report.match.left_dropped == 2 && inner.report.match.left_unkeyed == 2,
         "...and says so: two records dropped, both unkeyed");

  // Null and NaN keys never match, not even each other.
  Catalog a(Streams{{"k", with_nulls(nums({1.0, 2.0, 3.0}), {true, false, true})}, {"x", nums({1.0, 2.0, 3.0})}});
  Catalog b(Streams{{"k", with_nulls(nums({1.0, 2.0, 3.0}), {true, false, true})}, {"y", nums({4.0, 5.0, 6.0})}});
  const Locus nulls({{"a", a}, {"b", b}});
  JoinSpec on_k;
  on_k.keys = {{"k", "k"}};
  expect(values_of(nulls.join("a", "b", on_k), "x") == std::vector<double>({1.0, 3.0}), "null keys never match");
  Catalog nan_a(Streams{{"k", nums({1.0, std::nan(""), 3.0})}, {"x", nums({1.0, 2.0, 3.0})}});
  Catalog nan_b(Streams{{"k", nums({std::nan(""), 3.0})}, {"y", nums({5.0, 6.0})}});
  const Locus nans({{"a", nan_a}, {"b", nan_b}});
  expect(values_of(nans.join("a", "b", on_k), "x") == std::vector<double>({3.0}), "NaN keys never match");
}

void test_join_key_kinds_and_forms() {
  JoinSpec on_k;
  on_k.keys = {{"k", "k"}};

  // Text keys.
  Catalog t1(Streams{{"k", words({"b", "a", "c", "a"})}, {"x", nums({1.0, 2.0, 3.0, 4.0})}});
  Catalog t2(Streams{{"k", words({"a", "b"})}, {"y", nums({10.0, 20.0})}});
  const Locus text({{"t1", t1}, {"t2", t2}});
  const auto text_join = text.join("t1", "t2", on_k);
  expect(values_of(text_join, "x") == std::vector<double>({1.0, 2.0, 4.0}) &&
             values_of(text_join, "y") == std::vector<double>({20.0, 10.0, 10.0}),
         "text keys join, in left-record order");

  // Integer keys against float keys compare by value.
  Catalog ints(Streams{{"k", Stream(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}, {3}))}, {"x", nums({1.0, 2.0, 3.0})}});
  Catalog floats(Streams{{"k", nums({1.0, 2.5, 3.0})}, {"y", nums({7.0, 8.0, 9.0})}});
  const Locus mixed({{"ints", ints}, {"floats", floats}});
  expect(values_of(mixed.join("ints", "floats", on_k), "x") == std::vector<double>({1.0, 3.0}), "int and float keys match by value");

  // Key kinds that cannot be compared are an error, not a silent empty result.
  const Locus bad({{"ints", ints}, {"text", t2}});
  expect_throw([&] { (void)bad.join("ints", "text", on_k); }, "a numeric key cannot join a text key");

  // Many-to-many expansion keeps left order, then right order.
  Catalog m1(Streams{{"k", nums({1.0, 1.0, 2.0})}, {"x", nums({1.0, 2.0, 3.0})}});
  Catalog m2(Streams{{"k", nums({1.0, 2.0, 1.0})}, {"y", nums({10.0, 20.0, 30.0})}});
  const Locus many({{"m1", m1}, {"m2", m2}});
  const auto expanded = many.join("m1", "m2", on_k);
  expect(values_of(expanded, "x") == std::vector<double>({1.0, 1.0, 2.0, 2.0, 3.0}) &&
             values_of(expanded, "y") == std::vector<double>({10.0, 30.0, 10.0, 30.0, 20.0}),
         "duplicate keys expand in left-then-right order");

  // Composite keys.
  Catalog c1(Streams{{"a", nums({1.0, 1.0, 2.0})}, {"b", words({"x", "y", "x"})}, {"v", nums({1.0, 2.0, 3.0})}});
  Catalog c2(Streams{{"a", nums({1.0, 2.0})}, {"b", words({"y", "x"})}, {"w", nums({10.0, 20.0})}});
  const Locus comp({{"c1", c1}, {"c2", c2}});
  JoinSpec on_ab;
  on_ab.keys = {{"a", "a"}, {"b", "b"}};
  const auto composite = comp.join("c1", "c2", on_ab);
  expect(values_of(composite, "v") == std::vector<double>({2.0, 3.0}) &&
             values_of(composite, "w") == std::vector<double>({10.0, 20.0}),
         "composite keys must all match");

  // Same-named keys are coalesced where no right-only record can exist, kept apart where it can.
  Catalog l(Streams{{"id", nums({1.0, 2.0})}, {"x", nums({1.0, 2.0})}});
  Catalog r(Streams{{"id", nums({2.0, 3.0})}, {"y", nums({20.0, 30.0})}});
  const Locus pair({{"l", l}, {"r", r}});
  JoinSpec on_id;
  on_id.keys = {{"id", "id"}};
  const auto inner = pair.join("l", "r", on_id);
  expect(inner.nkeys() == 3 && inner.has("id") && !inner.has("id_r"), "an inner join folds the shared key into one column");
  JoinSpec outer = on_id;
  outer.how = JoinHow::Outer;
  const auto full = pair.join("l", "r", outer);
  expect(full.has("id") && full.has("id_r"), "an outer join keeps both keys, so the right-only record's key survives");
  expect(values_of(full, "id_r").size() == 3 && valid_of(full, "id_r") == std::vector<bool>({false, true, true}),
         "the right key column carries the right-only record");
  pluss::bundle::locus::JoinOptions keep_both;
  keep_both.coalesce_keys = false;
  expect(pair.join("l", "r", on_id, keep_both).has("id_r"), "coalesce_keys=false keeps both key columns");

  // Non-key collisions get the per-table suffix; if that still collides it is an error.
  Catalog dup1(Streams{{"id", nums({1.0})}, {"name", words({"left"})}});
  Catalog dup2(Streams{{"id", nums({1.0})}, {"name", words({"right"})}});
  const Locus dup({{"dup1", dup1}, {"dup2", dup2}});
  const auto suffixed = dup.join("dup1", "dup2", on_id);
  expect(suffixed.has("name") && suffixed.has("name_dup2"), "a clashing column gets the right table's suffix");
  JoinSpec same_suffix = on_id;
  same_suffix.right_suffix = "";
  Catalog dup3(Streams{{"id", nums({1.0})}, {"name", words({"x"})}, {"name_dup2", words({"y"})}});
  const Locus clash({{"dup1", dup1}, {"dup2", dup3}});
  expect_throw([&] { (void)clash.join("dup1", "dup2", on_id); }, "a suffix that still collides should be an error");

  // The engine never guesses between links.
  auto two_links = pair.link("l", "id", "r", "id");
  two_links = two_links.link("l", "id", "r", "id");
  expect_throw([&] { (void)two_links.join("l", "r"); }, "several direct links without a named one should be an error");
  JoinSpec named;
  named.relationship_name = "l->r#2";
  expect(values_of(two_links.join("l", "r", named), "x") == std::vector<double>({2.0}), "naming the link resolves it");
  expect_throw([&] { (void)pair.join("l", "r"); }, "no keys and no link should be an error");
}

void test_match_against_brute_force() {
  namespace rel = pluss::relational;
  std::mt19937_64 rng(77);
  const std::vector<JoinHow> hows = {JoinHow::Inner, JoinHow::Left, JoinHow::Right, JoinHow::Outer,
                                     JoinHow::LeftOnly, JoinHow::RightOnly};

  const auto brute = [](const std::vector<std::vector<std::int64_t>>& L, const std::vector<std::vector<std::int64_t>>& R,
                        std::int64_t nl, std::int64_t nr, JoinHow how) {
    const auto keyed = [](const std::vector<std::vector<std::int64_t>>& side, std::int64_t i) {
      for (const auto& c : side) if (c[static_cast<std::size_t>(i)] == rel::kNoKey) return false;
      return true;
    };
    const auto equal = [&](std::int64_t i, std::int64_t j) {
      if (!keyed(L, i) || !keyed(R, j)) return false;
      for (std::size_t c = 0; c < L.size(); ++c) if (L[c][static_cast<std::size_t>(i)] != R[c][static_cast<std::size_t>(j)]) return false;
      return true;
    };
    std::vector<std::pair<std::int64_t, std::int64_t>> out;
    const bool right_driven = how == JoinHow::Right || how == JoinHow::RightOnly;
    const bool emit_matched = how != JoinHow::LeftOnly && how != JoinHow::RightOnly;
    if (!right_driven) {
      for (std::int64_t i = 0; i < nl; ++i) {
        bool any = false;
        for (std::int64_t j = 0; j < nr; ++j) if (equal(i, j)) { any = true; if (emit_matched) out.push_back({i, j}); }
        if (!any && how != JoinHow::Inner) out.push_back({i, -1});
      }
      if (how == JoinHow::Outer) {
        for (std::int64_t j = 0; j < nr; ++j) {
          bool any = false;
          for (std::int64_t i = 0; i < nl; ++i) if (equal(i, j)) any = true;
          if (!any) out.push_back({-1, j});
        }
      }
    } else {
      for (std::int64_t j = 0; j < nr; ++j) {
        bool any = false;
        for (std::int64_t i = 0; i < nl; ++i) if (equal(i, j)) { any = true; if (emit_matched) out.push_back({i, j}); }
        if (!any) out.push_back({-1, j});
      }
    }
    return out;
  };

  for (int trial = 0; trial < 150; ++trial) {
    const std::int64_t nl = static_cast<std::int64_t>(rng() % 40), nr = static_cast<std::int64_t>(rng() % 40);
    const std::size_t ncols = 1 + rng() % 2;
    std::vector<std::vector<std::int64_t>> L(ncols, std::vector<std::int64_t>(static_cast<std::size_t>(nl)));
    std::vector<std::vector<std::int64_t>> R(ncols, std::vector<std::int64_t>(static_cast<std::size_t>(nr)));
    for (auto& c : L) for (auto& x : c) x = (rng() % 7 == 0) ? rel::kNoKey : static_cast<std::int64_t>(rng() % 5);
    for (auto& c : R) for (auto& x : c) x = (rng() % 7 == 0) ? rel::kNoKey : static_cast<std::int64_t>(rng() % 5);
    rel::KeyColumns lk, rk;
    lk.records = nl; rk.records = nr;
    for (auto& c : L) lk.cols.push_back(c.data());
    for (auto& c : R) rk.cols.push_back(c.data());
    for (const auto how : hows) {
      rel::MatchOptions options;
      options.how = how;
      const auto got = rel::match(lk, rk, options);
      const auto want = brute(L, R, nl, nr, how);
      bool same = got.left.size() == want.size();
      for (std::size_t p = 0; same && p < want.size(); ++p) same = got.left[p] == want[p].first && got.right[p] == want[p].second;
      expect(same, "match must equal the brute-force reference for every join kind");
      expect(got.report.pairs == static_cast<std::int64_t>(want.size()), "the report counts the pairs");
    }
  }

  // Large enough for the parallel probe: 200k left records against 1k distinct right keys.
  const std::int64_t nl = 200000, nr = 1000;
  std::vector<std::int64_t> L(static_cast<std::size_t>(nl)), R(static_cast<std::size_t>(nr));
  for (std::int64_t i = 0; i < nl; ++i) L[static_cast<std::size_t>(i)] = (i % 17 == 0) ? rel::kNoKey : i % 1500;
  for (std::int64_t j = 0; j < nr; ++j) R[static_cast<std::size_t>(j)] = j;
  rel::KeyColumns lk, rk;
  lk.records = nl; rk.records = nr;
  lk.cols = {L.data()}; rk.cols = {R.data()};
  rel::MatchOptions options;
  options.how = JoinHow::Left;
  const auto big = rel::match(lk, rk, options);
  std::int64_t expected = 0;
  for (std::int64_t i = 0; i < nl; ++i) expected += 1;  // unique right keys: one pair per left record
  expect(big.report.pairs == expected && big.report.right_unique, "unique right keys give one pair per left record");
  bool ordered = true;
  for (std::int64_t i = 0; i < nl && ordered; ++i) {
    const auto l = L[static_cast<std::size_t>(i)];
    ordered = big.left[static_cast<std::size_t>(i)] == i && big.right[static_cast<std::size_t>(i)] == ((l != rel::kNoKey && l < nr) ? l : -1);
  }
  expect(ordered, "the parallel probe returns pairs in left order with the right partner");
  expect(big.report.left_unkeyed == (nl + 16) / 17, "unkeyed records are counted");

  // Contracts are enforced before any pair is produced.
  std::vector<std::int64_t> dup = {1, 1, 2};
  std::vector<std::int64_t> uniq = {1, 2, 3};
  rel::KeyColumns dk, uk;
  dk.records = 3; uk.records = 3;
  dk.cols = {dup.data()}; uk.cols = {uniq.data()};
  rel::MatchOptions many_to_one;
  many_to_one.validate = RelationshipCardinality::ManyToOne;
  expect_throw([&] { (void)rel::match(uk, dk, many_to_one); }, "many_to_one with repeating right keys should throw");
  expect(rel::match(dk, uk, many_to_one).report.matched_pairs == 3, "many_to_one with unique right keys is fine");
  rel::MatchOptions one_to_many;
  one_to_many.validate = RelationshipCardinality::OneToMany;
  expect_throw([&] { (void)rel::match(dk, uk, one_to_many); }, "one_to_many with repeating left keys should throw");
}


Stream times(const std::vector<std::string>& iso) { return Stream(pluss::make_timeline(iso)); }

// A small feature-store: customers (with a signup time), their region, and timed events.
Locus make_feature_store() {
  Catalog customers(Streams{
      {"id", nums({1.0, 2.0, 3.0, 4.0})},
      {"name", words({"Ada", "Ben", "Cy", "Di"})},
      {"region_id", nums({10.0, 10.0, 20.0, 30.0})},
      {"signup", times({"2024-03-01T00:00:00Z", "2024-03-05T00:00:00Z", "2024-03-02T00:00:00Z", "2024-03-09T00:00:00Z"})},
  });
  Catalog regions(Streams{{"region_id", nums({10.0, 20.0, 30.0})}, {"zone", words({"west", "east", "north"})}});
  Catalog events(Streams{
      {"event_id", nums({1, 2, 3, 4, 5, 6, 7, 8, 9})},
      {"cust_id", nums({1, 1, 1, 2, 2, 3, 1, 2, 9})},
      {"spend", nums({5, 7, 2, 3, 11, 4, 6, 8, 100})},
      {"tag", words({"a", "b", "a", "c", "c", "d", "b", "a", "z"})},
      {"at", times({"2024-03-01T00:00:00Z", "2024-03-03T00:00:00Z", "2024-03-10T00:00:00Z", "2024-03-02T00:00:00Z",
                    "2024-03-04T00:00:00Z", "2024-03-01T00:00:00Z", "2024-03-11T00:00:00Z", "2024-03-05T00:00:00Z",
                    "2024-03-01T00:00:00Z"})},
  });
  pluss::bundle::locus::TableMeta customer_meta;
  customer_meta.primary_key = {"id"};
  customer_meta.time_field = "signup";
  pluss::bundle::locus::TableMeta event_meta;
  event_meta.primary_key = {"event_id"};
  event_meta.time_field = "at";
  return Locus({{"customers", customers}, {"regions", regions}, {"events", events}})
      .link("customers", "region_id", "regions", "region_id", RelationshipCardinality::ManyToOne)
      .link("events", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne)
      .meta("customers", customer_meta)
      .meta("events", event_meta);
}

// One column of a Nexus pull result against one native stream: same values, same nulls.
void expect_same_column(const Catalog& native, const pluss::table::DataFrame& nexus, const std::string& name) {
  expect(native.has(name) && nexus.get(name).has_value(), "both pulls should produce column '" + name + "'");
  const auto theirs = nexus.features(name);
  const auto mine = *native.get(name);
  const auto their_null = theirs.isnull().as_series().to_vec<bool>();
  std::vector<bool> my_valid = valid_of(native, name);
  expect(their_null.size() == my_valid.size(), "column '" + name + "' should have the same length in both pulls");
  for (std::size_t i = 0; i < my_valid.size(); ++i) {
    expect(their_null[i] == !my_valid[i], "column '" + name + "' should have the same nulls in both pulls");
  }
  if (theirs.kind() == pluss::table::SlateKind::Note) {
    const auto a = theirs.as_note().vector();
    const auto b = mine.as_note().vector();
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (my_valid[i]) expect(a[i] == b[i], "column '" + name + "' should have the same text in both pulls");
    }
    return;
  }
  if (theirs.kind() == pluss::table::SlateKind::Timeline) {
    const auto a = theirs.as_timeline().epoch_nanos().to_vec<std::int64_t>();
    const auto b = mine.as_timeline().epoch_nanos().to_vec<std::int64_t>();
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (my_valid[i]) expect(a[i] == b[i], "column '" + name + "' should have the same times in both pulls");
    }
    return;
  }
  const auto a = theirs.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  const auto b = mine.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (my_valid[i]) {
      expect(std::fabs(a[i] - b[i]) < 1e-9 * (1.0 + std::fabs(a[i])),
             "column '" + name + "' should have the same values in both pulls");
    }
  }
}

void test_native_pull_matches_nexus() {
  namespace nx = pluss::bundle::nexus;
  const auto store = make_feature_store();

  auto native = store.pull("customers");
  native.take({"id", "name"}).take("regions.zone");
  native.agg("events")
      .sum("spend", "total")
      .count("event_id", "n")
      .ndistinct("tag", "tags")
      .mean("spend", "avg")
      .median("spend", "mid")
      .min("spend", "lo")
      .max("spend", "hi")
      .std("spend", "sd")
      .var("spend", "vr")
      .end();
  native.add(nx::sum("events.spend", "spend_7d").windowed(7, "d"))
      .add(nx::count("events.event_id", "n_7d").windowed(7, "d"))
      .add(nx::mean("events.spend", "avg_7d").windowed(7, "d"))
      .add(nx::min("events.spend", "lo_7d").windowed(7, "d"))
      .add(nx::max("events.spend", "hi_7d").windowed(7, "d"))
      .add(nx::sum("events.spend", "spend_7d_root").root_windowed(7, "d"))
      .add(nx::count("events.event_id", "n_7d_root").root_windowed(7, "d"));
  const auto mine = native.collect();

  // The same specs through the delegated (aligned Nexus) pull.
  auto reference = store.nexus().pull("customers");
  for (const auto& spec : native.specs()) reference.add(spec);
  const auto theirs = reference.collect();

  for (const auto& name : mine.keys()) expect_same_column(mine, theirs, name);
  expect(mine.nkeys() == static_cast<std::int64_t>(theirs.nfeatures()), "both pulls produce the same columns");

  // And the numbers themselves, computed by hand.
  expect(values_of(mine, "total").size() == 4, "one record per customer");
  const auto total = values_of(mine, "total");
  expect(total[0] == 20.0 && total[1] == 22.0 && total[2] == 4.0 && !valid_of(mine, "total")[3],
         "spend per customer; a customer with no events has no sum");
  expect(values_of(mine, "n") == std::vector<double>({4.0, 3.0, 1.0, 0.0}), "events per customer; none is a count of 0");
  expect(values_of(mine, "tags") == std::vector<double>({2.0, 2.0, 1.0, 0.0}), "distinct tags per customer");
  expect(values_of(mine, "mid")[0] == 5.5 && values_of(mine, "mid")[1] == 8.0, "median of an even and an odd group");
  const auto spend_7d = values_of(mine, "spend_7d");
  expect(spend_7d[0] == 8.0 && spend_7d[1] == 22.0 && spend_7d[2] == 4.0 && !valid_of(mine, "spend_7d")[3],
         "7-day window ends at each customer's latest event");
  const auto spend_root = values_of(mine, "spend_7d_root");
  expect(spend_root[0] == 5.0 && spend_root[1] == 22.0 && spend_root[2] == 4.0, "7-day window ends at the customer's signup");
  expect(values_of(mine, "n_7d_root")[3] == 0.0 && valid_of(mine, "n_7d_root")[3], "no events in a rooted window is a count of 0");

  // Aggregates Nexus does not support in a window are native here.
  const auto windowed_median = store.pull("customers")
                                   .add(nx::median("events.spend", "mid_7d").windowed(7, "d"))
                                   .add(nx::std("events.spend", "sd_7d").windowed(7, "d"))
                                   .add(nx::ndistinct("events.tag", "tags_7d").windowed(7, "d"))
                                   .collect();
  expect(values_of(windowed_median, "mid_7d")[0] == 4.0, "median of the two events in customer 1's latest window");
  expect(std::fabs(values_of(windowed_median, "sd_7d")[0] - std::sqrt(8.0)) < 1e-12, "std of {2, 6}");
  expect(values_of(windowed_median, "tags_7d")[0] == 2.0, "distinct tags in the window");
}

void test_native_pull_rules() {
  namespace nx = pluss::bundle::nexus;
  const auto store = make_feature_store();

  // Plain fields follow only root-preserving links; expanding one is an error that names the link.
  expect_throw([&] { (void)store.pull("customers").take("events.spend").collect(); },
               "a plain field across a one-to-many link must be rejected");
  expect_throw([&] { (void)store.pull("regions").take("customers.name").collect(); },
               "a plain field across a one-to-many link (from the other end) must be rejected");
  expect_throw([&] { (void)store.pull("customers").take("nope").collect(); }, "an unknown feature should be rejected");
  expect_throw([&] { (void)store.pull("customers").take("ghost.x").collect(); }, "an unknown table should be rejected");
  expect_throw([&] { (void)store.pull("customers").collect(); }, "a pull with no fields should be rejected");
  expect_throw([&] { (void)store.pull("customers").take("id", "x").take("name", "x").collect(); },
               "an output alias collision should be rejected");
  expect_throw([&] { (void)store.pull("customers").sum("name").collect(); },
               "an aggregate on the root table should be rejected");
  expect_throw([&] { (void)store.pull("ghost"); }, "an unknown root should be rejected");

  // Aggregates need the root's primary key, unless that check is switched off.
  auto no_meta = Locus({{"customers", store.catalog("customers")}, {"events", store.catalog("events")}})
                     .link("events", "cust_id", "customers", "id", RelationshipCardinality::ManyToOne);
  expect_throw([&] { (void)no_meta.pull("customers").sum("events.spend", "t").collect(); },
               "an aggregate without a declared root primary key should be rejected");
  pluss::bundle::locus::PullOptions relaxed;
  relaxed.require_primary_key = false;
  auto without_key = no_meta.pull("customers");
  without_key.options(relaxed).sum("events.spend", "t");
  expect(values_of(without_key.collect(), "t")[0] == 20.0, "with the key check off, records are grouped by root record");

  // A duplicate primary key is refused too: a root record would be ambiguous.
  pluss::bundle::locus::TableMeta bad_key;
  bad_key.primary_key = {"region_id"};
  const auto dup_key = store.meta("customers", bad_key);
  expect_throw([&] { (void)dup_key.pull("customers").sum("events.spend", "t").collect(); },
               "a non-unique root primary key should be rejected");

  // Windows need a declared time field.
  const auto no_time = store.meta("events", []() { pluss::bundle::locus::TableMeta m; m.primary_key = {"event_id"}; return m; }());
  expect_throw([&] { (void)no_time.pull("customers").add(nx::sum("events.spend", "s").windowed(7, "d")).collect(); },
               "a window without a time_field should be rejected");
  expect_throw(
      [&] {
        (void)store.pull("customers")
            .add(nx::sum("events.spend", "s").windowed(std::numeric_limits<std::int64_t>::max(), "w"))
            .collect();
      },
      "a window whose unit conversion overflows should be rejected");
  expect_throw(
      [&] {
        (void)store.pull("customers")
            .add(nx::sum("events.spend", "s").windowed(std::numeric_limits<std::int64_t>::max(), "s"))
            .collect();
      },
      "a window outside the nanosecond timestamp range should be rejected");

  // Multi-hop: customers -> events -> (events' own link), aggregated through an intermediate table.
  auto chain = store;
  Catalog items(Streams{{"item_id", nums({1, 2, 3, 4})}, {"event_id", nums({1, 1, 4, 8})}, {"qty", nums({2, 3, 5, 7})}});
  pluss::bundle::locus::TableMeta item_meta;
  item_meta.primary_key = {"item_id"};
  chain = chain.with("items", items).link("items", "event_id", "events", "event_id", RelationshipCardinality::ManyToOne).meta("items", item_meta);
  const auto qty = chain.pull("customers").sum("items.qty", "qty").collect();
  expect(values_of(qty, "qty")[0] == 5.0 && values_of(qty, "qty")[1] == 12.0 && !valid_of(qty, "qty")[2],
         "an aggregate reaches through two links without a table in between");
  const auto reference = chain.nexus().pull("customers").sum("items.qty", "qty").collect();
  expect_same_column(qty, reference, "qty");

  // The plain-field path through a ragged root: nothing is dropped, nothing padded.
  Catalog ragged_root(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"label", words({"x", "y"})}, {"region_id", nums({10.0, 20.0, 30.0})}});
  auto ragged = Locus({{"customers", ragged_root}, {"regions", store.catalog("regions")}})
                    .link("customers", "region_id", "regions", "region_id", RelationshipCardinality::ManyToOne);
  const auto kept = ragged.pull("customers").take({"id", "label", "regions.zone"}).collect();
  expect(records_of(kept, "id") == 3 && records_of(kept, "label") == 2 && records_of(kept, "zone") == 3,
         "a ragged root keeps its raggedness through a pull");
  pluss::bundle::locus::PullOptions pad;
  pad.ragged.materialize = pluss::bundle::locus::RaggedMaterialize::Pad;
  auto padded_pull = ragged.pull("customers");
  padded_pull.options(pad).take({"id", "label"});
  const auto padded = padded_pull.collect();
  expect(records_of(padded, "label") == 3 && valid_of(padded, "label") == std::vector<bool>({true, true, false}),
         "materialize=Pad fills the short stream with a null");
}

void test_native_traverse_and_plans() {
  const auto store = make_feature_store();

  const auto via = store.from("events").via("regions").collect();
  expect(has_key(via, "zone") && has_key(via, "spend") && values_of(via, "spend").size() == 9,
         "via walks the shortest path and keeps every event (a left join)");
  expect(valid_of(via, "zone")[8] == false, "the orphan event has no region");

  const auto picked = store.from("events").join("customers").pick({"event_id", "name"}).collect();
  expect(picked.nkeys() == 2 && has_key(picked, "name"), "pick keeps only the named columns");
  const auto dropped = store.from("events").join("customers").drop(std::vector<std::string>{"tag", "at"}).collect();
  expect(!has_key(dropped, "tag") && has_key(dropped, "name"), "drop removes the named columns");
  expect_throw([&] { (void)store.from("events").pick({"a", "a"}); }, "picking a feature twice should be rejected");
  expect_throw([&] { (void)store.from("events").join("regions").collect(); }, "joining tables with no direct link should be rejected");

  // The shared resolver refuses to guess between links, in traversal too.
  const auto two = store.link("events", "cust_id", "customers", "id");
  expect_throw([&] { (void)two.from("events").join("customers").collect(); },
               "several direct links without a named one should be rejected");

  // Every matching mode is available through the immutable traversal builder.
  const auto shop = make_linked_shop();
  auto mode = [](JoinHow how) {
    JoinSpec spec;
    spec.how = how;
    return spec;
  };
  expect(shop.from("orders").join("customers").collect().maxrows() == 3,
         "the default traversal join is inner");
  expect(shop.from("orders").join("customers", mode(JoinHow::Left)).collect().maxrows() == 4,
         "a left traversal keeps every root record");
  expect(shop.from("orders").join("customers", mode(JoinHow::Right)).collect().maxrows() == 4,
         "a right traversal keeps every right record");
  expect(shop.from("orders").join("customers", mode(JoinHow::Outer)).collect().maxrows() == 5,
         "an outer traversal keeps both orphans");
  expect(shop.from("orders").join("customers", mode(JoinHow::LeftOnly)).collect().maxrows() == 1,
         "left-only returns root records without a partner");
  expect(shop.from("orders").join("customers", mode(JoinHow::RightOnly)).collect().maxrows() == 1,
         "right-only returns target records without a partner");
  expect(make_shop().from("orders").join("customers", mode(JoinHow::Cross)).collect().maxrows() == 12,
         "cross traversal produces the Cartesian product without a relationship");

  const auto root = shop.from("orders");
  expect(root.pick({"order_id"}).pick({"amount"}).collect().keys() == std::vector<std::string>({"amount"}),
         "a later pick replaces the earlier pick");
  expect(root.drop({"amount"}).drop({"cust_id"}).collect().has("amount"),
         "a later drop replaces the earlier drop");
  expect(root.pick({}).collect().defined() && root.pick({}).collect().nkeys() == 0,
         "an empty pick produces a defined empty catalog");
  expect(root.drop({}).collect().keys() == root.collect().keys(), "an empty drop is a no-op");
  const auto unknown_pick = root.pick({"absent"});
  expect_throw([&] { (void)unknown_pick.collect(); }, "unknown projected keys are checked at collection");
  expect(root.via("orders").collect().keys() == root.collect().keys(), "a zero-hop path preserves the anchor");
  expect(shop.from("items").via("customers", JoinHow::Cross).collect().maxrows() == 60,
         "cross traversal uses the path to choose catalogs without binding relationship keys");

  // A native plan equals the aligned Nexus plan on rectangular data.
  const auto plan = store.plan("events", "regions", pluss::bundle::locus::JoinHow::Left);
  expect(plan.steps.size() == 2 && plan.root_table == "events", "plan() is the shortest path as join steps");
  const auto mine = store.materialize(plan);
  const auto theirs = store.nexus().materialize(plan);
  for (const auto& name : mine.keys()) expect_same_column(mine, theirs, name);

  // Saved plans run natively and follow renames.
  const auto saved = store.store("with_region", plan);
  expect(values_of(saved.run("with_region"), "spend").size() == 9, "a saved plan runs");
  expect(has_key(saved.rename("regions", "zones").run("with_region"), "zone"), "a saved plan follows a rename");
}

void test_workspace_ops_and_meta() {
  auto shop = make_linked_shop();

  // Policy: default is Outer; it round-trips through with_policy and feeds nexus().
  expect(shop.policy().align == Align::Outer, "default policy aligns Outer");
  TabelizePolicy strict;
  strict.align = Align::Strict;
  expect(shop.with_policy(strict).policy().align == Align::Strict && shop.policy().align == Align::Outer,
         "with_policy returns a new Locus");
  const auto nx = shop.nexus();
  expect(nx.ntables() == 3 && nx.relationships().size() == 2, "nexus() carries tables and links");
  expect(shop.nexus().ntables() == 3, "nexus() is repeatable (cached)");

  // rename keeps links pointing at the new name.
  const auto people = shop.rename("customers", "people");
  expect(people.has("people") && !people.has("customers"), "rename re-keys the catalog");
  expect(people.relationship("orders->customers")->right_table == "people", "rename updates link ends");
  expect(values_of(people.join("orders", "people"), "amount") == std::vector<double>({5.0, 7.0, 11.0}),
         "joins keep working after a rename");
  expect_throw([&] { (void)shop.rename("customers", "orders"); }, "rename onto an existing name should throw");
  expect_throw([&] { (void)shop.rename("customers", "customers"); }, "rename to the same name should throw");

  expect(shop.drop({"items", "orders"}).keys() == std::vector<std::string>({"customers"}),
         "drop removes several catalogs");
  expect(shop.drop({"items"}).nrelationships() == 1, "drop removes the links touching it");

  const auto injected = shop.inject({{"extra", shop.catalog("items")}, {"more", shop.catalog("orders")}});
  expect(injected.ntables() == 5, "inject adds several catalogs");

  const auto trimmed = shop.apply("orders", [](const Catalog& c) { return c.head(2); });
  expect(values_of(trimmed.catalog("orders"), "order_id").size() == 2 && shop.catalog("orders").maxrows() == 4,
         "apply rewrites one catalog without touching the original");
  expect_throw([&] { (void)shop.apply("orders", Locus::CatalogFn{}); }, "an empty callback should throw");
  expect_throw([&] { (void)shop.apply("orders", [](const Catalog&) { return Catalog(); }); },
               "a callback returning an undefined Catalog should throw");
  expect(shop.apply_all([](const Catalog& c) { return c.head(1); }).catalog("items").maxrows() == 1,
         "apply_all applies to every catalog");
  expect(shop.flow([](const Locus& l) { return l.drop({"items"}); }).ntables() == 2, "flow threads the Locus through a function");

  // Archives: one catalog per depth slice.
  const pluss::kindle::Archive archive(std::vector<Catalog>{shop.catalog("orders"), shop.catalog("items")});
  const auto from_archive = Locus::from_archive(archive);
  expect(from_archive.keys() == std::vector<std::string>({"catalog_0", "catalog_1"}), "unlabelled archive slices get index names");
  const pluss::kindle::Archive labelled(std::vector<Catalog>{shop.catalog("orders"), shop.catalog("items")},
                                        words({"o", "i"}));
  expect(Locus::from_archive(labelled).keys() == std::vector<std::string>({"o", "i"}), "text depth labels name the catalogs");
  expect(shop.inject(labelled).ntables() == 5, "inject(Archive) appends its slices");

  // Table metadata.
  pluss::bundle::locus::TableMeta meta;
  meta.grain = "one row per customer";
  meta.primary_key = {"id"};
  const auto described = shop.meta("customers", meta);
  expect(described.has_table_meta("customers") && !shop.has_table_meta("customers"), "meta returns a new Locus");
  expect(described.table_meta("customers")->primary_key == std::vector<std::string>({"id"}), "meta round-trips");
  expect(described.rename("customers", "people").has_table_meta("people"), "metadata follows a rename");
  expect(!described.drop({"customers"}).has_table_meta("customers"), "metadata is dropped with its catalog");
  expect_throw([&] { (void)shop.meta("ghost", meta); }, "metadata on an unknown catalog should throw");
  expect(described.nexus().table_meta("customers").has_value(), "metadata reaches the aligned Nexus");
}

void test_integrity_and_reports() {
  auto shop = make_linked_shop();
  pluss::bundle::locus::TableMeta meta;
  meta.primary_key = {"id"};
  shop.meta_("customers", meta);

  expect(shop.validate_keys("customers"), "the declared primary key is present and unique");
  expect(shop.validate_keys("customers", {"id"}), "an explicit unique key validates");
  expect(!shop.validate_keys("orders", {"cust_id"}), "a repeating key does not validate");

  const auto dups = shop.duplicates("orders", {"cust_id"});
  expect(has_key(dups, "cust_id"), "duplicates() returns the offending records");
  expect(values_of(dups, "order_id").size() >= 2, "both orders of customer 1 are reported");

  const auto left_orphans = shop.detect_orphans("orders->customers");
  expect(values_of(left_orphans, "order_id") == std::vector<double>({13.0}), "order 13 has no customer");
  const auto right_orphans = shop.detect_orphans("orders->customers", pluss::bundle::locus::RelationshipSide::Right);
  expect(values_of(right_orphans, "id") == std::vector<double>({3.0}), "customer 3 has no orders");

  expect(!shop.check("orders->customers"), "check is strict: orphans on a required side fail it");
  pluss::bundle::locus::Relationship tolerant;
  tolerant.name = "tolerant";
  tolerant.left_table = "orders";
  tolerant.right_table = "customers";
  tolerant.keys = {{"cust_id", "id"}};
  tolerant.left_optional = true;
  tolerant.right_optional = true;
  const auto relaxed = make_shop().link(tolerant);
  expect(relaxed.check("tolerant") && relaxed.validate_relationship("tolerant"),
         "orphans declared optional on both sides pass check and validate_relationship");
  const auto wrong = make_shop().link("customers", "id", "orders", "cust_id", RelationshipCardinality::OneToOne);
  expect(!wrong.validate_relationship("customers->orders"), "a contradicted cardinality fails validation");

  // Reports are DataFrames over the workspace.
  expect(shop.audit_report().ncases() >= 2, "audit_report has a row per link");
  expect(shop.relationship_audit("orders->customers").ncases() >= 1, "relationship_audit reports one link");
  expect(shop.grain_audit("customers").ncases() >= 1, "grain_audit reports a table");
  expect(shop.summary().ncases() >= 3, "summary covers every catalog");
  expect(shop.profile().ncases() >= 1 && shop.scan().ncases() >= 1, "profile and scan report on the data");
  expect(shop.schema().ncases() >= 1 && shop.graph().ncases() >= 2, "schema and graph describe the workspace");
  (void)shop.deduce();

  const auto fp = shop.fingerprint();
  expect(!fp.empty() && fp == shop.fingerprint(), "the fingerprint is stable");
  expect(fp == make_linked_shop().meta("customers", meta).fingerprint(), "equal workspaces share a fingerprint");
  expect(fp != shop.with("customers", shop.catalog("orders")).fingerprint(), "changed data changes the fingerprint");
}

void test_saved_plans() {
  auto shop = make_linked_shop();
  pluss::bundle::locus::JoinPlan plan;
  plan.root_table = "orders";
  plan.result_name = "orders_with_customers";
  pluss::bundle::locus::JoinPlanStep step;
  step.left_table = "orders";
  step.right_table = "customers";
  step.spec.keys = {{"cust_id", "id"}};
  step.spec.how = JoinHow::Inner;
  plan.steps.push_back(step);

  const auto direct = shop.materialize(plan);
  expect(values_of(direct, "amount") == std::vector<double>({5.0, 7.0, 11.0}), "materialize runs a plan");

  auto stored = shop.store("by_customer", plan);
  expect(stored.has_store("by_customer") && !shop.has_store("by_customer"), "store returns a new Locus");
  expect(stored.store("by_customer")->root_table == "orders", "store round-trips the plan");
  expect(values_of(stored.run("by_customer"), "amount") == std::vector<double>({5.0, 7.0, 11.0}), "run executes a saved plan");
  expect(values_of(shop.run("anonymous", plan), "amount").size() == 3, "run with an inline plan");
  expect_throw([&] { (void)shop.run("missing"); }, "running an unknown plan should throw");

  const auto renamed = stored.rename("customers", "people");
  expect(renamed.store("by_customer")->steps[0].right_table == "people", "plans follow a rename");
  expect(values_of(renamed.run("by_customer"), "amount").size() == 3, "a renamed plan still runs");
  expect(!stored.drop({"customers"}).has_store("by_customer"), "a plan is dropped with a catalog it uses");

  auto bad = plan;
  bad.steps[0].right_table = "ghost";
  expect_throw([&] { (void)shop.store("bad", bad); }, "a plan naming an unknown catalog should be refused");
}

void test_builders() {
  pluss::bundle::locus::TableMeta customer_meta;
  customer_meta.primary_key = {"id"};
  pluss::bundle::locus::TableMeta order_meta;
  order_meta.primary_key = {"order_id"};
  const auto shop = make_linked_shop().meta("customers", customer_meta).meta("orders", order_meta);

  // Plain takes need no metadata; aggregating pulls need the root's grain.
  expect(has_key(make_linked_shop().pull("customers").take("id").collect(), "id"), "take works without metadata");
  expect_throw([&] { (void)make_linked_shop().pull("customers").take("id").sum("orders.amount", "t").collect(); },
               "an aggregating pull without a declared primary key should fail");

  const auto pulled = shop.pull("customers").take("id").sum("orders.amount", "total").collect();
  expect(has_key(pulled, "id") && has_key(pulled, "total"), "pull gathers columns across a link");
  const auto totals = values_of(pulled, "total");
  expect(totals.size() == 3 && totals[0] == 12.0 && totals[1] == 11.0, "customer totals sum their orders");

  const auto scoped = shop.pull("customers")
                          .take("id")
                          .agg("orders")
                          .count("order_id", "n")
                          .sum({{"amount", "spent"}})
                          .end()
                          .collect();
  expect(values_of(scoped, "n")[0] == 2.0 && values_of(scoped, "spent")[0] == 12.0, "agg() scopes reducers to one table");
  auto builder = shop.pull("customers");
  builder.take("id");
  expect(builder.root() == "customers" && builder.specs().size() == 1, "a pull reports its root and specs");
  expect_throw([&] { (void)shop.pull("ghost"); }, "pulling from an unknown root should throw");

  const auto via = shop.from("orders").via("customers").pick({"order_id", "name"}).collect();
  expect(has_key(via, "order_id") && has_key(via, "name") && !has_key(via, "amount"), "traverse picks columns");
  expect(values_of(via, "order_id").size() == 4, "via() is a left join, keeping the orphan order");
  const auto joined = shop.from("orders").join("customers").drop(std::vector<std::string>{"amount"}).collect();
  expect(has_key(joined, "name") && !has_key(joined, "amount"), "traverse can drop columns");

  const auto sql = shop.query().execute("SELECT order_id, amount FROM orders WHERE amount > 6 ORDER BY order_id");
  expect(values_of(sql, "order_id") == std::vector<double>({11.0, 12.0, 13.0}), "SQL runs over the catalogs by name");
  expect(values_of(sql, "amount") == std::vector<double>({7.0, 11.0, 13.0}), "SQL returns the selected streams");
  const auto prepared = shop.query().prepare("SELECT order_id FROM orders WHERE amount > {floor} ORDER BY order_id");
  expect(values_of(prepared.execute({{"floor", 6.0}}), "order_id").size() == 3, "prepared SQL binds parameters");
  expect(values_of(prepared.execute({{"floor", 11.0}}), "order_id").size() == 1, "prepared SQL re-runs with new parameters");
  expect_throw([&] { (void)prepared.execute(); }, "prepared SQL rejects an unresolved value placeholder");
  expect_throw([&] { (void)shop.query().execute("SELEKT order_id FROM orders"); }, "bad SQL should throw");
}

void test_persistence() {
  namespace fs = std::filesystem;
  const auto dir = fs::temp_directory_path() / ("locus_store_test_" + std::to_string(::getpid()));
  fs::remove_all(dir);

  pluss::bundle::locus::TableMeta meta;
  meta.primary_key = {"id"};
  pluss::bundle::locus::JoinPlan plan;
  plan.root_table = "orders";
  plan.result_name = "orders_with_customers";
  pluss::bundle::locus::JoinPlanStep step;
  step.left_table = "orders";
  step.right_table = "customers";
  step.spec.keys = {{"cust_id", "id"}};
  step.spec.how = JoinHow::Inner;
  plan.steps.push_back(step);
  const auto shop = make_linked_shop().meta("customers", meta).store("by_customer", plan);
  pluss::bundle::locus::StoreOptions options;
  options.kind = pluss::bundle::nexus::StoreKind::Csv;
  shop.save(dir.string(), options);
  expect(fs::exists(dir), "save writes a directory");

  const auto back = Locus::read(dir.string(), options);
  expect(back.keys() == shop.keys(), "the saved catalogs come back by name");
  expect(back.nrelationships() == 2, "links survive the round trip");
  expect(back.has_table_meta("customers"), "table metadata survives the round trip");
  expect(back.has_store("by_customer"), "saved plans survive the round trip");
  expect(values_of(back.run("by_customer"), "amount") == std::vector<double>({5.0, 7.0, 11.0}),
         "a restored saved plan still runs");
  expect(values_of(back.join("orders", "customers"), "amount") == std::vector<double>({5.0, 7.0, 11.0}),
         "a restored Locus joins like the original");
  fs::remove_all(dir);

  {  // Ragged catalogs persist as they are: no padding, no trimming, in either format.
    Catalog ragged(Streams{
        {"id", nums({1.0, 2.0, 3.0})},
        {"tag", words({"x", "y"})},
    });
    Catalog scores(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"score", nums({10.0, 20.0, 30.0})}});
    auto rl = Locus({{"ragged", ragged}, {"scores", scores}});
    rl.link_(Relationship{"", "ragged", "scores", {{"id", "id"}}});
    for (const auto kind : {pluss::bundle::nexus::StoreKind::Parquet, pluss::bundle::nexus::StoreKind::Csv}) {
      pluss::bundle::locus::StoreOptions o;
      o.kind = kind;
      const auto rdir = fs::temp_directory_path() / ("locus_ragged_store_" + std::to_string(::getpid()));
      fs::remove_all(rdir);
      rl.save(rdir.string(), o);
      const auto rb = Locus::read(rdir.string(), o);
      expect(rb.catalog("ragged").get("id")->size() == 3 && rb.catalog("ragged").get("tag")->size() == 2,
             "a ragged catalog comes back with its exact extents");
      expect(rb.nrelationships() == 1, "links survive a ragged round trip");
      const auto j = rb.join_detailed("ragged", "scores");
      expect(j.report.summary() == rl.join_detailed("ragged", "scores").report.summary(),
             "a restored Locus joins ragged data exactly like the original");
      fs::remove_all(rdir);
    }
  }

  {  // A store written by Nexus (rectangular tables) is still readable.
    const auto ndir = fs::temp_directory_path() / ("locus_from_nexus_store_" + std::to_string(::getpid()));
    fs::remove_all(ndir);
    shop.nexus().save(ndir.string(), options);
    const auto nb = Locus::read(ndir.string(), options);
    expect(nb.keys() == shop.keys() && nb.nrelationships() == 2, "Locus::read accepts a Nexus-written store");
    fs::remove_all(ndir);
  }

  // from_nexus is the same bridge without the disk.
  const auto via_nexus = Locus::from_nexus(shop.nexus());
  expect(via_nexus.shape() == shop.shape(), "from_nexus(nexus()) preserves tables and links");
}


void test_alignment_cache() {
  auto shop = make_linked_shop();
  const auto s0 = shop.alignment_cache_stats();
  (void)shop.nexus();
  const auto s1 = shop.alignment_cache_stats();
  expect(s1.second - s0.second == shop.ntables(), "the first nexus() aligns every catalog once");

  // Changing only the links reuses every aligned table.
  auto relinked = shop;
  relinked.unlink_("items->orders");
  (void)relinked.nexus();
  const auto s2 = shop.alignment_cache_stats();
  expect(s2.second == s1.second && s2.first - s1.first == shop.ntables(),
         "a link change re-aligns nothing");

  // Replacing one catalog aligns only that one.
  auto edited = shop.with("extra", shop.catalog("items"));
  (void)edited.nexus();
  const auto s3 = shop.alignment_cache_stats();
  expect(s3.second - s2.second == 0 && s3.first > s2.first,
         "an added catalog sharing a handle with an existing one is served from the cache");
  auto changed = shop.with("items", Catalog(Streams{{"x", nums({1.0, 2.0, 3.0})}}));
  (void)changed.nexus();
  const auto s4 = shop.alignment_cache_stats();
  expect(s4.second - s3.second == 1, "a replaced catalog is re-aligned exactly once");
  const auto same_content = shop.with("items", shop.catalog("items").map([](const pluss::kindle::Stream& st) { return st; }));
  (void)same_content.nexus();
  expect(shop.alignment_cache_stats().second == s4.second, "a catalog with identical contents is served from the cache");

  // A different policy is a different cache line.
  auto inner = shop.with_policy(pluss::kindle::TabelizePolicy{});
  pluss::kindle::TabelizePolicy p;
  p.align = pluss::kindle::Align::Inner;
  (void)shop.with_policy(p).nexus();
  const auto s5 = shop.alignment_cache_stats();
  expect(s5.second - s4.second == shop.ntables(), "a new policy aligns each catalog under that policy");
  (void)inner;

  // Results agree with an uncached build.
  expect(shop.nexus().ntables() == shop.ntables(), "cached Nexus has every table");
}


void test_hardening_edges() {
  namespace fs = std::filesystem;
  using pluss::bundle::locus::JoinHow;
  auto ints = [](const std::vector<std::int64_t>& v) {
    return Stream(pluss::make_series(v, {static_cast<std::int64_t>(v.size())}));
  };
  Catalog a(Streams{{"id", nums({1, 2, 3})}, {"v", nums({10, 20, 30})}});
  Catalog empty_streams(Streams{{"id", nums({})}, {"w", nums({})}});
  JoinSpec on_id;
  on_id.keys = {{"id", "id"}};

  // Zero-length streams on either or both sides.
  on_id.how = JoinHow::Left;
  expect(Locus({{"a", empty_streams}, {"b", a}}).join_detailed("a", "b", on_id).report.output_records == 0,
         "an empty left side joins to nothing");
  expect(Locus({{"a", a}, {"b", empty_streams}}).join_detailed("a", "b", on_id).report.output_records == 3,
         "a left join against an empty right keeps every left record");
  on_id.how = JoinHow::Outer;
  expect(Locus({{"a", empty_streams}, {"b", empty_streams}}).join_detailed("a", "b", on_id).report.output_records == 0,
         "two empty sides join to nothing");

  // Extreme and special key values.
  {
    Catalog x(Streams{{"k", ints({INT64_MIN, INT64_MAX, 0})}, {"p", nums({1, 2, 3})}});
    Catalog y(Streams{{"k", ints({INT64_MIN, INT64_MAX, 5})}, {"q", nums({7, 8, 9})}});
    JoinSpec s;
    s.keys = {{"k", "k"}};
    s.how = JoinHow::Inner;
    expect(Locus({{"x", x}, {"y", y}}).join_detailed("x", "y", s).report.output_records == 2,
           "INT64_MIN and INT64_MAX are ordinary keys");
    const double nan = std::nan("");
    Catalog fx(Streams{{"k", nums({nan, INFINITY, -0.0, 1})}});
    Catalog fy(Streams{{"k", nums({nan, INFINITY, 0.0, 2})}, {"q", nums({1, 2, 3, 4})}});
    expect(Locus({{"x", fx}, {"y", fy}}).join_detailed("x", "y", s).report.output_records == 2,
           "NaN never matches, infinity matches itself, -0.0 equals +0.0");
  }

  // Empty workspaces and catalogs persist and reload.
  const auto tmp = fs::temp_directory_path() / ("locus_hardening_" + std::to_string(::getpid()));
  fs::remove_all(tmp);
  {
    Locus none(std::vector<std::pair<std::string, Catalog>>{});
    none.save((tmp / "empty").string());
    expect(Locus::read((tmp / "empty").string()).ntables() == 0, "an empty Locus round-trips");
    Catalog nostreams(Streams{});
    Locus mixed({{"n", nostreams}, {"a", a}});
    mixed.save((tmp / "mixed").string());
    const auto back = Locus::read((tmp / "mixed").string());
    expect(back.ntables() == 2 && back.catalog("n").nkeys() == 0, "a catalog with no streams round-trips");
    (void)mixed.nexus();
  }
  {  // zero-length streams, path-like names, nested target directory, both formats
    Catalog z(Streams{{"../evil/../key", nums({})}, {"n", words({})}, {"ok", nums({1})}});
    Locus l({{"we/ird name", z}});
    for (const auto kind : {pluss::bundle::nexus::StoreKind::Parquet, pluss::bundle::nexus::StoreKind::Csv}) {
      pluss::bundle::locus::StoreOptions o;
      o.kind = kind;
      const auto dir = (tmp / "z" / "nested").string();
      l.save(dir, o);
      const auto b = Locus::read(dir, o);
      expect(b.catalog("we/ird name").get("../evil/../key")->size() == 0 &&
                 b.catalog("we/ird name").get("n")->size() == 0 && b.catalog("we/ird name").get("ok")->size() == 1,
             "zero-length streams and path-like names round-trip");
    }
    expect(!fs::exists(tmp / "evil"), "stream keys cannot escape the store directory");
  }
  {  // saving a smaller workspace over a bigger one reads back as the smaller one
    Locus big({{"a", a}, {"b", a}, {"c", a}});
    big.save((tmp / "over").string());
    Locus small({{"z", a}});
    small.save((tmp / "over").string());
    const auto b = Locus::read((tmp / "over").string());
    expect(b.ntables() == 1 && b.has("z"), "the manifest, not leftover files, defines the store");
  }
  fs::remove_all(tmp);

  // A const Locus is safe to use from many threads at once.
  {
    Catalog cust(Streams{{"id", nums({1, 2, 3})}, {"name", words({"A", "B", "C"})}});
    Catalog ord(Streams{{"oid", nums({10, 11, 12, 13})}, {"cid", nums({1, 1, 2, 9})}, {"amt", nums({5, 7, 11, 13})}});
    Locus l({{"customers", cust}, {"orders", ord}});
    l.link_(Relationship{"", "orders", "customers", {{"cid", "id"}}});
    const Locus& shared = l;
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
      threads.emplace_back([&] {
        try {
          for (int i = 0; i < 20; ++i) {
            if (shared.join("orders", "customers").get("amt")->size() != 3) ++errors;
            if (shared.nexus().ntables() != 2) ++errors;
            (void)shared.alignment_cache_stats();
            (void)shared.with("extra", shared.catalog("orders")).nexus();
            if (shared.pull("orders").take("customers.name").collect().nkeys() < 1) ++errors;
          }
        } catch (...) {
          ++errors;
        }
      });
    }
    for (auto& th : threads) th.join();
    expect(errors == 0, "concurrent joins, alignments and pulls on a const Locus agree");
  }

  // Zero-length streams through reductions and elementwise ops.
  {
    Catalog z(Streams{{"x", nums({})}, {"y", nums({})}});
    expect(z.reduce({"sum", "count", "std"}).get("sum")->size() == 0, "fused reductions of empty streams are empty");
    expect(z.combine("x", "y", "+").get("x+y")->size() == 0, "combining empty streams is empty");
    Catalog half(Streams{{"x", nums({1, 2})}, {"y", nums({})}});
    expect(half.combine("x", "y", "+").get("x+y")->size() == 2, "an empty operand makes every position null");
  }
}

void test_in_place_and_sharing() {
  auto shop = make_shop();
  const auto before = shop;
  shop.link_(Relationship{"", "orders", "customers", {{"cust_id", "id"}}});
  expect(shop.nrelationships() == 1 && before.nrelationships() == 0, "in-place update must not alias other handles");
  shop.with_("extra", shop.catalog("items")).unlink_("orders->customers").without_("extra");
  expect(shop.nrelationships() == 0 && shop.ntables() == 3, "chained in-place updates apply in order");

  // The aligned Nexus is cached; every in-place edit must invalidate it.
  auto live = make_linked_shop();
  expect(live.nexus().ntables() == 3 && live.nexus().relationships().size() == 2, "baseline aligned workspace");
  live.with_("extra", live.catalog("items"));
  expect(live.nexus().ntables() == 4, "with_ invalidates the cached Nexus");
  live.unlink_("items->orders");
  expect(live.nexus().relationships().size() == 1, "unlink_ invalidates the cached Nexus");
  live.apply_("orders", [](const Catalog& c) { return c.head(1); });
  expect(live.nexus().frame("orders").ncases() == 1, "apply_ invalidates the cached Nexus");
  TabelizePolicy inner;
  inner.align = Align::Inner;
  expect(live.nexus(inner).ntables() == 4 && live.nexus().ntables() == 4, "a per-call policy gets its own cache entry");
}

} // namespace

int main() {
  ::setenv("PLUSS_ERROR_NATIVE_TRACE", "0", 0);
  try {
    test_construction_and_selection();
    test_links_and_paths();
    test_audit_and_validate();
    test_join_aligned_catalogs();
    test_join_ragged_catalogs();
    test_join_unkeyed_and_null_keys();
    test_join_key_kinds_and_forms();
    test_match_against_brute_force();
    test_native_pull_matches_nexus();
    test_native_pull_rules();
    test_native_traverse_and_plans();
    test_workspace_ops_and_meta();
    test_integrity_and_reports();
    test_saved_plans();
    test_builders();
    test_persistence();
    test_alignment_cache();
    test_hardening_edges();
    test_in_place_and_sharing();
  } catch (const std::exception& e) {
    std::cerr << "[bundle_locus_tests] failure: " << e.what() << "\n";
    return 1;
  }
  std::cout << "[bundle_locus_tests] all checks passed\n";
  return 0;
}
