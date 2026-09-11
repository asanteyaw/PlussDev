#include "CodeN/core/TokenBatch.h"

#include <array>

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

TokenBatch TokenBatchFactoryAccess::make(Note tokens, ::std::vector<::std::int64_t> offsets) {
  // Store both halves of the ragged result as owning Note handles in
  // separate allocations (Note is incomplete in TokenBatchImpl.h).
  auto* tokens_ptr = new Note(::std::move(tokens));
  const ::std::array<::std::int64_t, 1> shape{static_cast<::std::int64_t>(offsets.size())};
  auto* offsets_ptr = new Note(note_from_int64(offsets, shape));
  auto impl = ::pluss::msharp::make_intrusive<TokenBatchImpl>(tokens_ptr, offsets_ptr);
  return TokenBatch(::std::move(impl));
}

} // namespace pluss::coden::ops_internal
