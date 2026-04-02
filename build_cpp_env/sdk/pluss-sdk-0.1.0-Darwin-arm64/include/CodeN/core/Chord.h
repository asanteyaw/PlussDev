#pragma once

#include "pluss/export.h"

#include <string>
#include <string_view>

#include "CodeN/core/Errors.h"
#include "CodeN/core/ChordBase.h"

namespace pluss::coden {

namespace ops_internal {
struct ChordFactoryAccess;
} // namespace ops_internal

class TokenBatch;

// Internal scalar text handle.
class CODEN_API Chord final : public ChordBase {
public:
  static Chord undefined() noexcept { return Chord(); }

  bool empty() const noexcept { return size_bytes() == 0; }

  // Full surface (hundreds of methods)
  #include "CodeN/generated/ChordMethodDeclsGen.h"

  // Materialize to owned UTF-8 string.
  ::std::string to_utf8() const { return ::std::string(utf8()); }

private:
  Chord() = default;

  explicit Chord(::pluss::flame::intrusive_ptr<ChordImpl> impl)
      : ChordBase(::std::move(impl)) {}

  friend struct ops_internal::ChordFactoryAccess;
};


} // namespace pluss::coden

// Inline method definitions are included outside the namespace to avoid
// accidental nested-namespace pollution via transitive includes.
#include "CodeN/generated/ChordMethodDefsGen.h"
