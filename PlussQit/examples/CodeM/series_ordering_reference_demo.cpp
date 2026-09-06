#include <pluss/pluss.h>

#include <iostream>
#include <string>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

void print_error(const std::exception& error) {
  const std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

}  // namespace

int main() {
  auto scores = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
  auto table = pluss::make_series({
      {4.0, 1.0, 3.0},
      {2.0, 5.0, 1.0},
  });

  heading("sort_default");
  pluss::series::print(std::cout, scores.sort());
  std::cout << "\n\n";

  heading("sort_descending");
  std::cout << scores.sort(0, true) << "\n\n";

  heading("sort_dim0");
  std::cout << table.sort(0) << "\n\n";

  heading("sort_dim1");
  std::cout << table.sort(-1) << "\n\n";

  auto stable_input = pluss::make_series({2LL, 1LL, 2LL, 1LL});
  heading("sort_stable_pair");
  const auto sort_pair = stable_input.sort_pair(0, false, true);
  std::cout << "values=" << sort_pair.first << "\n";
  std::cout << "indices=" << sort_pair.second << "\n\n";

  heading("sort_bad_dim_error");
  try {
    std::cout << scores.sort(1) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("argsort_default");
  pluss::series::print(std::cout, scores.argsort());
  std::cout << "\n\n";

  heading("argsort_descending");
  std::cout << scores.argsort(0, true) << "\n\n";

  heading("argsort_dim0");
  std::cout << table.argsort(0) << "\n\n";

  heading("argsort_dim1");
  std::cout << table.argsort(1) << "\n\n";

  heading("argsort_stable");
  std::cout << stable_input.argsort(0, false, true) << "\n\n";

  heading("argsort_apply_order");
  auto order = scores.argsort();
  std::cout << scores.index_select(0, order) << "\n\n";

  heading("argsort_bad_dim_error");
  try {
    std::cout << scores.argsort(1) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("topk_default");
  std::cout << scores.topk(3) << "\n\n";

  heading("topk_smallest");
  std::cout << scores.topk(3, 0, false, true) << "\n\n";

  heading("topk_dim0");
  std::cout << table.topk(1, 0) << "\n\n";

  heading("topk_unsorted");
  std::cout << scores.topk(3, 0, true, false) << "\n\n";

  heading("topk_pair");
  const auto topk_pair = scores.topk_pair(3, 0, true, true);
  std::cout << "values=" << topk_pair.first << "\n";
  std::cout << "indices=" << topk_pair.second << "\n\n";

  heading("topk_zero");
  std::cout << scores.topk(0) << "\n\n";

  heading("kthvalue_default");
  std::cout << scores.kthvalue(3) << "\n\n";

  heading("kthvalue_endpoints");
  std::cout << scores.kthvalue(1) << "\n";
  std::cout << scores.kthvalue(5) << "\n\n";

  heading("kthvalue_dim0");
  std::cout << table.kthvalue(2, 0) << "\n\n";

  heading("kthvalue_dim1_keepdim");
  std::cout << table.kthvalue(2, 1, true) << "\n\n";

  heading("topk_bad_k_error");
  try {
    std::cout << scores.topk(6) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n";

  heading("kthvalue_bad_k_error");
  try {
    std::cout << scores.kthvalue(0) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
  std::cout << "\n\n";

  heading("kthvalue_too_large_error");
  try {
    std::cout << scores.kthvalue(6) << "\n";
  } catch (const std::exception& error) {
    print_error(error);
  }
}
