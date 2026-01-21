#include "table/io/parquet.h"

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_parquet(const std::string& filename) {
    std::cout << filename << "\n";
    throw std::runtime_error("Parquet loading not implemented yet");
}

}