#pragma once

#include <cstdint>
#include <vector>

#include "MSharp/IntrusivePtr.h"

namespace pluss::coden {

class Note;
class TokenBatch;

// Implementation storage for TokenBatch.
//
// Important: Note is an incomplete type here; we intentionally store it as an
// owning pointer and define the destructor out-of-line (TokenBatch.cpp) where
// Note is complete. This avoids incomplete-type delete / destruction hazards.
struct TokenBatchImpl final : ::pluss::msharp::RefCounted {
  Note* tokens_ptr = nullptr; // owning
  ::std::vector<::std::int64_t> offsets;

  TokenBatchImpl(Note* tokens_ptr_, ::std::vector<::std::int64_t> offsets_)
      : tokens_ptr(tokens_ptr_), offsets(::std::move(offsets_)) {}

  ~TokenBatchImpl();

  TokenBatchImpl(const TokenBatchImpl&) = delete;
  TokenBatchImpl& operator=(const TokenBatchImpl&) = delete;
};

namespace ops_internal {
struct TokenBatchFactoryAccess {
  static TokenBatch make(Note tokens, ::std::vector<::std::int64_t> offsets);
};
} // namespace ops_internal

} // namespace pluss::coden
