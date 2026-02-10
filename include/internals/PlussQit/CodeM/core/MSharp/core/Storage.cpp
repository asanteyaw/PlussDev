#include <MSharp/core/RefcountedDeleter.h>
#include <MSharp/core/Storage.h>

namespace MSharp {

bool isSharedStorageAlias(const Storage& storage0, const Storage& storage1) {
  MSharp::DeleterFnPtr deleter_expected = &MSharp::refcounted_deleter;
  MSharp::DeleterFnPtr deleter0 = storage0.data_ptr().get_deleter();
  MSharp::DeleterFnPtr deleter1 = storage1.data_ptr().get_deleter();

  if ((deleter0 != deleter_expected) || (deleter1 != deleter_expected)) {
    return false;
  }

  return storage0.data_ptr().get_context() == storage1.data_ptr().get_context();
}

} // namespace MSharp
