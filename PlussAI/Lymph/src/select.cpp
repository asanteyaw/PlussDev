// select.cpp -- Comparison::run's dispatch.
//
// Expected to hold what's today selection.h's compare_batch_ (the
// candidate x fold parallel dispatch behind selection::compare) moved out
// of the header and into its own translation unit now that it's reached by
// a real class (Comparison) instead of a bare free function -- likely a
// close-to-verbatim move of existing, tested logic, not a rewrite.
//
// Nothing implemented yet -- placeholder until Comparison's actual API
// lands in select.h and this file's logic gets moved in from selection.h.
