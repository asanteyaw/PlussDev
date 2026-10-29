#pragma once

#include <cstdint>
#include <string_view>

namespace pluss::codem {
class Series;
}

namespace pluss::codem::_compat::fourier {

Series fft(const Series &input, std::int64_t n, std::int64_t dim,
           std::string_view norm);
Series ifft(const Series &input, std::int64_t n, std::int64_t dim,
            std::string_view norm);
Series rfft(const Series &input, std::int64_t n, std::int64_t dim,
            std::string_view norm);
Series irfft(const Series &input, std::int64_t n, std::int64_t dim,
             std::string_view norm);

} // namespace pluss::codem::_compat::fourier
