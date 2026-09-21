#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& note) {
  std::cout << label << '\n' << pluss::note::print(note) << '\n';
}

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

} // namespace

int main() {
  const auto names = pluss::note::make({"Ada", "Grace", "Linus"});
  const auto punctuation = pluss::note::make({"!", "?", "."});

  std::cout << "=== concatenation ===\n";
  show("paired", names + punctuation);
  show("suffix literal", names + "!");
  show("prefix literal", "Dr. " + names);
  show("one-element Note broadcast", names + pluss::note::make({"."}));

  auto assigned = names;
  assigned += punctuation;
  show("plus-equals Note", assigned);
  assigned += "#";
  show("plus-equals literal", assigned);
  show("original after plus-equals", names);

  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix shape", matrix + pluss::note::make("!"));
  const auto raw = pluss::note::bytes(
      std::vector<std::string>{std::string("A\0", 2), "B"},
      std::array<std::int64_t, 1>{2});
  const auto raw_joined = raw + pluss::note::bytes("!");
  std::cout << "bytes=" << std::boolalpha << raw_joined.is_bytes()
            << " sizes=" << raw_joined.at(0).size() << ','
            << raw_joined.at(1).size() << '\n';
  rejected("concat shape mismatch", [&] {
    (void)(matrix + pluss::note::make({"1", "2", "3", "4"}));
  });
  rejected("concat mixed encoding", [&] { (void)(names + raw); });
  rejected("concat native dtype", [&] {
    (void)(pluss::note::make(std::vector<std::int64_t>{1, 2}) +
           pluss::note::make(std::vector<std::int64_t>{3, 4}));
  });
  const auto empty_text = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  show("concat empty", empty_text + empty_text);
  rejected("concat undefined", [&] {
    (void)(names + pluss::Note::undefined());
  });

  std::cout << "\n=== repetition ===\n";
  show("right multiply", pluss::note::make({"ha", "go"}) * 3);
  show("left multiply", 2 * pluss::note::make({"ab", "x"}));
  show("zero", matrix * 0);
  auto repeated = pluss::note::make({"na", "batman"});
  repeated *= 2;
  show("times-equals", repeated);
  rejected("negative repeat", [&] { (void)(names * -1); });
  rejected("native repeat", [&] {
    (void)(pluss::note::make(std::vector<bool>{true, false}) * 2);
  });

  const auto lhs = pluss::note::make({"a", "b", "c"});
  const auto rhs = pluss::note::make({"a", "x", "b"});
  std::cout << "\n=== comparisons ===\n";
  show("equal", lhs == rhs);
  show("not equal", lhs != rhs);
  show("less", lhs < rhs);
  show("less equal", lhs <= rhs);
  show("greater", lhs > rhs);
  show("greater equal", lhs >= rhs);
  show("literal right", lhs >= "b");
  show("literal left", "b" < lhs);
  show("scalar Note left", pluss::note::make("b") <= lhs);
  show("matrix comparison", matrix > pluss::note::make("b"));

  const auto ints = pluss::note::make(std::vector<std::int64_t>{2, 10, -1});
  const auto int_limit = pluss::note::make(std::int64_t{2});
  show("Int64 comparison", ints >= int_limit);
  const auto reals = pluss::note::make(
      std::vector<double>{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0});
  show("Float64 NaN equality", reals == reals);
  const auto bools = pluss::note::make(std::vector<bool>{false, true, true});
  show("Bool ordering", bools > pluss::note::make(false));
  show("Bytes equality", raw == raw);
  show("empty comparison", empty_text == empty_text);
  rejected("comparison mixed dtype", [&] { (void)(ints == reals); });
  rejected("comparison shape mismatch", [&] {
    (void)(matrix == pluss::note::make({"a", "b", "c", "d"}));
  });
  rejected("string against Int64", [&] { (void)(ints == "2"); });
  rejected("comparison undefined", [&] {
    (void)(lhs == pluss::Note::undefined());
  });

  const auto has_a = names.contains("a");
  const auto is_long = names.lengths_chars() >= pluss::note::make(std::int64_t{5});
  std::cout << "\n=== Bool composition ===\n";
  show("and", has_a & is_long);
  show("or", has_a | is_long);
  show("xor", has_a ^ is_long);
  show("not", ~has_a);
  show("broadcast true", has_a & pluss::note::make(true));
  rejected("logical non-Bool", [&] { (void)(has_a & names); });
  rejected("logical shape mismatch", [&] {
    (void)(has_a & pluss::note::make(
        std::vector<bool>{true, false, true},
        std::array<std::int64_t, 2>{1, 3}));
  });

  std::cout << "\n=== reductions ===\n";
  show("any mixed", pluss::note::make({true, false, false}).any());
  show("any false", pluss::note::make({false, false}).any());
  show("all true", pluss::note::make({true, true}).all());
  show("all mixed", pluss::note::make({true, false}).all());
  const auto bool_matrix = pluss::note::make(
      std::vector<bool>{true, true, false, true},
      std::array<std::int64_t, 2>{2, 2});
  show("matrix any", bool_matrix.any());
  show("matrix all", bool_matrix.all());
  const auto empty = pluss::note::make(
      std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
  show("empty any", empty.any());
  show("empty all", empty.all());
  show("scalar any", pluss::note::make(true).any());
  show("scalar all", pluss::note::make(false).all());
  const auto selected = names.starts_with("A") | names.ends_with("s");
  show("compound any mask", selected);
  show("compound any", selected.any());
  const auto acceptable = names.contains("a") |
                          (names.lengths_chars() >=
                           pluss::note::make(std::int64_t{5}));
  show("compound all mask", acceptable);
  show("compound all", acceptable.all());
  rejected("any Text", [&] { (void)names.any(); });
  rejected("all Text", [&] { (void)names.all(); });
  rejected("any undefined", [&] { (void)pluss::Note::undefined().any(); });
  rejected("all undefined", [&] { (void)pluss::Note::undefined().all(); });

  std::cout << "\n=== stream insertion ===\n" << names << '\n';
}
