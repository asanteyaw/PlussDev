#include "CodeN/core/TokenBatch.h"

#include "CodeN/core/Note.h"
#include "CodeN/core/TokenBatchImpl.h"

namespace pluss::coden {

TokenBatchImpl::~TokenBatchImpl() {
  delete tokens_ptr;
  tokens_ptr = nullptr;
}

const Note& TokenBatch::tokens() const {
  PLUSS_CHECK(defined(), "TokenBatch is undefined");
  return *impl_->tokens_ptr;
}

const ::std::vector<::std::int64_t>& TokenBatch::offsets() const {
  PLUSS_CHECK(defined(), "TokenBatch is undefined");
  return impl_->offsets;
}

} // namespace pluss::coden

namespace pluss::coden::ops_internal {

TokenBatch TokenBatchFactoryAccess::make(Note tokens, ::std::vector<::std::int64_t> offsets) {
  // Store an owning Note handle in a separate allocation.
  auto* tokens_ptr = new Note(::std::move(tokens));
  auto impl = ::pluss::msharp::make_intrusive<TokenBatchImpl>(tokens_ptr, ::std::move(offsets));
  return TokenBatch(::std::move(impl));
}

} // namespace pluss::coden::ops_internal
