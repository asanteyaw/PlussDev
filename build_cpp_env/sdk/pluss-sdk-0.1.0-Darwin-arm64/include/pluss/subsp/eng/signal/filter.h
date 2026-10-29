#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>

namespace pluss::subsp::eng::signal {

enum class ConvMode {
  Full,
  Same,
  Valid,
};

enum class PadMode {
  Odd,
  Even,
  Constant,
  None,
};

struct FilterRun {
  Series values;
  Series state;
};

struct PhaseOpts {
  std::int64_t dim{-1};
  std::int64_t edge{-1};
  PadMode pad{PadMode::Odd};
};

Series conv(const Series &input, const Series &kernel, std::int64_t dim = -1,
            ConvMode mode = ConvMode::Full);
Series corr(const Series &input, const Series &kernel, std::int64_t dim = -1,
            ConvMode mode = ConvMode::Full);
Series fir(const Series &input, const Series &taps, std::int64_t dim = -1);

FilterRun filter(const Series &input, const Series &numerator,
                 const Series &denominator, std::int64_t dim = -1);
FilterRun filter(const Series &input, const Series &numerator,
                 const Series &denominator, const Series &state,
                 std::int64_t dim = -1);

FilterRun sos(const Series &input, const Series &sections,
              std::int64_t dim = -1);
FilterRun sos(const Series &input, const Series &sections, const Series &state,
              std::int64_t dim = -1);

Series zero_phase(const Series &input, const Series &numerator,
                  const Series &denominator, const PhaseOpts &options = {});
Series zero_phase(const Series &input, const Series &sections,
                  const PhaseOpts &options = {});

Series freqz(const Series &numerator, const Series &omega,
             std::int64_t dim = -1);
Series freqz(const Series &numerator, const Series &denominator,
             const Series &omega, std::int64_t dim = -1);

} // namespace pluss::subsp::eng::signal
