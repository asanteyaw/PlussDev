#include <pluss/pluss.h>
#include <iostream>
#include <stdexcept>

namespace pluss::table::io {

std::shared_ptr<DataFrame> load_excel(const std::string& filename) {
    std::cout << filename << "\n";
    throw std::runtime_error("Excel loading not implemented yet");
}

}