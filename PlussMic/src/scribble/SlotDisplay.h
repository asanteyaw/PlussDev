#pragma once

#include <string>

#include "scribble/Slot.h"

// How a slot is described to a reader, shared by everything that shows one (a slot, a registry's fields, an axel's
// levels) so that they all say the same thing in the same words.
namespace pluss::scribble::detail {

// What a slot holds: the kind of its cells ("Series", "Note", "Timeline", "Duration"), "Mixed (Series, Note)" when
// they are of different kinds, "Null" when no cell has a value, "Empty" when there are no cells.
std::string slot_held_text(const Slot& slot);

// What one cell shows: its value, NULL when it has none, "[n values]" when it holds more than one.
std::string slot_cell_text(const Slot::Value& cell);

}  // namespace pluss::scribble::detail
