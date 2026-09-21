#include "CodeN/core/TokenBatch.h"

#include <algorithm>

#include "CodeN/core/Note.h"
#include "CodeN/core/OpsInternal.h"
#include "CodeN/core/TokenBatchImpl.h"

namespace pluss::coden {

TokenBatchImpl::~TokenBatchImpl() {
  delete tokens_ptr;
  tokens_ptr = nullptr;
  delete offsets_ptr;
  offsets_ptr = nullptr;
}

bool TokenBatch::empty() const noexcept {
  return !defined() || !impl_->tokens_ptr->defined() || impl_->tokens_ptr->empty();
}

const Note& TokenBatch::tokens() const {
  PLUSS_CHECK(defined(), "TokenBatch is undefined");
  return *impl_->tokens_ptr;
}

const Note& TokenBatch::offsets() const {
  PLUSS_CHECK(defined(), "TokenBatch is undefined");
  return *impl_->offsets_ptr;
}

} // namespace pluss::coden

namespace pluss::coden::ops_internal {

TokenBatch TokenBatchFactoryAccess::make(Note tokens, Note offsets) {
  PLUSS_CHECK(offsets.defined() && offsets.is_int64(),
              "TokenBatch offsets must be a defined Int64 Note");
  PLUSS_CHECK(offsets.dim() == 1 && offsets.numel() >= 1,
              "TokenBatch offsets must be a non-empty rank-1 Note");
  const auto lanes = offsets.integers();
  PLUSS_CHECK(lanes.front() == 0, "TokenBatch offsets must start at zero");
  PLUSS_CHECK(::std::is_sorted(lanes.begin(), lanes.end()),
              "TokenBatch offsets must be nondecreasing");
  if (tokens.defined()) {
    PLUSS_CHECK(tokens.is_text() || tokens.is_bytes(),
                "TokenBatch tokens must use Text or Bytes storage");
    PLUSS_CHECK(lanes.back() == tokens.numel(),
                "TokenBatch final offset must equal token count");
  } else {
    PLUSS_CHECK(lanes.size() == 1 && lanes.front() == 0,
                "Undefined TokenBatch tokens require the canonical {0} offsets");
  }

  // Store both halves of the ragged result as owning Note handles in
  // separate allocations (Note is incomplete in TokenBatchImpl.h).
  auto* tokens_ptr = new Note(::std::move(tokens));
  auto* offsets_ptr = new Note(::std::move(offsets));
  auto impl = ::pluss::msharp::make_intrusive<TokenBatchImpl>(tokens_ptr, offsets_ptr);
  return TokenBatch(::std::move(impl));
}

} // namespace pluss::coden::ops_internal
