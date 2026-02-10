#pragma once

#include <MSharp/core/ScalarType.h>
#include <MSharp/macros/Export.h>

namespace caffe2 {
class TypeMeta;
} // namespace caffe2

namespace MSharp {
C10_API void set_default_dtype(caffe2::TypeMeta dtype);
C10_API const caffe2::TypeMeta get_default_dtype();
C10_API ScalarType get_default_dtype_as_scalartype();
C10_API const caffe2::TypeMeta get_default_complex_dtype();
} // namespace MSharp
