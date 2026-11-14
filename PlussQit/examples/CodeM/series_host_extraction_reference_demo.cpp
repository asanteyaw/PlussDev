#include <pluss/pluss.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

template <class T>
void print_vector(const std::vector<T>& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    if constexpr (std::is_same_v<T, bool>) {
      std::cout << (values[i] ? "true" : "false");
    } else {
      std::cout << values[i];
    }
  }
  std::cout << ']';
}

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << '\n';
}

}  // namespace

int main() {
  const auto scalar = pluss::make_series(42);
  const auto vector = pluss::make_series({2, 4, 8});
  const auto matrix = pluss::make_series({1, 2, 3, 4}, {2, 2});

  std::cout << "== item ==\n";
  std::cout << scalar.item<std::int64_t>() << "\n\n";

  std::cout << "== items ==\n";
  print_vector(vector.items<std::int64_t>());
  std::cout << "\n\n";

  std::cout << "== host ==\n";
  const auto exported = matrix.host<std::int64_t>();
  std::cout << "rank=" << exported.array().sizes.size() << " shape=";
  print_vector(exported.array().sizes);
  std::cout << " data=";
  print_vector(exported.array().data);
  std::cout << "\n\n";

  std::cout << "== item rejects rank one ==\n";
  try {
    static_cast<void>(vector.item<std::int64_t>());
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << '\n';

  std::cout << "== items rejects rank zero ==\n";
  try {
    static_cast<void>(scalar.items<std::int64_t>());
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << '\n';

  std::cout << "== items rejects rank two ==\n";
  try {
    static_cast<void>(matrix.items<std::int64_t>());
  } catch (const std::exception& error) {
    print_error(error);
  }
}
