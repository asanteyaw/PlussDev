#pragma once

#include "pluss/ai/nodej/status.h"

namespace pluss::ai::nodeb {

using StatusCode = nodej::StatusCode;
using Status = nodej::Status;

template <class T>
using StatusOr = nodej::StatusOr<T>;

} // namespace pluss::ai::nodeb
