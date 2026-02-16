#pragma once

#include "pluss/export.h"

#include <cstddef>
#include <string_view>

#include "Flame/Flame.h"
#include "CodeN/core/ChordImpl.h"

namespace pluss::coden {

namespace ops_internal { struct ChordAccess; }

// Lightweight handle (like TensorBase): cheap copy, refcounted, non-owning view.
class CODEN_API ChordBase {
public:
  ChordBase() = default;

  bool defined() const noexcept { return static_cast<bool>(impl_); }
  ::std::size_t size_bytes() const noexcept { return impl_ ? impl_->size_bytes() : 0; }
  ::std::string_view utf8() const { return impl_ ? impl_->utf8() : ::std::string_view{}; }

protected:
  friend struct ops_internal::ChordAccess;
  explicit ChordBase(::pluss::flame::intrusive_ptr<ChordImpl> impl)
      : impl_(::std::move(impl)) {}

  const ::pluss::flame::intrusive_ptr<ChordImpl>& impl() const noexcept { return impl_; }
  ::pluss::flame::intrusive_ptr<ChordImpl>& impl() noexcept { return impl_; }

private:
  ::pluss::flame::intrusive_ptr<ChordImpl> impl_;
};

} // namespace pluss::coden
