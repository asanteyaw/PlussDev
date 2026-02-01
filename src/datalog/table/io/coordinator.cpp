#include <pluss/pluss.h>
#include <torch/torch.h>
#include <cmath>
#include <ranges>
#include <string>
#include <vector>
#include <charconv>
#include <cerrno>
#include <cstdlib>

namespace pluss::table::io {

std::shared_ptr<pluss::table::DataFrame> convert_raw_to_dataframe(const RawTable& raw) {
    auto df = std::make_shared<pluss::table::DataFrame>();

    for (const auto& col : raw.columns) {
        const auto& values = raw.data.at(col);

        // Heuristic: consider column integer if every non-empty cell has no '.' or 'e'/'E'
        bool is_int = std::ranges::all_of(values, [](const std::string& val) {
            if (val.empty()) return true; // treat empty as compatible with int (will become 0)
            return val.find('.') == std::string::npos && val.find('e') == std::string::npos && val.find('E') == std::string::npos;
        });

        if (is_int) {
            std::vector<int32_t> int_vals;
            int_vals.reserve(values.size());
            for (const auto& v : values) {
                if (v.empty()) {
                    int_vals.push_back(0);
                    continue;
                }
                // fast parse integer using from_chars
                long long tmp = 0;
                auto res = std::from_chars(v.data(), v.data() + v.size(), tmp);
                if (res.ec == std::errc() ) {
                    int_vals.push_back(static_cast<int32_t>(tmp));
                } else {
                    // fallback to stoll (locale-dependent but robust)
                    try {
                        tmp = std::stoll(v);
                        int_vals.push_back(static_cast<int32_t>(tmp));
                    } catch (...) {
                        int_vals.push_back(0);
                    }
                }
            }
            // Create a SeriesDataContainer to leverage pluss' factory path (avoids direct torch::tensor call)
            SeriesDataContainer container_int(int_vals);
            at::Tensor t_int = container_int.convert_to_tensor(at::TensorOptions().dtype(at::kInt));
            pluss::Series s_int(t_int);
            df = df->set_col(col, pluss::table::Thread(std::move(s_int), col));
        } else {
            std::vector<float> float_vals;
            float_vals.reserve(values.size());
            for (const auto& v : values) {
                if (v.empty()) {
                    float_vals.push_back(NAN);
                    continue;
                }
                // fast C parsing for float
                char* endptr = nullptr;
                errno = 0;
                float f = std::strtof(v.c_str(), &endptr);
                if (endptr != v.c_str() && *endptr == '\0' && errno == 0) {
                    float_vals.push_back(f);
                } else {
                    try {
                        float_vals.push_back(std::stof(v));
                    } catch (...) {
                        float_vals.push_back(NAN);
                    }
                }
            }
            // Create a SeriesDataContainer to leverage pluss' factory path (avoids direct torch::tensor call)
            SeriesDataContainer container_flt(float_vals);
            at::Tensor t_flt = container_flt.convert_to_tensor(at::TensorOptions().dtype(at::kFloat));
            pluss::Series s_flt(t_flt);
            df = df->set_col(col, pluss::table::Thread(std::move(s_flt), col));
        }
    }

    return df;
}

} // namespace pluss::table::io
