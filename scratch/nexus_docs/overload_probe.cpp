// Scratch probe: confirms how Nexus::select's three "multi-name" overloads
// (initializer_list, RangeView::Of via implicit conversion, and the
// templated StringViewNameRange concept overload) actually resolve for
// different call-site forms, before writing any doc examples.
#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <iostream>
#include <set>
#include <vector>

int main() {
  using namespace pluss::table;
  using namespace pluss::bundle;

  auto customers = DataFrame({
      {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
  });
  auto orders = DataFrame({
      {"order_id", pluss::make_series({10.0, 11.0})},
  });
  auto payments = DataFrame({
      {"payment_id", pluss::make_series({100.0})},
  });

  auto biz = Nexus({
      {"customers", customers},
      {"orders", orders},
      {"payments", payments},
  });

  // Form 1: plain brace literal -- should resolve to the initializer_list
  // overload directly (best conversion sequence, no ambiguity).
  auto a = biz.select({"payments", "customers"});
  std::cout << "brace literal: ";
  for (auto& k : a.keys()) std::cout << k << " ";
  std::cout << "\n";

  // Form 2: a named std::vector<std::string_view> -- BasicRangeView has an
  // implicit ctor from vector<T, Alloc> (same T), so this should bind to
  // the RangeView::Of<std::string_view> overload without naming RangeView.
  std::vector<std::string_view> names = {"orders", "payments"};
  auto b = biz.select(names);
  std::cout << "vector<string_view>: ";
  for (auto& k : b.keys()) std::cout << k << " ";
  std::cout << "\n";

  // Form 3: a named std::vector<std::string> (owning strings, NOT
  // string_view) -- BasicRangeView's vector ctor requires the exact same
  // element type T, so vector<std::string> does NOT match RangeView::Of
  // <std::string_view> directly. It should instead satisfy the templated
  // StringViewNameRange concept (any input_range whose reference type
  // converts to string_view) and bind to the template overload.
  std::vector<std::string> owned_names = {"customers", "orders"};
  auto c = biz.select(owned_names);
  std::cout << "vector<string> (owning): ";
  for (auto& k : c.keys()) std::cout << k << " ";
  std::cout << "\n";

  // Form 4: a std::set<std::string_view> -- also an input_range with a
  // string_view-convertible reference type, not span, not
  // initializer_list -- should also hit the template overload, and proves
  // it isn't just "any vector," but genuinely any qualifying range.
  std::set<std::string_view> set_names = {"payments", "customers"};
  auto d = biz.select(set_names);
  std::cout << "set<string_view>: ";
  for (auto& k : d.keys()) std::cout << k << " ";
  std::cout << "\n";

  return 0;
}
