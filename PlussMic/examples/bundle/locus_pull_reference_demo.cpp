#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::bundle::Locus;
using pluss::bundle::locus::PullOptions;
using pluss::bundle::locus::PullSpec;
using pluss::bundle::locus::RelationshipCardinality;
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
  for (const auto value : values) owned.emplace_back(value);
  return Stream(pluss::make_note(owned, {static_cast<std::int64_t>(owned.size())}));
}

Stream times(std::initializer_list<std::string_view> values) {
  std::vector<std::string> owned;
  owned.reserve(values.size());
  for (const auto value : values) owned.emplace_back(value);
  return Stream(pluss::timeline::make(owned));
}

Locus shop(bool ragged = false, bool metadata = true) {
  Catalog customers(ragged ? Streams{
                                 {"id", nums({1, 2, 3, 4})},
                                 {"name", words({"Ada", "Bola"})},
                                 {"region_id", nums({10, 10, 20, 30})},
                                 {"signup", times({"2024-01-10T00:00:00Z", "2024-01-15T00:00:00Z",
                                                   "2024-01-20T00:00:00Z", "2024-01-25T00:00:00Z"})},
                             }
                           : Streams{
                                 {"id", nums({1, 2, 3, 4})},
                                 {"name", words({"Ada", "Bola", "Chen", "Dara"})},
                                 {"region_id", nums({10, 10, 20, 30})},
                                 {"signup", times({"2024-01-10T00:00:00Z", "2024-01-15T00:00:00Z",
                                                   "2024-01-20T00:00:00Z", "2024-01-25T00:00:00Z"})},
                             });
  Catalog regions(Streams{
      {"region_id", nums({10, 20, 30})},
      {"zone", words({"west", "east", "north"})},
  });
  Catalog events(Streams{
      {"event_id", nums({101, 102, 103, 104, 105, 106, 107, 108})},
      {"customer_id", nums({1, 1, 1, 1, 2, 2, 2, 3})},
      {"spend", nums({2, 4, 6, 8, 5, 8, 9, 4})},
      {"reading", nums({1, std::numeric_limits<double>::quiet_NaN(), 1, 2,
                         std::numeric_limits<double>::quiet_NaN(), 5, 5})},
      {"tag", words({"web", "shop", "web", "shop", "web", "web", "shop", "shop"})},
      {"at", times({"2024-01-01T00:00:00Z", "2024-01-05T00:00:00Z",
                    "2024-01-12T00:00:00Z", "2024-01-18T00:00:00Z",
                    "2024-01-09T00:00:00Z", "2024-01-14T00:00:00Z",
                    "2024-01-20T00:00:00Z", "2024-01-19T00:00:00Z"})},
  });

  auto out = Locus({{"customers", customers}, {"regions", regions}, {"events", events}})
                 .link("customers", "region_id", "regions", "region_id", RelationshipCardinality::ManyToOne)
                 .link("events", "customer_id", "customers", "id", RelationshipCardinality::ManyToOne);
  if (!metadata) return out;

  TableMeta customers_meta;
  customers_meta.primary_key = {"id"};
  customers_meta.time_field = "signup";
  TableMeta regions_meta;
  regions_meta.primary_key = {"region_id"};
  TableMeta events_meta;
  events_meta.primary_key = {"event_id"};
  events_meta.time_field = "at";
  return out.meta("customers", customers_meta).meta("regions", regions_meta).meta("events", events_meta);
}

void heading(std::string_view title) { std::cout << "== " << title << " ==\n"; }

void print_keys(const Catalog& catalog) {
  std::cout << "keys=[";
  const auto keys = catalog.keys();
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << keys[i];
  }
  std::cout << "] rows=" << catalog.maxrows() << "\n";
}

void print_numbers(const Catalog& catalog, std::string_view key) {
  const auto stream = catalog.keys(key);
  const auto values = stream.as_series().host<double>();
  const auto valid = pluss::kindle::detail::validity_mask(stream);
  const auto mask = valid.has_value() ? valid->host<bool>() : std::vector<bool>{};
  const auto old_precision = std::cout.precision();
  std::cout << std::setprecision(12) << key << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    if (!mask.empty() && !mask[i]) std::cout << "null";
    else std::cout << values[i];
  }
  std::cout << "]\n" << std::setprecision(old_precision);
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

std::string_view op_name(pluss::bundle::nexus::PullOp op) {
  using pluss::bundle::nexus::PullOp;
  switch (op) {
    case PullOp::Take: return "take";
    case PullOp::Count: return "count";
    case PullOp::Ndistinct: return "ndistinct";
    case PullOp::Sum: return "sum";
    case PullOp::Mean: return "mean";
    case PullOp::Median: return "median";
    case PullOp::Min: return "min";
    case PullOp::Max: return "max";
    case PullOp::Std: return "std";
    case PullOp::Var: return "var";
  }
  return "unknown";
}

void print_specs(const std::vector<PullSpec>& specs) {
  std::cout << "specs=" << specs.size() << "\n";
  for (const auto& spec : specs) {
    std::cout << "source=" << spec.source << " op=" << op_name(spec.op)
              << " alias=" << (spec.alias.has_value() ? *spec.alias : "<default>");
    if (spec.window.has_value()) {
      std::cout << " window=" << spec.window->value << spec.window->unit
                << " anchor=" << (spec.window->anchor_root ? "root" : "latest");
    }
    std::cout << "\n";
  }
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

} // namespace

int main() {
  namespace nx = pluss::bundle::nexus;
  const auto data = shop();

  heading("root initial and stable");
  auto root_plan = data.pull("customers");
  std::cout << "root=" << root_plan.root() << " specs=" << root_plan.specs().size() << "\n";
  root_plan.take("id").sum("events.spend", "spent");
  std::cout << "root=" << root_plan.root() << " specs=" << root_plan.specs().size() << "\n";

  heading("root related grain");
  auto related_plan = data.pull("customers").take("regions.zone");
  std::cout << "root=" << related_plan.root() << "\n";
  const auto related_result = related_plan.collect();
  std::cout << "rows=" << related_result.maxrows() << "\n";
  print_words(related_result, "zone");

  heading("take single and default names");
  auto plain = data.pull("customers").take("id").take("regions.zone").collect();
  print_keys(plain);
  print_numbers(plain, "id");
  print_words(plain, "zone");

  heading("take single aliases");
  auto renamed = data.pull("customers").take("name", "customer").take("regions.zone", "area").collect();
  print_keys(renamed);
  print_words(renamed, "customer");
  print_words(renamed, "area");

  heading("take source list");
  auto names = data.pull("customers").take({"name", "regions.zone", "id"}).collect();
  print_keys(names);

  heading("take alias list");
  auto aliases = data.pull("customers")
                     .take({std::pair<std::string_view, std::string_view>{"id", "customer_id"},
                            {"name", "customer_name"}})
                     .collect();
  print_keys(aliases);

  heading("take table scope");
  auto scoped = data.pull("customers")
                    .take("id")
                    .take("regions", {{"region_id", "region"}, {"zone", "area"}})
                    .collect();
  print_keys(scoped);
  print_numbers(scoped, "region");
  print_words(scoped, "area");

  heading("take empty lists");
  auto empty_lists = data.pull("customers");
  empty_lists.take(std::initializer_list<std::string_view>{});
  empty_lists.take(std::initializer_list<std::pair<std::string_view, std::string_view>>{});
  empty_lists.take("regions", std::initializer_list<std::pair<std::string_view, std::string_view>>{});
  std::cout << "specs=" << empty_lists.specs().size() << "\n";
  print_error("collect", [&] { (void)empty_lists.collect(); });

  heading("take failures");
  print_error("empty source", [&] { (void)data.pull("customers").take(""); });
  print_error("empty table", [&] {
    (void)data.pull("customers").take("", {{"zone", "area"}});
  });
  const auto unknown = data.pull("customers").take("regions.absent");
  std::cout << "unknown plan specs=" << unknown.specs().size() << "\n";
  print_error("unknown collect", [&] { (void)unknown.collect(); });
  print_error("expanding", [&] { (void)data.pull("customers").take("events.spend").collect(); });

  heading("add single and batch");
  auto added = data.pull("customers");
  auto* same = &added.add(nx::take("id", "customer_id"));
  added.add({nx::take("name", "customer_name"), nx::sum("events.spend", "spent")});
  std::cout << "same_builder=" << std::boolalpha << (same == &added) << "\n";
  print_specs(added.specs());
  print_keys(added.collect());

  heading("add named and windows");
  auto windowed = data.pull("customers");
  windowed.add(nx::sum("events.spend").named("recent").windowed(7, "d"));
  windowed.add(nx::sum("events.spend").named("before_signup").root_windowed(7, "d"));
  print_specs(windowed.specs());
  const auto window_result = windowed.collect();
  print_numbers(window_result, "recent");
  print_numbers(window_result, "before_signup");

  heading("add deferred validation");
  auto malformed = data.pull("customers");
  malformed.add(PullSpec{});
  std::cout << "malformed plan specs=" << malformed.specs().size() << "\n";
  print_error("malformed collect", [&] { (void)malformed.collect(); });
  auto duplicate = data.pull("customers");
  duplicate.add({nx::take("id"), nx::take("id")});
  print_error("duplicate", [&] { (void)duplicate.collect(); });

  heading("add copies specs");
  auto source_spec = nx::take("id", "customer_id");
  auto copied = data.pull("customers");
  copied.add(source_spec);
  source_spec.source = "name";
  source_spec.alias = "changed";
  print_specs(copied.specs());

  heading("specs borrowed state");
  auto inspected = data.pull("customers");
  inspected.take("id");
  const auto& spec_view = inspected.specs();
  std::cout << "before=" << spec_view.size() << "\n";
  inspected.take("name");
  std::cout << "after=" << spec_view.size() << "\n";
  print_specs(spec_view);

  heading("specs fresh and populated");
  auto spec_plan = data.pull("customers");
  std::cout << "empty specs=" << spec_plan.specs().size() << "\n";
  spec_plan.take("id").sum("events.spend", "spent");
  print_specs(spec_plan.specs());

  heading("specs all builder spellings");
  auto mixed_plan = data.pull("customers");
  mixed_plan.take("id");
  mixed_plan.add(nx::take("name", "customer"));
  mixed_plan.agg("events").sum("spend", "spent").count("event_id", "events").end();
  print_specs(mixed_plan.specs());

  heading("options replace state");
  auto option_plan = data.pull("customers").take("id");
  PullOptions relaxed;
  relaxed.require_primary_key = false;
  auto* option_same = &option_plan.options(relaxed);
  std::cout << "same_builder=" << (option_same == &option_plan) << " specs=" << option_plan.specs().size() << "\n";
  PullOptions strict;
  option_plan.options(strict);
  print_numbers(option_plan.collect(), "id");

  heading("options primary key");
  const auto no_meta = shop(false, false);
  print_error("strict", [&] { (void)no_meta.pull("customers").sum("events.spend", "spent").collect(); });
  auto relaxed_plan = no_meta.pull("customers");
  relaxed_plan.options(relaxed).sum("events.spend", "spent");
  print_numbers(relaxed_plan.collect(), "spent");

  heading("options ragged materialize");
  const auto ragged = shop(true);
  const auto preserved = ragged.pull("customers").take({"id", "name"}).collect();
  std::cout << "preserve id=" << preserved.keys("id").size()
            << " name=" << preserved.keys("name").size() << " rows=" << preserved.maxrows() << "\n";
  PullOptions pad;
  pad.ragged.materialize = pluss::bundle::locus::RaggedMaterialize::Pad;
  auto padded_plan = ragged.pull("customers");
  padded_plan.options(pad).take({"id", "name"});
  const auto padded = padded_plan.collect();
  print_words(padded, "name");

  heading("options tail alignment");
  PullOptions tail;
  tail.ragged.alignment = pluss::bundle::locus::RaggedAlignment::Tail;
  auto tail_plan = ragged.pull("customers");
  tail_plan.options(tail).take({"id", "name"});
  const auto tail_result = tail_plan.collect();
  print_numbers(tail_result, "id");
  print_words(tail_result, "name");

  heading("options domains");
  PullOptions shortest;
  shortest.ragged.domain = pluss::bundle::locus::RaggedDomain::Shortest;
  auto short_plan = ragged.pull("customers");
  short_plan.options(shortest).take({"id", "name"});
  const auto cut = short_plan.collect();
  std::cout << "shortest id=" << cut.keys("id").size() << " rows=" << cut.maxrows() << "\n";
  PullOptions exact;
  exact.ragged.domain = pluss::bundle::locus::RaggedDomain::Exact;
  auto exact_plan = ragged.pull("customers");
  exact_plan.options(exact).take({"id", "name"});
  print_error("exact", [&] { (void)exact_plan.collect(); });
  PullOptions no_loss = shortest;
  no_loss.ragged.loss = pluss::bundle::locus::RaggedLoss::Error;
  auto no_loss_plan = ragged.pull("customers");
  no_loss_plan.options(no_loss).take({"id", "name"});
  print_error("loss", [&] { (void)no_loss_plan.collect(); });

  heading("options invalid pull modes");
  PullOptions label;
  label.ragged.alignment = pluss::bundle::locus::RaggedAlignment::Label;
  auto label_plan = data.pull("customers");
  label_plan.options(label).take("id");
  std::cout << "label plan built\n";
  print_error("label", [&] { (void)label_plan.collect(); });
  PullOptions skip;
  skip.ragged.absent = pluss::bundle::locus::RaggedAbsent::Skip;
  auto skip_plan = data.pull("customers");
  skip_plan.options(skip).take("id");
  print_error("absent", [&] { (void)skip_plan.collect(); });
  PullOptions anchor;
  anchor.ragged.domain = pluss::bundle::locus::RaggedDomain::Anchor;
  anchor.ragged.anchor_stream = "id";
  auto anchor_plan = data.pull("customers");
  anchor_plan.options(anchor).take("id");
  print_error("anchor", [&] { (void)anchor_plan.collect(); });
  PullOptions propagate;
  propagate.ragged.nulls = pluss::bundle::locus::RaggedNulls::Propagate;
  auto propagate_plan = data.pull("customers");
  propagate_plan.options(propagate).take("id");
  print_error("nulls", [&] { (void)propagate_plan.collect(); });

  heading("collect output order and aliases");
  const auto ordered = data.pull("customers")
                           .take("regions.zone")
                           .take("id", "customer_id")
                           .sum("events.spend", "spent")
                           .collect();
  print_keys(ordered);

  heading("collect repeated and extended");
  auto reusable = data.pull("customers").take("id");
  print_keys(reusable.collect());
  print_keys(reusable.collect());
  reusable.take("name");
  print_keys(reusable.collect());

  heading("collect failures");
  print_error("empty", [&] { (void)data.pull("customers").collect(); });
  print_error("alias", [&] {
    (void)data.pull("customers").take("id", "value").take("name", "value").collect();
  });
  print_error("unknown table", [&] { (void)data.pull("customers").take("ghost.value").collect(); });

  heading("direct reducer values");
  const auto reduced = data.pull("customers")
                           .take("id")
                           .count("events.spend", "n")
                           .ndistinct("events.spend", "distinct_spend")
                           .sum("events.spend", "total")
                           .mean("events.spend", "average")
                           .median("events.spend", "middle")
                           .min("events.spend", "low")
                           .max("events.spend", "high")
                           .std("events.spend", "sample_sd")
                           .var("events.spend", "sample_var")
                           .collect();
  for (const auto key : {"n", "distinct_spend", "total", "average", "middle",
                         "low", "high", "sample_sd", "sample_var"}) {
    print_numbers(reduced, key);
  }

  heading("counting stream kinds");
  const auto counted = data.pull("customers")
                           .count("events.tag", "tag_count")
                           .ndistinct("events.tag", "tag_kinds")
                           .count("events.at", "time_count")
                           .ndistinct("events.at", "time_kinds")
                           .collect();
  for (const auto key : {"tag_count", "tag_kinds", "time_count", "time_kinds"}) {
    print_numbers(counted, key);
  }

  heading("invalid numeric values");
  const auto cleaned = data.pull("customers")
                           .count("events.reading", "usable")
                           .ndistinct("events.reading", "distinct")
                           .sum("events.reading", "total")
                           .collect();
  print_numbers(cleaned, "usable");
  print_numbers(cleaned, "distinct");
  print_numbers(cleaned, "total");

  heading("reducer default aliases");
  const auto defaults = data.pull("customers")
                            .count("events.event_id")
                            .ndistinct("events.tag")
                            .sum("events.spend")
                            .mean("events.spend")
                            .median("events.spend")
                            .min("events.spend")
                            .max("events.spend")
                            .std("events.spend")
                            .var("events.spend")
                            .collect();
  print_keys(defaults);

  heading("scoped scalar and batch reducers");
  auto scoped_plan = data.pull("customers").take("id");
  auto event_scope = scoped_plan.agg("events");
  auto* same_scope = &event_scope.count("event_id", "events");
  event_scope.sum({{"spend", "spent"}, {"reading", "reading_total"}});
  auto* same_owner = &event_scope.end();
  std::cout << "same_scope=" << (same_scope == &event_scope)
            << " same_owner=" << (same_owner == &scoped_plan)
            << " specs=" << scoped_plan.specs().size() << "\n";
  const auto scoped_result = scoped_plan.collect();
  print_keys(scoped_result);
  print_numbers(scoped_result, "events");
  print_numbers(scoped_result, "spent");
  print_numbers(scoped_result, "reading_total");

  heading("scoped scalar reducer family");
  auto scalar_plan = data.pull("customers");
  scalar_plan.agg("events")
      .count("spend", "n")
      .ndistinct("spend", "distinct_spend")
      .sum("spend", "total")
      .mean("spend", "average")
      .median("spend", "middle")
      .min("spend", "low")
      .max("spend", "high")
      .std("spend", "sample_sd")
      .var("spend", "sample_var")
      .end();
  const auto scalar_result = scalar_plan.collect();
  for (const auto key : {"n", "distinct_spend", "total", "average", "middle",
                         "low", "high", "sample_sd", "sample_var"}) {
    print_numbers(scalar_result, key);
  }

  heading("scoped batch reducer family");
  auto batch_plan = data.pull("customers");
  batch_plan.agg("events")
      .count({{"spend", "n"}})
      .ndistinct({{"spend", "distinct_spend"}})
      .sum({{"spend", "total"}})
      .mean({{"spend", "average"}})
      .median({{"spend", "middle"}})
      .min({{"spend", "low"}})
      .max({{"spend", "high"}})
      .std({{"spend", "sample_sd"}})
      .var({{"spend", "sample_var"}})
      .end();
  const auto batch_result = batch_plan.collect();
  print_keys(batch_result);
  for (const auto key : {"n", "distinct_spend", "total", "average", "middle",
                         "low", "high", "sample_sd", "sample_var"}) {
    print_numbers(batch_result, key);
  }

  heading("scoped empty batch");
  auto empty_batch = data.pull("customers").take("id");
  auto empty_scope = empty_batch.agg("events");
  empty_scope.sum(std::initializer_list<std::pair<std::string_view, std::string_view>>{});
  std::cout << "specs=" << empty_scope.end().specs().size() << "\n";

  heading("windowed counting reducers");
  auto count_window = data.pull("customers");
  count_window.add(nx::count("events.event_id", "latest_7d").windowed(7, "d"));
  count_window.add(nx::ndistinct("events.tag", "tag_kinds_7d").windowed(7, "d"));
  count_window.add(nx::count("events.event_id", "before_signup").root_windowed(7, "d"));
  const auto count_window_result = count_window.collect();
  print_numbers(count_window_result, "latest_7d");
  print_numbers(count_window_result, "tag_kinds_7d");
  print_numbers(count_window_result, "before_signup");

  heading("windowed reducer family");
  auto reducer_window = data.pull("customers");
  reducer_window.add(nx::sum("events.spend", "total_7d").windowed(7, "d"));
  reducer_window.add(nx::mean("events.spend", "mean_7d").windowed(7, "d"));
  reducer_window.add(nx::median("events.spend", "median_7d").windowed(7, "d"));
  reducer_window.add(nx::min("events.spend", "min_7d").windowed(7, "d"));
  reducer_window.add(nx::max("events.spend", "max_7d").windowed(7, "d"));
  reducer_window.add(nx::std("events.spend", "std_7d").windowed(7, "d"));
  reducer_window.add(nx::var("events.spend", "var_7d").windowed(7, "d"));
  const auto reducer_window_result = reducer_window.collect();
  for (const auto key : {"total_7d", "mean_7d", "median_7d", "min_7d",
                         "max_7d", "std_7d", "var_7d"}) {
    print_numbers(reducer_window_result, key);
  }

  heading("reducer failures");
  print_error("empty scope", [&] { (void)data.pull("customers").agg(""); });
  print_error("empty feature", [&] {
    auto plan = data.pull("customers");
    (void)plan.agg("events").sum("");
  });
  print_error("unknown feature", [&] {
    (void)data.pull("customers").sum("events.absent", "value").collect();
  });
  print_error("text sum", [&] {
    (void)data.pull("customers").sum("events.tag", "value").collect();
  });
  print_error("text mean", [&] {
    (void)data.pull("customers").mean("events.tag", "value").collect();
  });
  print_error("text median", [&] {
    (void)data.pull("customers").median("events.tag", "value").collect();
  });
  print_error("text min", [&] {
    (void)data.pull("customers").min("events.tag", "value").collect();
  });
  print_error("text max", [&] {
    (void)data.pull("customers").max("events.tag", "value").collect();
  });
  print_error("text std", [&] {
    (void)data.pull("customers").std("events.tag", "value").collect();
  });
  print_error("text var", [&] {
    (void)data.pull("customers").var("events.tag", "value").collect();
  });
  print_error("root aggregate", [&] {
    (void)data.pull("customers").sum("name", "value").collect();
  });
  print_error("unit overflow", [&] {
    auto plan = data.pull("customers");
    plan.add(nx::sum("events.spend", "value")
                 .windowed(std::numeric_limits<std::int64_t>::max(), "w"));
    (void)plan.collect();
  });
  print_error("nanosecond overflow", [&] {
    auto plan = data.pull("customers");
    plan.add(nx::sum("events.spend", "value")
                 .windowed(std::numeric_limits<std::int64_t>::max(), "s"));
    (void)plan.collect();
  });
}
