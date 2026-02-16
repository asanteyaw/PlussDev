#pragma once

#include "pluss/export.h"

#include <array>
#include <cstdint>

#include "Flame/Flame.h"
#include "CodeN/core/NoteImpl.h"

namespace pluss::coden {

namespace ops_internal { struct NoteAccess; }

class CODEN_API NoteBase {
public:
  NoteBase() = default;

  bool defined() const noexcept { return static_cast<bool>(impl_); }
  ::std::int64_t dim() const noexcept { return impl_ ? impl_->dim() : 0; }
  ::std::array<::std::int64_t,3> sizes3() const noexcept { return impl_ ? impl_->sizes3() : ::std::array<::std::int64_t,3>{0,0,0}; }
  ::std::int64_t size(::std::int64_t d) const noexcept {
    const auto s = sizes3();
    return (d>=0 && d<3) ? s[static_cast<::std::size_t>(d)] : 0;
  }
  ::std::int64_t numel() const noexcept { return impl_ ? impl_->numel() : 0; }

  // Encoding / storage intent.
  // - false: intended UTF-8 text (not validated)
  // - true : raw bytes (no UTF-8 guarantees)
  bool is_bytes() const noexcept { return impl_ ? impl_->is_bytes() : false; }

protected:
  friend struct ops_internal::NoteAccess;
  explicit NoteBase(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : impl_(::std::move(impl)) {}

  const ::pluss::flame::intrusive_ptr<NoteImpl>& impl() const noexcept { return impl_; }
  ::pluss::flame::intrusive_ptr<NoteImpl>& impl() noexcept { return impl_; }

private:
  ::pluss::flame::intrusive_ptr<NoteImpl> impl_;
};

} // namespace pluss::coden
