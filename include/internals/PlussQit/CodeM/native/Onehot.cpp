#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/arange.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/eq.h>
#include <PlussQit/ops/one_hot_native.h>
#include <PlussQit/ops/zeros.h>
#endif

namespace pluss::native {

Series one_hot(const Series &self, int64_t num_classes) {
    TORCH_CHECK(self.dtype() == kLong, "one_hot is only applicable to index series of type LongTensor.");

    // using meta bit test to catch Fake Series as well until __torch_function__
    if (self.key_set().has_all(DispatchKeySet(BackendComponent::MetaBit)) ||
            self.key_set().has_all(DispatchKeySet(DispatchKey::Python))) {
        // functional version that torch.compiles better and works with dynamic shapes
        if (num_classes == -1) {
          num_classes = self.max().item().toLong() + 1;
        }
        pluss::Series index = pluss::arange(num_classes, self.options());
        return pluss::eq(self.unsqueeze(-1), index).to(kLong);
    }

    auto shape = self.sizes().vec();

    // empty series could be converted to one hot representation,
    // but shape inference is not possible.
    if (self.numel() == 0) {
        if (num_classes <= 0) {
            TORCH_CHECK(false, "Can not infer total number of classes from empty series.");
        } else {
            shape.push_back(num_classes);
            return pluss::empty(shape, self.options());
        }
    }

    // non-empty series
    if (self.device().type() != pluss::kCUDA && self.device().type() != pluss::kMPS &&
        self.device().type() != pluss::kPrivateUse1 && self.device().type() != pluss::kXLA) {
      // for cuda, rely on device assert thrown by scatter
      TORCH_CHECK(self.min().item().toLong() >= 0, "Class values must be non-negative.");
    }
    if (num_classes == -1) {
        num_classes = self.max().item().toLong() + 1;
    } else {
        if (self.device().type() != pluss::kCUDA && self.device().type() != pluss::kMPS &&
            self.device().type() != pluss::kPrivateUse1 && self.device().type() != pluss::kXLA) {
          // rely on device asserts from scatter to avoid sync here
          TORCH_CHECK(num_classes > self.max().item().toLong(), "Class values must be smaller than num_classes.");
        } else {
            //for cuda, assert that num_classes is at least 1
            TORCH_CHECK(num_classes >= 1, "num_classes should be positive");
        }
    }

    shape.push_back(num_classes);
    Series ret = pluss::zeros(shape, self.options());
    ret.scatter_(-1, self.unsqueeze(-1), 1);
    return ret;
}

} // namespace pluss::native
