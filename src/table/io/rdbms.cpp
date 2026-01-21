#include "table/io/rdbms.h"

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_rdbms(const std::string& filename) {
    std::cout << filename << "\n";
    throw std::runtime_error("RDBMS loading not implemented yet");
}

}