#ifndef C10_UTIL_BACKTRACE_H_
#define C10_UTIL_BACKTRACE_H_

#include <cstddef>
#include <memory>
#include <string>
#include <typeinfo>

#include <MSharp/macros/Macros.h>
#include <MSharp/util/Lazy.h>

namespace MSharp {

// Symbolizing the backtrace can be expensive; pass it around as a lazy string
// so it is symbolized only if actually needed.
using Backtrace = std::shared_ptr<const LazyValue<std::string>>;

// DEPRECATED: Prefer get_lazy_backtrace().
C10_API std::string get_backtrace(
    size_t frames_to_skip = 0,
    size_t maximum_number_of_frames = 64,
    bool skip_python_frames = true);

C10_API Backtrace get_lazy_backtrace(
    size_t frames_to_skip = 0,
    size_t maximum_number_of_frames = 64,
    bool skip_python_frames = true);

} // namespace MSharp

#endif // C10_UTIL_BACKTRACE_H_
