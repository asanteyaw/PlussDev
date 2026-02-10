#pragma once

#include <pluss/Series.h>
#include <tuple>
#include <vector>

namespace pluss {
namespace rnn {

/**
 * @brief Macro for dispatching functions that return a std::tuple of Series
 */
#define PLUSS_INTERNAL_DISPATCH_TUPLE(code_block)                                        \
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

/**
 * @brief Macro for dispatching functions that return a std::vector of Series
 */
#define PLUSS_INTERNAL_DISPATCH_VECTOR(code_block)                                       \
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

} // namespace rnn
} // namespace pluss
