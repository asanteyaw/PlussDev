#include "table/io/csv.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <algorithm>
#include <stdexcept>
#include <torch/torch.h>

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    std::vector<std::string> column_names;

    // ✅ Read header
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string col;
        while (std::getline(ss, col, ',')) {
            column_names.push_back(col);
        }
    }

    const size_t num_cols = column_names.size();
    std::unordered_map<std::string, std::vector<std::string>> raw_data;

    // ✅ Read all rows
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        size_t col_index = 0;
        while (std::getline(ss, cell, ',')) {
            if (col_index < num_cols) {
                raw_data[column_names[col_index++]].push_back(cell);
            }
        }
    }
    file.close();

    auto df = std::make_shared<DataFrame>();

    for (const auto& [col, values] : raw_data) {
        bool is_int = std::ranges::all_of(values, [](const std::string& val) {
            return val.find('.') == std::string::npos && val.find('e') == std::string::npos;
        });

        if (is_int) {
            std::vector<int32_t> int_vals;
            for (const auto& val : values) {
                try {
                    int_vals.push_back(std::stoi(val));
                } catch (...) {
                    int_vals.push_back(0);
                }
            }
            df = df->set_col(col, torch::tensor(int_vals, torch::kInt32));
        } else {
            std::vector<float> float_vals;
            for (const auto& val : values) {
                try {
                    float_vals.push_back(std::stof(val));
                } catch (...) {
                    float_vals.push_back(NAN);
                }
            }
            df = df->set_col(col, torch::tensor(float_vals, torch::kFloat32));
        }
    }

    return df;
}

}  // namespace pluss::table::io