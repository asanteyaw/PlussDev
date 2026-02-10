#include <PlussQit/native/DispatchStub.h>
#include <MSharp/core/Scalar.h>

namespace pluss {
struct TensorIterator;

namespace native {

DECLARE_DISPATCH(void(*)(TensorIterator&, const Scalar&, const Scalar&, const Scalar&), arange_stub)
DECLARE_DISPATCH(void(*)(TensorIterator&, const Scalar&, const Scalar&, int64_t), linspace_stub)

}}  // namespace pluss::native
