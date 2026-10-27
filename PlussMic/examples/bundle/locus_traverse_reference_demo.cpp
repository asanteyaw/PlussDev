#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::bundle::Locus;
using pluss::bundle::locus::JoinHow;
using pluss::bundle::locus::JoinOptions;
using pluss::bundle::locus::JoinSpec;
using pluss::bundle::locus::Relationship;
using pluss::bundle::locus::RelationshipCardinality;
using pluss::kindle::Catalog;
using pluss::kindle::RaggedLoss;
using pluss::kindle::RaggedMaterialize;
using pluss::kindle::Stream;

using Streams = std::vector<std::pair<std::string, Stream>>;

Stream nums(std::initializer_list<double> values) {
  std::vector<double> owned(values);
  return Stream(pluss::make_series(owned, {static_cast<std::int64_t>(owned.size())}));
}

Stream words(std::initializer_list<std::string_view> values) {
  std::vector<std::string> owned;
  owned.reserve(values.size());
  for (const auto value : values) owned.emplace_back(value);
  return Stream(pluss::make_note(owned, {static_cast<std::int64_t>(owned.size())}));
}

Locus shop(bool linked = true, bool ragged = false) {
  Catalog customers(Streams{
      {"customer_id", nums({1, 2, 3, 4})},
      {"name", words({"Ada", "Bola", "Chen", "Dara"})},
  });
  Catalog orders(ragged ? Streams{
                              {"order_id", nums({100, 101, 102, 103, 104})},
                              {"customer_id", nums({1, 2, 2, 3, 9})},
                              {"amount", nums({50, 75, 20, 90})},
                              {"state", words({"paid", "paid", "held", "void", "paid"})},
                          }
                        : Streams{
                              {"order_id", nums({100, 101, 102, 103, 104})},
                              {"customer_id", nums({1, 2, 2, 3, 9})},
                              {"amount", nums({50, 75, 20, 90, 40})},
                              {"state", words({"paid", "paid", "held", "void", "paid"})},
                          });
  Catalog items(Streams{
      {"item_id", nums({1000, 1001, 1002, 1003})},
      {"order_id", nums({100, 100, 101, 104})},
      {"sku", words({"pen", "book", "bag", "lamp"})},
  });

  Locus out({{"customers", customers}, {"orders", orders}, {"items", items}});
  if (!linked) return out;
  return out
      .link("orders", "customer_id", "customers", "customer_id", RelationshipCardinality::ManyToOne)
      .link("items", "order_id", "orders", "order_id", RelationshipCardinality::ManyToOne);
}

void heading(std::string_view title) { std::cout << "== " << title << " ==\n"; }

void print_keys(const Catalog& catalog) {
  std::cout << "keys=[";
  for (std::size_t i = 0; i < catalog.keys().size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << catalog.keys()[i];
  }
  std::cout << "] rows=" << catalog.maxrows() << "\n";
}

void print_numbers(const Catalog& catalog, std::string_view key) {
  const auto stream = catalog.keys(key);
  const auto values = stream.as_series().host<double>();
  const auto valid = pluss::kindle::detail::validity_mask(stream);
  const auto mask = valid.has_value() ? valid->host<bool>() : std::vector<bool>{};
  std::cout << key << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    if (!mask.empty() && !mask[i]) std::cout << "null";
    else std::cout << values[i];
  }
  std::cout << "]\n";
}

void print_words(const Catalog& catalog, std::string_view key) {
  const auto stream = catalog.keys(key);
  const auto values = stream.as_note().vector();
  const auto valid = pluss::kindle::detail::validity_mask(stream);
  const auto mask = valid.has_value() ? valid->host<bool>() : std::vector<bool>{};
  std::cout << key << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    if (!mask.empty() && !mask[i]) std::cout << "null";
    else std::cout << values[i];
  }
  std::cout << "]\n";
}

template <class Fn>
void print_error(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const std::exception& error) {
    std::string message(error.what());
    const auto newline = message.find('\n');
    if (newline != std::string::npos) message.resize(newline);
    std::cout << label << ": " << message << "\n";
  }
}

JoinSpec mode(JoinHow how) {
  JoinSpec spec;
  spec.how = how;
  return spec;
}

} // namespace

int main() {
  const auto data = shop();

  heading("collect anchor");
  const auto anchor = data.from("orders").collect();
  print_keys(anchor);
  print_numbers(anchor, "order_id");

  heading("collect accumulated plan");
  const auto joined = data.from("orders").join("customers").collect();
  print_keys(joined);
  print_words(joined, "name");

  heading("collect immutable branches");
  const auto base = data.from("orders");
  print_keys(base.pick({"order_id"}).collect());
  print_keys(base.join("customers").pick({"order_id", "name"}).collect());
  print_keys(base.collect());

  heading("join modes");
  for (const auto [name, how] : {
           std::pair{"inner", JoinHow::Inner},
           std::pair{"left", JoinHow::Left},
           std::pair{"right", JoinHow::Right},
           std::pair{"outer", JoinHow::Outer},
           std::pair{"left_only", JoinHow::LeftOnly},
           std::pair{"right_only", JoinHow::RightOnly},
       }) {
    const auto result = data.from("orders").join("customers", mode(how)).collect();
    std::cout << name << " rows=" << result.maxrows() << "\n";
  }
  const auto crossed = shop(false).from("orders").join("customers", mode(JoinHow::Cross)).collect();
  std::cout << "cross rows=" << crossed.maxrows() << "\n";

  heading("join explicit keys");
  JoinSpec manual;
  manual.keys = {{"customer_id", "customer_id"}};
  manual.how = JoinHow::Left;
  const auto explicit_join = shop(false).from("orders").join("customers", manual).collect();
  print_numbers(explicit_join, "order_id");
  print_words(explicit_join, "name");

  heading("join named relationship");
  Relationship buyers;
  buyers.name = "orders_to_buyers";
  buyers.left_table = "orders";
  buyers.right_table = "customers";
  buyers.keys = {{"customer_id", "customer_id"}};
  buyers.cardinality = RelationshipCardinality::ManyToOne;
  const auto ambiguous = data.link(buyers);
  print_error("ambiguous", [&] { (void)ambiguous.from("orders").join("customers"); });
  JoinSpec named;
  named.relationship_name = "orders_to_buyers";
  print_keys(ambiguous.from("orders").join("customers", named).collect());

  heading("join validation timing");
  print_error("unknown catalog", [&] { (void)data.from("orders").join("absent"); });
  print_error("no relationship", [&] { (void)shop(false).from("orders").join("customers"); });
  JoinSpec bad_key;
  bad_key.keys = {{"missing", "customer_id"}};
  const auto lazy_bad_key = data.from("orders").join("customers", bad_key);
  std::cout << "bad key plan built\n";
  print_error("bad key collect", [&] { (void)lazy_bad_key.collect(); });
  JoinSpec wrong_cardinality;
  wrong_cardinality.keys = {{"customer_id", "customer_id"}};
  wrong_cardinality.validate = RelationshipCardinality::OneToOne;
  print_error("cardinality", [&] {
    (void)data.from("orders").join("customers", wrong_cardinality).collect();
  });

  heading("pick overloads and order");
  print_keys(data.from("orders").join("customers").pick({"name", "order_id"}).collect());
  const std::vector<std::string> pick_names{"amount", "state"};
  print_keys(data.from("orders").pick(pick_names).collect());

  heading("pick replacement and composition");
  print_keys(data.from("orders").pick({"order_id"}).pick({"state", "amount"}).collect());
  print_keys(data.from("orders").pick({"order_id", "state"}).drop({"state"}).collect());
  print_keys(data.from("orders").pick({}).collect());

  heading("pick failures");
  print_error("duplicate", [&] { (void)data.from("orders").pick({"order_id", "order_id"}); });
  print_error("empty name", [&] { (void)data.from("orders").pick({""}); });
  const auto lazy_pick = data.from("orders").pick({"absent"});
  std::cout << "unknown pick built\n";
  print_error("unknown pick collect", [&] { (void)lazy_pick.collect(); });

  heading("drop overloads");
  print_keys(data.from("orders").drop({"state", "amount"}).collect());
  const std::vector<std::string> drop_names{"customer_id", "state"};
  print_keys(data.from("orders").drop(drop_names).collect());

  heading("drop replacement and composition");
  print_keys(data.from("orders").drop({"state"}).drop({"amount"}).collect());
  print_keys(data.from("orders").pick({"order_id", "state"}).drop({"state"}).collect());
  print_keys(data.from("orders").drop({}).collect());

  heading("drop failures");
  print_error("duplicate", [&] { (void)data.from("orders").drop({"state", "state"}); });
  print_error("empty name", [&] { (void)data.from("orders").drop({""}); });
  const auto lazy_drop = data.from("orders").drop({"absent"});
  std::cout << "unknown drop built\n";
  print_error("unknown drop collect", [&] { (void)lazy_drop.collect(); });

  heading("via shortest path");
  const auto path = data.from("items").via("customers").pick({"item_id", "name"}).collect();
  print_numbers(path, "item_id");
  print_words(path, "name");

  heading("via mode and zero hop");
  std::cout << "left rows=" << data.from("items").via("customers").collect().maxrows() << "\n";
  std::cout << "inner rows=" << data.from("items").via("customers", JoinHow::Inner).collect().maxrows() << "\n";
  std::cout << "cross rows=" << data.from("items").via("customers", JoinHow::Cross).collect().maxrows() << "\n";
  print_keys(data.from("orders").via("orders").collect());

  heading("via updates anchor");
  const auto chained = data.from("items").via("orders").join("customers")
      .pick({"item_id", "name"}).collect();
  print_numbers(chained, "item_id");
  print_words(chained, "name");

  heading("via failures");
  print_error("unknown target", [&] { (void)data.from("orders").via("absent"); });
  const auto island = data.with("island", data.catalog("customers"));
  print_error("no path", [&] { (void)island.from("orders").via("island"); });

  heading("options key coalescing");
  JoinOptions separate_keys;
  separate_keys.coalesce_keys = false;
  const auto separate = data.from("orders").options(separate_keys).join("customers").collect();
  print_keys(separate);
  print_numbers(separate, "customer_id_customers");

  heading("options ragged materialization");
  const auto ragged = shop(true, true);
  JoinSpec left = mode(JoinHow::Left);
  const auto preserved = ragged.from("orders").join("customers", left).collect();
  std::cout << "preserve amount size=" << preserved.keys("amount").size() << " rows=" << preserved.maxrows() << "\n";
  JoinOptions padded;
  padded.ragged.materialize = RaggedMaterialize::Pad;
  const auto padded_result = ragged.from("orders").options(padded).join("customers", left).collect();
  print_numbers(padded_result, "amount");
  JoinOptions tails = padded;
  tails.ragged.alignment = pluss::kindle::RaggedAlignment::Tail;
  print_numbers(ragged.from("orders").options(tails).join("customers", left).collect(), "amount");

  heading("options ragged domain");
  JoinOptions shortest;
  shortest.ragged.domain = pluss::kindle::RaggedDomain::Shortest;
  const auto cut = ragged.from("orders").options(shortest).join("customers", left).collect();
  std::cout << "shortest rows=" << cut.maxrows() << " amount size=" << cut.keys("amount").size() << "\n";
  JoinOptions exact;
  exact.ragged.domain = pluss::kindle::RaggedDomain::Exact;
  print_error("exact", [&] {
    (void)ragged.from("orders").options(exact).join("customers", left).collect();
  });

  heading("options loss and validation");
  JoinOptions lossless;
  lossless.ragged.loss = RaggedLoss::Error;
  print_error("loss", [&] {
    (void)data.from("orders").options(lossless).join("customers").collect();
  });
  JoinOptions invalid;
  invalid.ragged.materialize = RaggedMaterialize::Pad;
  invalid.ragged.absent = pluss::kindle::RaggedAbsent::Skip;
  const auto lazy_options = data.from("orders").options(invalid).join("customers");
  std::cout << "invalid options plan built\n";
  print_error("invalid options collect", [&] { (void)lazy_options.collect(); });

  heading("options immutable branches");
  const auto option_base = data.from("orders").join("customers");
  std::cout << "default rows=" << option_base.collect().maxrows() << "\n";
  print_error("derived loss", [&] { (void)option_base.options(lossless).collect(); });
  std::cout << "default again rows=" << option_base.collect().maxrows() << "\n";
}
