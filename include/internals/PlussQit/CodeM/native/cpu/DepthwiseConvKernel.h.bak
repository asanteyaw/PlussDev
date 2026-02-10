#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <MSharp/util/ArrayRef.h>

/*
  Depthwise 3x3 Winograd convolution operator
*/

namespace pluss {
class Series;

namespace native {

using convolution_depthwise3x3_winograd_fn =
    Series (*)(const Series &, const Series &, const Series &, IntArrayRef, IntArrayRef, int64_t);

DECLARE_DISPATCH(convolution_depthwise3x3_winograd_fn, convolution_depthwise3x3_winograd_stub)

}  // namespace native
}  // namespace pluss
