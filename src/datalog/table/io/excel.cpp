#include "table/io/excel.h"

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_excel(const std::string& filename) {
    std::cout << filename << "\n";
    throw std::runtime_error("Excel loading not implemented yet");
}

}