#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_offsets(const pluss::TokenBatch& batch) {
  std::cout << "offsets=[";
  const auto values = batch.offsets().integers();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << (i == 0 ? "" : ", ") << values[i];
  }
  std::cout << "]\n";
}

void print_batch(const pluss::TokenBatch& batch) {
  std::cout << pluss::note::print(batch.tokens()) << '\n';
  print_offsets(batch);
}

template <class F>
void print_error(F&& call) {
  try {
    call();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }
}

} // namespace

int main() {
  const auto text = pluss::make_note({"A1,b22,,C333", "none"});

  std::cout << "=== match and search ===\n";
  std::cout << pluss::note::print(text.regex().match(R"([A-Z]\d,.*)")) << '\n';
  std::cout << pluss::note::print(text.regex().search(R"(\d+)")) << '\n';

  std::cout << "=== replace ===\n";
  std::cout << pluss::note::print(text.regex().replace(R"(\d+)", "#")) << '\n';
  std::cout << pluss::note::print(
                   pluss::make_note({"Alpha ALPHA"}).regex().replace("alpha", "x", 1))
            << '\n';

  std::cout << "=== findall ===\n";
  print_batch(text.regex().findall(R"(\d+)"));

  std::cout << "=== count ===\n";
  std::cout << pluss::note::print(text.regex().count(R"(\d+)")) << '\n';

  std::cout << "=== count shape ===\n";
  const auto grid = pluss::make_note(
      {{"id=7 id=42", "none"}, {"x9", "100 and 200"}});
  const auto grid_counts = grid.regex().count(R"(\d+)");
  std::cout << "shape=[" << grid_counts.size(0) << ", "
            << grid_counts.size(1) << "]\nvalues=[";
  for (std::size_t i = 0; i < grid_counts.integers().size(); ++i)
    std::cout << (i == 0 ? "" : ", ") << grid_counts.integers()[i];
  std::cout << "]\n";

  std::cout << "=== count non-overlap ===\n";
  std::cout << pluss::note::print(
                   pluss::make_note({"aaaa", "id=7 id=42"})
                       .regex().count(R"(id=(\d+)|aa)"))
            << '\n';

  std::cout << "=== split unlimited ===\n";
  print_batch(text.regex().split(","));

  std::cout << "=== split limit one ===\n";
  print_batch(text.regex().split(",", 1));

  std::cout << "=== split limit zero ===\n";
  print_batch(pluss::make_note({"a,b", "c;d"}).regex().split(R"([,;])", 0));

  std::cout << "=== split mixed delimiters ===\n";
  print_batch(pluss::make_note({"north, south;east"})
                  .regex().split(R"(\s*[,;]\s*)"));

  std::cout << "=== invalid pattern ===\n";
  print_error([&] { (void)text.regex().count("("); });
  print_error([&] { (void)text.regex().findall("("); });
  print_error([&] { (void)text.regex().match("("); });
  print_error([&] { (void)text.regex().replace("(", "x"); });
  print_error([&] { (void)text.regex().split("("); });

  std::cout << "=== invalid storage ===\n";
  print_error([&] {
    (void)pluss::note::make(std::vector<bool>{true, false})
        .regex().count("true");
  });
}
