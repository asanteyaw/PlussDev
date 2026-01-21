#pragma once

#include "error.h"
#include <exception>
#include <c10/util/Exception.h>

namespace pluss {

// Trim internal file paths and stack traces from Torch errors
inline std::string sanitize_torch_message(const std::string& msg) {
    auto pos = msg.find("Exception raised from");
    return (pos != std::string::npos) ? msg.substr(0, pos) : msg;
}

template <typename Func>
auto attempt(Func&& func) -> decltype(func()) {
    try {
        return func();
    } catch (const c10::Error& e) {
        throw PlussError(sanitize_torch_message(e.what()));
    } catch (const std::exception& e) {
        throw PlussError(std::string("std::exception: ") + e.what());
    }
}

}  // namespace pluss