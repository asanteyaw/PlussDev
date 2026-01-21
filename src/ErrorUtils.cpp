#include <pluss/Series.h>

namespace pluss {

// This file provides the implementation for the error handling utilities
// defined in Series.h. Most of the implementation is already in the header
// as inline functions, but this file ensures that the symbols are properly
// exported from the library.

// Example of a non-inline function that could be added here:
void add_error_frame(PlussError& error, const SourceLocation& location) {
    error.add_frame(location);
}

} // namespace pluss
