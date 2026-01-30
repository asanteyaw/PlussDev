#pragma once

// Main header file for the pluss library
#include <internals/aux/utils/Macros.h>
#include <internals/aux/let/text.h>
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

// Define `let` for users after including all internal headers that use
// the identifier `let` as a namespace or symbol. Placing this include
// last prevents the `let` macro from colliding with earlier headers.
#include <internals/core/include/j2/let.h>

// Convenience namespace for users
namespace pluss {
    // All the necessary classes and functions are already in the pluss namespace
    // This header just brings them all together for easy inclusion
}
