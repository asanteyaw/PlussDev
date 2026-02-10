#pragma once

#include <string>

// Forward declarations only.
// This header is safe to include from core type headers (Series/Note/Timeline)
// to declare friend construction functions without introducing heavy includes
// or circular dependencies.

namespace at {
class TensorBase;
} // namespace at

namespace pluss {

class Series;
class Note;
class Timeline;

namespace detail {
struct NoteBook;
} // namespace detail

template <typename T>
class intrusive_ptr;

} // namespace pluss

namespace b10::detail {

// Privileged construction entry points (friend-gated by the core types).
::pluss::Series series_gen(const at::TensorBase& base);
::pluss::Series series_gen(at::TensorBase&& base);

::pluss::Note note_gen(::pluss::intrusive_ptr<::pluss::detail::NoteBook> handle);

struct TimelineSpec;

// Single friend-gated Timeline construction entry point.
::pluss::Timeline tl_gen(const TimelineSpec& spec);

} // namespace b10::detail
