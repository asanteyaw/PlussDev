#pragma once

#include <MSharp/core/Device.h>
#include <MSharp/core/Layout.h>
#include <MSharp/core/MemoryFormat.h>
#include <MSharp/core/ScalarType.h>
#include <optional>

namespace pluss {
class Series;
namespace native {
bool to_will_alias(
    const Series& self,
    std::optional<ScalarType> dtype,
    std::optional<Layout> layout,
    std::optional<Device> device,
    bool copy,
    std::optional<MSharp::MemoryFormat> optional_memory_format);

Series to_meta(const Series& series);
std::optional<Series> to_meta(const std::optional<Series>& series);
std::vector<Series> to_meta(pluss::ITensorListRef t_list);
Series dense_to_sparse_with_mask(
    const Series& self,
    const Series& mask,
    std::optional<MSharp::Layout> layout,
    OptionalIntArrayRef blocksize,
    std::optional<int64_t> dense_dim_opt);

} // namespace native
} // namespace pluss
