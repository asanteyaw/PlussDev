#include <PlussQit/core/Series.h>

namespace pluss::native {

std::tuple<Series, Series, Series> slow_conv3d_backward_cpu(
    const Series& grad_output,
    const Series& self,
    const Series& weight,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    std::array<bool, 3> output_mask);

} // namespace pluss::native
