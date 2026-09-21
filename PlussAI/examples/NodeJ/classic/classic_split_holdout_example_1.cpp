#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::IndexSplit split = cls::holdout_split(10).value();
  std::cout << "train count = " << split.train_indices.numel() << "\n";
  std::cout << "test count  = " << split.test_indices.numel() << "\n";
  std::cout << "test_indices = " << split.test_indices << "\n";

  return 0;
}
