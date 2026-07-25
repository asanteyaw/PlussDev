#pragma once

#include "pluss/nn.h"

#include <string_view>
#include <vector>

namespace pluss::ai::ice::io {

// A generic, versioned, checksummed parameter-list artifact — the same
// "walk registered parameters in a fixed order, dump shape + values" idea
// generalized from NodeB's tasks/deep/token_classifier.cpp, but usable by
// any ice::Model rather than being written per call site.
//
// `parameters` must be in the same order, with the same shapes, on both
// save and load — i.e. the caller reconstructs an identical architecture
// before loading, then this restores the trained values into it.
void save_parameters(std::string_view filepath, const std::vector<Series*>& parameters);
void load_parameters(std::string_view filepath, const std::vector<Series*>& parameters);

} // namespace pluss::ai::ice::io
