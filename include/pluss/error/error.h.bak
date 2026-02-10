#pragma once
#include <stdexcept>
#include <string>

namespace pluss {

class PlussError : public std::runtime_error {
public:
    explicit PlussError(const std::string& message)
        : std::runtime_error(message) {}
};

}  // namespace pluss