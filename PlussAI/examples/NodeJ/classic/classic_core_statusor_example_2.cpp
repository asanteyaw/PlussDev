// core::StatusOr example 2 -- StatusOr<T>'s three constructor forms, and the
// one sharp edge: value() never checks ok() -- it always returns whatever T
// it holds, silently, even on a failure StatusOr.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  // 1) explicit(T) -- success, status defaults to Status::success().
  const cls::StatusOr<int> from_value(42);
  std::cout << "from_value.ok() = " << std::boolalpha << from_value.ok()
            << ", value = " << from_value.value() << "\n";

  // 2) explicit(Status) -- failure, T default-constructs (int -> 0).
  const cls::StatusOr<int> from_status(cls::Status::invalid_argument("no candidates"));
  std::cout << "from_status.ok() = " << from_status.ok()
            << ", value() returns = " << from_status.value() << "  <- default T, not a crash\n";

  // 3) (Status, T) -- has_value() tracks status.ok(), but the T is stored either way.
  const cls::StatusOr<int> mismatched(cls::Status::invalid_argument("ignored on purpose"), 7);
  std::cout << "mismatched.ok() = " << mismatched.ok()
            << ", value() returns = " << mismatched.value() << "  <- still 7, ok() lied to you if unchecked\n";

  const cls::StatusOr<int> matched(cls::Status::success(), 9);
  std::cout << "matched.ok() = " << matched.ok() << ", value() = " << matched.value() << "\n";

  // The only safe pattern: branch on ok() before reading value().
  if (!from_status.ok()) {
    std::cout << "correctly handled failure: " << from_status.status().message << "\n";
  }

  return 0;
}
