#pragma once

#include <internals/core/include/b10/deets/operations_fwd.h>

#include <pluss/Note.h>
#include <PlussQit/core/SeriesBody.h>

#include <string>
#include <utility>

// This header defines the *privileged construction* entry points for core types.
//
// Design intent:
// - End users should not construct core types directly via public constructors.
// - Public, user-facing factory APIs live in pluss/*_factory.h.
// - Factories (and other internal code) include this header and call b10::detail::*_gen.
// - The core types friend these functions via operations_fwd.h.

namespace b10::detail {

inline ::pluss::Series series_gen(const at::TensorBase& base) {
	return ::pluss::Series(base);
}

inline ::pluss::Series series_gen(at::TensorBase&& base) {
	return ::pluss::Series(std::move(base));
}

inline ::pluss::Note note_gen(::pluss::intrusive_ptr<::pluss::detail::NoteBook> handle) {
	return ::pluss::Note(std::move(handle));
}

} // namespace b10::detail
