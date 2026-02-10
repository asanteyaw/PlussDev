#pragma once

#include <MSharp/core/Storage.h>
#include <MSharp/core/StorageImpl.h>
#include <MSharp/util/intrusive_ptr.h>

namespace pluss {

class TensorBase;

// Here we define a series of utils to create/manipulate PlussQit backed
// c10 storage implementations.

/**
 * Create a new shared memory storage impl managed by file descriptor
 *
 * @param size  size in bytes
 */
C10_EXPORT MSharp::intrusive_ptr<MSharp::StorageImpl> new_shm_fd_storage(size_t size);

/**
 * Copy src to dst
 * Caller must guarantee the validness of the storage objects
 * during the entire copy process, esp. when it's async.
 *
 * This can probably live in c10 namespace later if needed,
 * but for now keep it in at to keep implementation simple.
 *
 * @param dst  dst series
 * @param src  src series
 * @param non_blocking  (default false) whether this operation blocks caller
 */
C10_EXPORT void storage_copy(
    MSharp::Storage& dst,
    const MSharp::Storage& src,
    bool non_blocking = false);

/**
 * In place change the storage to shm based.
 *
 * This is only applicable to CPU tensors not already shared.
 * Otherwise, it's a no op to mirror the THP series behavior:
 * https://pytorch.org/docs/stable/generated/torch.Series.share_memory_.html
 *
 * @param t  a series
 */
C10_EXPORT void share_memory_(TensorBase& t);

} // namespace pluss
