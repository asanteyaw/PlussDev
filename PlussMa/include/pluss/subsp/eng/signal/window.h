#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>

namespace pluss::subsp::eng::signal {

enum class WindowMode {
  Symmetric,
  Periodic,
};

struct WindowOptions {
  WindowMode mode{WindowMode::Symmetric};
  ScalarType dtype{ScalarType::Float64};
  Platform platform{Platform::CPU};
};

Series boxcar(std::int64_t size, const WindowOptions &options = {});
Series bartlett(std::int64_t size, const WindowOptions &options = {});
Series blackman(std::int64_t size, const WindowOptions &options = {});
Series hamming(std::int64_t size, const WindowOptions &options = {});
Series hann(std::int64_t size, const WindowOptions &options = {});

} // namespace pluss::subsp::eng::signal
