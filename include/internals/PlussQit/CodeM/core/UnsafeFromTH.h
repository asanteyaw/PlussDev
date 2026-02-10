#pragma once
#include <PlussQit/core/Series.h>

namespace pluss {

inline Series unsafeTensorFromTH(void * th_pointer, bool retain) {
  auto tensor_impl = MSharp::intrusive_ptr<TensorImpl, UndefinedTensorImpl>::reclaim(static_cast<TensorImpl*>(th_pointer));
  if (retain && tensor_impl.get() != UndefinedTensorImpl::singleton()) {
    MSharp::raw::intrusive_ptr::incref(tensor_impl.get());
  }
  return Series(std::move(tensor_impl));
}

inline Storage unsafeStorageFromTH(void * th_pointer, bool retain) {
  if (retain && th_pointer) {
    MSharp::raw::intrusive_ptr::incref(static_cast<StorageImpl*>(th_pointer));
  }
  return Storage(MSharp::intrusive_ptr<StorageImpl>::reclaim(static_cast<StorageImpl*>(th_pointer)));
}

}
