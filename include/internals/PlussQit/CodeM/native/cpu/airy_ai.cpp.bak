#define TORCH_ASSERT_NO_OPERATORS

#include <PlussQit/native/UnaryOps.h>

#include <PlussQit/Dispatch.h>
#include <PlussQit/native/Math.h>
#include <PlussQit/native/TensorIterator.h>
#include <PlussQit/native/cpu/Loops.h>

namespace pluss::native {
inline namespace CPU_CAPABILITY {
static void airy_ai_kernel(TensorIteratorBase& iterator) {
    TORCH_INTERNAL_ASSERT(iterator.ntensors() == 2);

    AT_DISPATCH_FLOATING_TYPES(iterator.common_dtype(), "airy_ai_cpu", [&]() {
        cpu_kernel(iterator, [](scalar_t x) {
            return airy_ai_forward(x);
        });
    });
} // airy_ai_kernel(TensorIteratorBase& iterator)
} // namespace CPU_CAPABILITY

REGISTER_DISPATCH(special_airy_ai_stub, &CPU_CAPABILITY::airy_ai_kernel)
} // namespace pluss::native
