#pragma once

#include <MSharp/util/Exception.h>
#include <MSharp/util/ParallelGuard.h>
#include <MSharp/util/SmallVector.h>

namespace pluss {

template <class F>
inline void parallel_for(
    const int64_t begin,
    const int64_t end,
    const int64_t grain_size,
    const F& f) {
  TORCH_INTERNAL_ASSERT_DEBUG_ONLY(grain_size >= 0);
  if (begin >= end) {
    return;
  }

#ifdef INTRA_OP_PARALLEL
  pluss::internal::lazy_init_num_threads();
  const auto numiter = end - begin;
  const bool use_parallel =
      (numiter > grain_size && numiter > 1 && !pluss::in_parallel_region() &&
       pluss::get_num_threads() > 1);
  if (!use_parallel) {
    internal::ThreadIdGuard tid_guard(0);
    MSharp::ParallelGuard guard(true);
    f(begin, end);
    return;
  }

  internal::invoke_parallel(
      begin, end, grain_size, [&](int64_t begin, int64_t end) {
        MSharp::ParallelGuard guard(true);
        f(begin, end);
      });
#else
  internal::ThreadIdGuard tid_guard(0);
  MSharp::ParallelGuard guard(true);
  f(begin, end);
#endif
}

template <class scalar_t, class F, class SF>
inline scalar_t parallel_reduce(
    const int64_t begin,
    const int64_t end,
    const int64_t grain_size,
    const scalar_t ident,
    const F& f,
    const SF& sf) {
  TORCH_CHECK(grain_size >= 0);
  if (begin >= end) {
    return ident;
  }

#ifdef INTRA_OP_PARALLEL
  pluss::internal::lazy_init_num_threads();
  const auto max_threads = pluss::get_num_threads();
  const bool use_parallel =
      ((end - begin) > grain_size && !pluss::in_parallel_region() &&
       max_threads > 1);
  if (!use_parallel) {
    internal::ThreadIdGuard tid_guard(0);
    MSharp::ParallelGuard guard(true);
    return f(begin, end, ident);
  }

  MSharp::SmallVector<scalar_t, 64> results(max_threads, ident);
  internal::invoke_parallel(
      begin,
      end,
      grain_size,
      [&](const int64_t my_begin, const int64_t my_end) {
        const auto tid = pluss::get_thread_num();
        MSharp::ParallelGuard guard(true);
        results[tid] = f(my_begin, my_end, ident);
      });

  scalar_t result = ident;
  for (auto partial_result : results) {
    result = sf(result, partial_result);
  }
  return result;
#else
  internal::ThreadIdGuard tid_guard(0);
  MSharp::ParallelGuard guard(true);
  return f(begin, end, ident);
#endif
}

} // namespace pluss
