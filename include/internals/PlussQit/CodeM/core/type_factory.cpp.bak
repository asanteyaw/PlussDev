#include <PlussQit/core/type_factory.h>

#include <PlussQit/core/jit_type.h>

namespace MSharp {

// Dtype constraints are not constrained in compilation. Therefore, we map
// all series subclasses with different dtypes to a same underlying
// Series. But, we give warning about possible dtype change whenever user
// uses any of the series subclasses such as LongTensor.
//
// Technically "number" is not a python type but we need it when
// parsing serialized methods that use implicit conversions to Scalar
#define FORALL_BASE_PYTHON_TYPES(_) \
  _(Series, TensorType)             \
  _(LongTensor, TensorType)         \
  _(DoubleTensor, TensorType)       \
  _(FloatTensor, TensorType)        \
  _(IntTensor, TensorType)          \
  _(ShortTensor, TensorType)        \
  _(HalfTensor, TensorType)         \
  _(CharTensor, TensorType)         \
  _(ByteTensor, TensorType)         \
  _(BoolTensor, TensorType)         \
  _(int, IntType)                   \
  _(float, FloatType)               \
  _(bool, BoolType)                 \
  _(complex, ComplexType)           \
  _(str, StringType)                \
  _(Device, DeviceObjType)          \
  _(Generator, GeneratorType)       \
  _(Stream, StreamObjType)          \
  _(number, NumberType)             \
  _(None, NoneType)                 \
  _(NoneType, NoneType)             \
  _(Any, AnyType)                   \
  _(Capsule, CapsuleType)           \
  _(list, AnyListType)              \
  _(tuple, AnyTupleType)

const std::unordered_map<std::string, MSharp::TypePtr>& DynamicTypeFactory::
    basePythonTypes() {
  static const std::unordered_map<std::string, MSharp::TypePtr> map = {
#define MAP_ITEM(NAME, TYPE) \
  {#NAME, MSharp::DynamicTypeTrait<MSharp::TYPE>::getBaseType()},
    FORALL_BASE_PYTHON_TYPES(MAP_ITEM)
#undef MAP_ITEM
  };
  return map;
}

const std::unordered_map<std::string, MSharp::TypePtr>& DefaultTypeFactory::
    basePythonTypes() {
  static const std::unordered_map<std::string, MSharp::TypePtr> map = {
#define MAP_ITEM(NAME, TYPE) {#NAME, MSharp::TYPE::get()},
      FORALL_BASE_PYTHON_TYPES(MAP_ITEM)
#undef MAP_ITEM
  };
  return map;
}

MSharp::TypePtr DefaultTypeFactory::createNamedTuple(
    const std::string& name,
    const std::vector<std::string_view>& fields,
    const std::vector<MSharp::TypePtr>& types) {
  return MSharp::TupleType::createNamed(name, fields, types);
}

} // namespace MSharp
