# `pluss::ai::nn::cnn`

`cnn` owns the convolutional layers -- learned kernels that slide across
one, two, or three spatial axes, sharing the same weights across every
window position. Unlike [`Fusion`](fusion/Fusion.md), which learns a
separate weight for every input/output pair, a convolution's weight count
is independent of the input's spatial size, making it the standard choice
whenever nearby positions (in time, in an image, in a volume) share
structure worth exploiting.

## The Surface

| Entity | Shape | What it gives you |
| --- | --- | --- |
| [`Conv1d`](cnn/Conv1d.md) | `[N,C,L]` -> `[N,C',L']` | a sliding kernel over one axis (time, a 1-D signal) |
| [`Conv`](cnn/Conv.md) (also `Conv2d`, `ConvNd`) | `[N,C,H,W]` -> `[N,C',H',W']` | a sliding kernel over two axes (images) |
| [`Conv3d`](cnn/Conv3d.md) | `[N,C,D,H,W]` -> `[N,C',D',H',W']` | a sliding kernel over three axes (video, volumetric data) |

## How To Call It

Stack two one-dimensional convolutions with a ReLU between them, then run two signals of length 20. The program prints the output shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::Schedule net;
  net.append("conv1", nn::Conv1d(1, 8, 5));
  net.append("act1", nn::ReLU());
  net.append("conv2", nn::Conv1d(8, 16, 3));
  const pluss::Series x = pluss::make_series(std::vector<double>(2 * 1 * 20, 0.1), {2, 1, 20});
  const pluss::Series y = net.forward(x);
  std::cout << "output = " << y.sizes()[0] << " x " << y.sizes()[1] << " x " << y.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
output = 2 x 16 x 14
```

Every `cnn` layer composes into a [`Schedule`](module/Schedule.md) the
same way any other layer does. All three accept either a batched input
(with a leading sample axis) or a single unbatched sample, detected
automatically by rank -- see any one of them for the exact shapes.

## Related Pages

- [`module`](module.md) -- the `Module` contract every type here
  implements, and `Schedule`, the container they compose into.
- [`fusion`](fusion.md) -- the layer conventionally placed after a stack
  of convolutions has been flattened, to produce a final prediction.
- [`activation`](activation.md) -- the elementwise nonlinearities
  conventionally placed after each convolution.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

cnn/Introduction
cnn/Conv1d
cnn/Conv
cnn/Conv3d
```
<!-- apiref:toctree:end -->
