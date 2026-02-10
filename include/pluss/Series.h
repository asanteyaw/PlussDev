#pragma once

#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <MSharp/util/Exception.h>

namespace pluss {

/**
 * @brief Struct to store source location information for error reporting
 */
struct SourceLocation {
    const char* file;
    int line;
    const char* function;
    
    SourceLocation(const char* file = __builtin_FILE(),
                   int line = __builtin_LINE(),
                   const char* function = __builtin_FUNCTION())
        : file(file), line(line), function(function) {}
};

/**
 * @brief Custom exception class for Pluss errors with Pythonic-style traceback
 */
class PlussError : public std::runtime_error {
private:
    std::string formatted_message;
    std::vector<SourceLocation> stack_trace;

public:
    PlussError(const std::string& message, const SourceLocation& location)
        : std::runtime_error(message) {
        stack_trace.push_back(location);
        format_message();
    }

    void add_frame(const SourceLocation& location) {
        stack_trace.push_back(location);
        format_message();
    }

    const char* what() const noexcept override {
        return formatted_message.c_str();
    }

private:
    void format_message() {
        std::ostringstream oss;
        oss << "\nPluss Error: " << std::runtime_error::what() << "\n";
        oss << "Traceback (most recent call last):\n";

        for (auto it = stack_trace.rbegin(); it != stack_trace.rend(); ++it) { // 🔁 reverse for Python-style
            oss << "  File \"" << it->file << "\", line " << it->line 
                << ", in " << it->function << "\n";
        }

        formatted_message = oss.str();
    }
};

inline std::string clean_torch_error(const MSharp::Error& e) {
    std::string msg = e.what_without_backtrace();
    const std::vector<std::string> prefixes = {
        "RuntimeError: ", "Error: ", "Exception: "
    };
    for (const auto& prefix : prefixes) {
        if (msg.substr(0, prefix.size()) == prefix) {
            msg = msg.substr(prefix.size());
            break;
        }
    }
    return msg;
}

// Enhanced to accumulate tracebacks
#define PLUSS_INTERNAL_TRY(code_block)                                                  \
  try {                                                                                 \
    code_block                                                                          \
  } catch (pluss::PlussError& e) {                                                      \
    e.add_frame(pluss::SourceLocation(__FILE__, __LINE__, __func__));                   \
    throw;                                                                              \
  } catch (const MSharp::Error& e) {                                                       \
    throw pluss::PlussError(pluss::clean_torch_error(e), pluss::SourceLocation(__FILE__, __LINE__, __func__)); \
  } catch (const std::exception& e) {                                                   \
    throw pluss::PlussError(e.what(), pluss::SourceLocation(__FILE__, __LINE__, __func__)); \
  }

#define PLUSS_INTERNAL_DISPATCH(code_block)                                             \
  try {                                                                                 \
    code_block                                                                          \
  } catch (pluss::PlussError& e) {                                                      \
    e.add_frame(pluss::SourceLocation(__FILE__, __LINE__, __func__));                   \
    throw;                                                                              \
  } catch (const MSharp::Error& e) {                                                       \
    throw pluss::PlussError(pluss::clean_torch_error(e), pluss::SourceLocation(__FILE__, __LINE__, __func__)); \
  } catch (const std::exception& e) {                                                   \
    throw pluss::PlussError(e.what(), pluss::SourceLocation(__FILE__, __LINE__, __func__)); \
  }

#define PLUSS_INTERNAL_CHECK(cond, msg)                                                 \
  if (!(cond)) {                                                                        \
    throw pluss::PlussError(msg, pluss::SourceLocation(__FILE__, __LINE__, __func__));  \
  }

class Series;  // unchanged

}