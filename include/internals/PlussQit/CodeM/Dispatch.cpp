#include <PlussQit/Dispatch.h>
#if defined ENABLE_RECORD_KERNEL_FUNCTION_DTYPE
#include <PlussQit/record_function.h>

namespace pluss::detail {

void record_kernel_function_dtype(std::string name) {
  RECORD_FUNCTION_WITH_SCOPE(
        pluss::RecordScope::KERNEL_FUNCTION_DTYPE,
        std::move(name),
        MSharp::ArrayRef<const MSharp::IValue>{});
}

}  // namespace pluss::detail
#endif
