#pragma once

#include <pluss/core.h>
#include "MSharp/RangeView.h"

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "NodeK/models/rnn/utils_config.h"

namespace pluss::ai::kernels::cudnn {

bool rnn_route_enabled() noexcept;
bool rnn_route_strict() noexcept;
bool rnn_route_ready() noexcept;

bool supports_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason = nullptr) noexcept;

bool supports_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason = nullptr) noexcept;

bool supports_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason = nullptr) noexcept;

std::optional<std::tuple<Series, Series>> try_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward);

std::optional<std::tuple<Series, Series>> try_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward);

std::optional<std::tuple<Series, std::tuple<Series, Series>>> try_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward);

} // namespace pluss::ai::kernels::cudnn
