#include <MSharp/core/Allocator.h>
#include <MSharp/util/Exception.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace MSharp {

template <typename T>
class DeviceArray {
 public:
  DeviceArray(MSharp::Allocator& allocator, size_t size)
      : data_ptr_(allocator.allocate(size * sizeof(T))) {
    static_assert(std::is_trivial_v<T>, "T must be a trivial type");
    TORCH_INTERNAL_ASSERT(
        0 == (reinterpret_cast<intptr_t>(data_ptr_.get()) % alignof(T)),
        "MSharp::DeviceArray: Allocated memory is not aligned for this data type");
  }

  T* get() {
    return static_cast<T*>(data_ptr_.get());
  }

 private:
  MSharp::DataPtr data_ptr_;
};

} // namespace MSharp
