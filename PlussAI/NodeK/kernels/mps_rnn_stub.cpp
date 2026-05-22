#include "NodeK/kernels/mps_rnn.h"

#if !defined(__APPLE__)

namespace pluss::ai::kernels::mps {

bool lstm_route_enabled() noexcept { return false; }
bool lstm_route_strict() noexcept { return false; }
bool lstm_route_ready() noexcept { return false; }

bool supports_lstm(
    const Series&,
    const Series&,
    const Series&,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight>,
    const ::pluss::ai::rnn::NetConfig&,
    bool,
    std::string* reason) noexcept {
  if (reason) *reason = "MPS LSTM route is only available on Apple platforms";
  return false;
}

std::optional<std::tuple<Series, std::tuple<Series, Series>>> try_lstm(
    const Series&,
    const Series&,
    const Series&,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight>,
    const ::pluss::ai::rnn::NetConfig&,
    bool) {
  return std::nullopt;
}

} // namespace pluss::ai::kernels::mps

#endif
