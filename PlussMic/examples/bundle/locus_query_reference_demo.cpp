#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::bundle::Locus;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;
using SqlOptions = pluss::table::DataFrame::SqlOptions;

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

Locus shop() {
  using Streams = std::vector<std::pair<std::string, Stream>>;
  Catalog customers(Streams{
      {"customer_id", nums({1, 2, 3})},
      {"name", words({"Ada", "Bola", "Chen"})},
  });
  Catalog orders(Streams{
      {"order_id", nums({100, 101, 102, 103})},
      {"customer_id", nums({1, 2, 2, 3})},
      {"amount", nums({50, 75, 20, 90})},
      {"state", words({"paid", "paid", "held", "void"})},
  });
  return Locus({{"customers", customers}, {"orders", orders}});
}

void heading(std::string_view title) { std::cout << "== " << title << " ==\n"; }

void print_numbers(const Catalog& catalog, std::string_view key) {
  const auto values = catalog.get(key)->as_series().to_vec<double>();
  std::cout << key << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << "]\n";
}

void print_words(const Catalog& catalog, std::string_view key) {
  const auto values = catalog.get(key)->as_note().vector();
  std::cout << key << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << "]\n";
}

void print_keys(const Catalog& catalog) {
  std::cout << "keys=[";
  for (std::size_t i = 0; i < catalog.keys().size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << catalog.keys()[i];
  }
  std::cout << "] rows=" << catalog.maxrows() << "\n";
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
  const auto data = shop();

  heading("from anchor");
  const auto traversed = data.from("orders").collect();
  print_keys(traversed);
  print_numbers(traversed, "order_id");

  heading("from owns workspace");
  const auto retained_walk = shop().from("orders");
  print_words(retained_walk.pick({"state"}).collect(), "state");

  heading("from failure");
  print_error("unknown", [&] { (void)data.from("absent"); });

  heading("pull root fields");
  const auto pulled = data.pull("orders").take({"order_id", "amount"}).collect();
  print_keys(pulled);
  print_numbers(pulled, "amount");

  heading("pull builder state");
  auto pull_plan = data.pull("orders");
  std::cout << "root=" << pull_plan.root() << " specs=" << pull_plan.specs().size() << "\n";
  pull_plan.take("state", "order_state");
  std::cout << "root=" << pull_plan.root() << " specs=" << pull_plan.specs().size() << "\n";
  print_words(pull_plan.collect(), "order_state");

  heading("pull failure");
  print_error("unknown", [&] { (void)data.pull("absent"); });

  heading("query default");
  auto default_query = data.query();
  const auto all = default_query.execute("SELECT order_id FROM orders ORDER BY order_id");
  print_numbers(all, "order_id");

  heading("query options");
  SqlOptions limited;
  limited.max_result_rows = 2;
  const auto first_two = data.query(limited).execute("SELECT order_id FROM orders ORDER BY order_id");
  print_numbers(first_two, "order_id");

  heading("query owns workspace");
  const auto retained = shop().query();
  print_words(retained.execute("SELECT name FROM customers ORDER BY customer_id"), "name");

  heading("execute filter");
  const auto filtered = data.query().execute(
      "SELECT order_id, amount FROM orders WHERE amount >= 50 ORDER BY amount DESC");
  print_numbers(filtered, "order_id");
  print_numbers(filtered, "amount");

  heading("execute join");
  const auto joined = data.query().execute(
      "SELECT customers.name AS customer, orders.amount AS amount "
      "FROM customers JOIN orders ON customers.customer_id = orders.customer_id "
      "WHERE orders.state = 'paid' ORDER BY orders.amount DESC");
  print_words(joined, "customer");
  print_numbers(joined, "amount");

  heading("execute empty");
  print_keys(data.query().execute("SELECT order_id FROM orders WHERE amount > 1000"));

  heading("execute failure");
  print_error("malformed", [&] { (void)data.query().execute("SELEKT order_id FROM orders"); });

  heading("prepare is lazy");
  const auto lazy = data.query().prepare("SELECT id FROM absent_table");
  std::cout << "prepared\n";
  print_error("on execute", [&] { (void)lazy.execute(); });

  heading("prepared no parameters");
  const auto fixed = data.query().prepare("SELECT order_id FROM orders ORDER BY order_id DESC LIMIT 2");
  print_numbers(fixed.execute(), "order_id");

  heading("prepared values reused");
  const auto threshold = data.query().prepare(
      "SELECT order_id FROM orders WHERE amount >= {floor} ORDER BY order_id");
  print_numbers(threshold.execute({{"floor", 50.0}}), "order_id");
  print_numbers(threshold.execute({{"floor", 80.0}}), "order_id");

  heading("prepared scalar kinds");
  const auto kinds = data.query().prepare(
      "SELECT order_id FROM orders "
      "WHERE state = {state} AND amount >= {floor} AND {enabled} AND {nothing} IS NULL "
      "ORDER BY order_id LIMIT {limit}");
  SqlOptions::SqlParam null_value = nullptr;
  print_numbers(kinds.execute({
      {"state", std::string("paid")},
      {"floor", 40.0},
      {"enabled", true},
      {"nothing", null_value},
      {"limit", std::int64_t{3}},
  }), "order_id");

  heading("prepared identifier");
  const auto projected = data.query().prepare(
      "SELECT {@field} FROM orders ORDER BY order_id LIMIT 2");
  const auto amounts = projected.execute({}, {{"field", "amount"}});
  print_keys(amounts);
  print_numbers(amounts, "amount");

  heading("prepared option override");
  SqlOptions defaults;
  defaults.params["floor"] = 80.0;
  const auto inherited = data.query(defaults).prepare(
      "SELECT order_id FROM orders WHERE amount >= {floor} ORDER BY order_id");
  print_numbers(inherited.execute(), "order_id");
  print_numbers(inherited.execute({{"floor", 50.0}}), "order_id");

  heading("prepared failures");
  print_error("missing", [&] { (void)threshold.execute(); });
  print_error("identifier", [&] {
    (void)projected.execute({}, {{"field", "amount; DROP TABLE orders"}});
  });
}
