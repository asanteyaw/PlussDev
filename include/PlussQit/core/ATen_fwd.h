#pragma once
#include <MSharp/core/QScheme.h>

// Forward declarations of core ATen types used in dispatch functions
namespace MSharp {

template<typename T>
class List;
template<typename T>
class IListRef;
class Stream;
class Scalar;
class SymInt;
class SymIntList;
struct Storage;
struct TensorOptions;
template <typename T>
class ArrayRef;
template <typename T>
class OptionalArrayRef;

}  // namespace MSharp

namespace at {

class Tensor;
class OptionalTensorRef;
struct Dimname;
struct Generator;
using TensorList = MSharp::ArrayRef<Tensor>;
using ITensorListRef = MSharp::IListRef<Tensor>;
using IOptTensorListRef = MSharp::IListRef<OptionalTensorRef>;
using DimnameList = MSharp::ArrayRef<Dimname>;
using IntArrayRef = MSharp::ArrayRef<int64_t>;
using OptionalIntArrayRef = MSharp::OptionalArrayRef<int64_t>;
using OptionalSymIntArrayRef = MSharp::OptionalArrayRef<MSharp::SymInt>;

using MSharp::Stream;
using MSharp::Storage;
using MSharp::QScheme;
using MSharp::Scalar;
using MSharp::SymInt;
using MSharp::SymIntList;
using MSharp::TensorOptions;

}  // namespace at
