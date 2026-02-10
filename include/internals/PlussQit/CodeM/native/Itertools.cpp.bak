#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/TensorOperators.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/arange.h>
#include <PlussQit/ops/cartesian_prod_native.h>
#include <PlussQit/ops/combinations_native.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/full.h>
#include <PlussQit/ops/meshgrid.h>
#include <PlussQit/ops/stack.h>
#endif

#include <vector>

namespace {

using namespace pluss;

Series _triu_mask(int64_t n, int64_t dims, bool diagonal, TensorOptions opt) {
  // get a mask that has value 1 whose indices satisfies i < j < k < ...
  // or i <= j <= k <= ... (depending on diagonal)
  Series range = pluss::arange(n, opt.dtype(kLong));
  std::vector<Series> index_grids = pluss::meshgrid(std::vector<Series>(dims, range), "ij");
  Series mask = pluss::full(index_grids[0].sizes(), true, opt.dtype(kBool));
  if(diagonal) {
    for(int64_t i = 0; i < dims - 1; i++) {
      mask *= index_grids[i] <= index_grids[i+1];
    }
  } else {
    for(int64_t i = 0; i < dims - 1; i++) {
      mask *= index_grids[i] < index_grids[i+1];
    }
  }
  return mask;
}

}  // namespace

namespace pluss::native {

Series cartesian_prod(TensorList tensors) {
  for(const Series &t : tensors) {
    TORCH_CHECK(t.dim() == 1, "Expect a 1D vector, but got shape ", t.sizes());
  }
  if (tensors.size() == 1) {
    return tensors[0];
  }
  std::vector<Series> grids = pluss::meshgrid(tensors, "ij");
  for(Series &t : grids) {
    t = t.flatten();
  }
  return pluss::stack(grids, 1);
}

Series combinations(const Series& self, int64_t r, bool with_replacement) {
  TORCH_CHECK(self.dim() == 1, "Expect a 1D vector, but got shape ", self.sizes());
  TORCH_CHECK(r >= 0, "Expect a non-negative number, but got ", r);
  if (r == 0) {
    return pluss::empty({0}, self.options());
  }
  int64_t num_elements = self.numel();
  std::vector<Series> grids = pluss::meshgrid(std::vector<Series>(r, self), "ij");
  Series mask = _triu_mask(num_elements, r, with_replacement, self.options());
  for(Series &t : grids) {
    t = t.masked_select(mask);
  }
  return pluss::stack(grids, 1);
}

}  // namespace pluss::native
