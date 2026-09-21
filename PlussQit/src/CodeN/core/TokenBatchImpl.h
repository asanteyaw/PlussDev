#pragma once

#include "MSharp/IntrusivePtr.h"

namespace pluss::coden {

class Note;
class TokenBatch;

// Implementation storage for TokenBatch.
//
// Important: Note is an incomplete type here; we intentionally store both
// tokens and offsets as owning pointers and define the destructor out-of-line
// (TokenBatch.cpp) where Note is complete. This avoids incomplete-type
// delete / destruction hazards. A ragged result is a closed pair of Notes
// (pass 6 of the Note redesign): `tokens_ptr` (Text/Bytes) and `offsets_ptr`
// (Int64, length numel()+1) -- no raw std::vector<int64_t> leaks out.
struct TokenBatchImpl final : ::pluss::msharp::RefCounted {
  Note* tokens_ptr = nullptr;  // owning
  Note* offsets_ptr = nullptr; // owning; Int64 Note

  TokenBatchImpl(Note* tokens_ptr_, Note* offsets_ptr_)
      : tokens_ptr(tokens_ptr_), offsets_ptr(offsets_ptr_) {}

  ~TokenBatchImpl();

  TokenBatchImpl(const TokenBatchImpl&) = delete;
  TokenBatchImpl& operator=(const TokenBatchImpl&) = delete;
};

namespace ops_internal {
struct TokenBatchFactoryAccess {
  static TokenBatch make(Note tokens, Note offsets);
};
} // namespace ops_internal

} // namespace pluss::coden
