#pragma once

#include <MSharp/macros/Macros.h>
#include <MSharp/util/intrusive_ptr.h>

namespace MSharp {
struct StorageImpl;
class DataPtr;
} // namespace MSharp

namespace MSharp::impl::cow {

// Creates a Copy-on-write (COW) clone of the given storage. This will also
// convert the given storage into a COW storage if it is not COW already.
//
// Converting the storage into a COW storage will not be successful if the
// storage's DataPtr has some context (`DataPtr::get_context()`) which is not
// equal to the data pointer (`DataPtr::get()`). In this case, a nullptr is
// returned.
C10_API MSharp::intrusive_ptr<StorageImpl> lazy_clone_storage(
    StorageImpl& storage);

// Check if a storage has a simple DataPtr with no abnormal context
C10_API bool has_simple_data_ptr(const MSharp::StorageImpl& storage);

// Check if a DataPtr is COW
C10_API bool is_cow_data_ptr(const MSharp::DataPtr& data_ptr);

// Eagerly copies a COW storage's data, turning it into a non-COW storage.
C10_API void materialize_cow_storage(StorageImpl& storage);

} // namespace MSharp::impl::cow
