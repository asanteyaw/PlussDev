// core::Status example 1 -- every named factory, plus ok()/code/message.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Status ok = cls::Status::success();
  const cls::Status bad_arg = cls::Status::invalid_argument("feature_names must not be empty");
  const cls::Status precond = cls::Status::precondition("learn() requires an untrained estimator");
  const cls::Status shape = cls::Status::shape_mismatch("y.numel() must equal x.sizes().front()");
  const cls::Status unimpl = cls::Status::not_implemented("dump_state() is not implemented for this estimator");
  const cls::Status numeric = cls::Status::numerical_failure("Cholesky factorization encountered a non-finite pivot");
  const cls::Status internal = cls::Status::internal("unreachable branch in family dispatch");

  const cls::Status* all[] = {&ok, &bad_arg, &precond, &shape, &unimpl, &numeric, &internal};
  for (const cls::Status* s : all) {
    std::cout << "code = " << static_cast<int>(s->code) << ", ok = " << std::boolalpha << s->ok()
              << ", message = \"" << s->message << "\"\n";
  }

  // Status{} default-constructs to the same thing success() returns.
  const cls::Status defaulted{};
  std::cout << "default-constructed ok = " << defaulted.ok() << "\n";

  return 0;
}
