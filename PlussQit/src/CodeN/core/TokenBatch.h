#pragma once

#include "pluss/export.h"

#include <cstdint>
#include <vector>

#include "MSharp/IntrusivePtr.h"

// TokenBatch holds an intrusive_ptr<TokenBatchImpl>, which requires that
// TokenBatchImpl derives from RefCounted (and be complete when the intrusive
// pointer's destructor is instantiated).
#include "CodeN/core/TokenBatchImpl.h"

namespace pluss::coden {

namespace ops_internal { struct TokenBatchFactoryAccess; }

class Note;

// A cheap, refcounted handle to a ragged token batch.
// Designed to be returned by tokenization-like ops without forcing Note's full
// definition into all includes.
class CODEN_API TokenBatch {
public:
  TokenBatch() = default;

  bool defined() const noexcept { return static_cast<bool>(impl_); }
  bool empty() const noexcept;

  // Accessors (references are valid as long as *this is alive).
  const Note& tokens() const;
  const ::std::vector<::std::int64_t>& offsets() const;

private:
  explicit TokenBatch(::pluss::msharp::intrusive_ptr<TokenBatchImpl> impl) : impl_(::std::move(impl)) {}

  ::pluss::msharp::intrusive_ptr<TokenBatchImpl> impl_;

  friend struct ops_internal::TokenBatchFactoryAccess;
  friend struct TokenBatchImpl;
};

} // namespace pluss::coden
