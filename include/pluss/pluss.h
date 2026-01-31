#pragma once

// Main header file for the pluss library
#include <pluss/Types.h>
#include <pluss/SeriesBody.h>
#include <pluss/series_factory.h>
#include <pluss/note_factory.h>
#include <pluss/SeriesFormatter.h>
#include <pluss/Chord.h>
#include <pluss/Note.h>
#include <pluss/NoteImpl.h>
#include <pluss/Timeline.h>
#include <pluss/tl_factory.h>
#include <pluss/abstractions.h>
#include <pluss/datalog/datalog.h>

// Provide `let` macro for users (enhanced auto). Include last to avoid
// macro leaking into internal implementation headers.
// Note: text helper types live under `lexicon::` and are intentionally
// not included by this umbrella header to avoid bringing internal
// implementation headers into user compilation units.
#include <internals/core/include/j2/let.h>
// always include public macros
#include <internals/core/include/b10/utils/Macros.h>

// Convenience namespace for users
namespace pluss {
    // All the necessary classes and functions are already in the pluss namespace
    // This header just brings them all together for easy inclusion
}
