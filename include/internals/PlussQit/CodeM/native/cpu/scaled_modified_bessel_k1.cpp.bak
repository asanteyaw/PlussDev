#define TORCH_ASSERT_NO_OPERATORS

#include <PlussQit/native/UnaryOps.h>

#include <PlussQit/Dispatch.h>
#include <PlussQit/native/Math.h>
#include <PlussQit/native/TensorIterator.h>
#include <PlussQit/native/cpu/Loops.h>

namespace pluss::native {
inline namespace CPU_CAPABILITY {
    static void scaled_modified_bessel_k1_kernel(TensorIteratorBase& iterator) {
        TORCH_INTERNAL_ASSERT(iterator.ntensors() == 2);

        AT_DISPATCH_FLOATING_TYPES(iterator.common_dtype(), "scaled_modified_bessel_k1_cpu", [&]() {
            cpu_kernel(iterator, [](scalar_t x) {
                return scaled_modified_bessel_k1_forward(x);
            });
        });
    } // scaled_modified_bessel_k1_kernel(TensorIteratorBase& iterator)
} // namespace CPU_CAPABILITY

REGISTER_DISPATCH(special_scaled_modified_bessel_k1_stub, &CPU_CAPABILITY::scaled_modified_bessel_k1_kernel)
} // namespace pluss::native
