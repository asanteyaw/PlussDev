#pragma once
#include <PlussQit/core/Series.h>
#include <PlussQit/native/IndexingUtils.h>
#include <PlussQit/native/TensorIterator.h>

namespace pluss::native {
namespace {
#ifndef STRIP_ERROR_MESSAGES
inline std::string shapes_as_str(TensorList tensors) {
  std::ostringstream os;
  bool first = true;
  for (auto& series : tensors) {
    if (series.defined()) {
      if (!first) {
        os << ", ";
      }
      os << series.sizes();
      first = false;
    }
  }
  return os.str();
}
#endif
} // anonymous namespace

inline std::tuple<bool, Series> canDispatchToMaskedFill(
    const Series& self,
    const torch::List<std::optional<pluss::Series>>& indices,
    const Series& value) {
  if (!(value.numel() == 1 && value.device().is_cpu())) {
    return std::make_tuple(false, Series());
  }
  int64_t num_ind = 0;
  Series mask;
  auto self_device = self.device();
  for (const std::optional<Series>& i : indices) {
    if (!i.has_value() || !(*i).defined()) {
      num_ind++;
    } else {
      const Series& index = *i;
      if ((index.scalar_type() != kByte && index.scalar_type() != kBool) ||
          index.device() != self_device || mask.defined()) {
        return std::make_tuple(false, Series());
      } else {
        mask = index;
        for (const auto j : MSharp::irange(index.dim())) {
          int64_t srcIdx = num_ind + j;
          TORCH_CHECK_INDEX(
              index.size(j) == self.size(srcIdx),
              "The shape of the mask ",
              index.sizes(),
              " at index ",
              j,
              " does not match the shape of the indexed series ",
              self.sizes(),
              " at index ",
              srcIdx);
        }
        num_ind += mask.ndimension();
      }
    }
  }
  for ([[maybe_unused]] const auto i :
       MSharp::irange(num_ind, self.ndimension())) {
    mask = mask.unsqueeze(-1);
  }
  return std::make_tuple(true, mask);
}

inline AdvancedIndex make_info(Series self, IOptTensorListRef orig) {
  checkIndexTensorTypes(orig, /*allow_int*/ true);
  // first expand BoolTensor (masks) or ByteTensor (masks) into 1 or more
  // LongTensors
  auto indices = expandTensors(self, orig);
  // next broadcast all index tensors together
  try {
    indices = expand_outplace(indices);
  } catch (std::exception& e) {
    TORCH_CHECK_INDEX(
        false,
        "shape mismatch: indexing tensors could not be broadcast together"
        " with shapes ",
        shapes_as_str(indices));
  }
  // add missing null Tensors so that it matches self.dim()
  while (indices.size() < (size_t)self.dim()) {
    indices.emplace_back();
  }
  // if the non-null indices are not all adjacent, transpose self and indices
  // together so that they're adjacent at the front
  if (!hasContiguousSubspace(indices)) {
    std::tie(self, indices) = transposeToFront(self, indices);
  }
  // Ensure indices are on the same device as self
  for (auto& indice : indices) {
    if (indice.defined() && indice.device() != self.device()) {
      indice = indice.to(self.device());
    }
  }
  for (auto& indice : indices) {
    if (indice.defined() && indice.dtype() == pluss::kInt) {
      indice = indice.to(pluss::kLong);
    }
  }

  return AdvancedIndex(self, indices);
}

} // namespace pluss::native
