#pragma once

#include <type_traits>
#include <unordered_map>

#include <PlussQit/core/dynamic_type.h>
#include <PlussQit/core/jit_type_base.h>
#include <MSharp/macros/Macros.h>

namespace MSharp {

template <typename T>
struct TORCH_API TypeFactoryBase {};

template <>
struct TORCH_API TypeFactoryBase<MSharp::DynamicType> {
  template <typename T, typename... Args>
  static MSharp::DynamicTypePtr create(TypePtr ty, Args&&... args) {
    return std::make_shared<MSharp::DynamicType>(
        MSharp::DynamicTypeTrait<T>::tagValue(),
        MSharp::DynamicType::Arguments(MSharp::ArrayRef<MSharp::TypePtr>(
            {std::move(ty), std::forward<Args>(args)...})));
  }
  template <typename T>
  static MSharp::DynamicTypePtr create(const std::vector<MSharp::TypePtr>& types) {
    return std::make_shared<MSharp::DynamicType>(
        MSharp::DynamicTypeTrait<T>::tagValue(),
        MSharp::DynamicType::Arguments(types));
  }
  static MSharp::DynamicTypePtr createNamedTuple(
      const std::string& name,
      const std::vector<std::string_view>& fields,
      const std::vector<MSharp::TypePtr>& types) {
    return std::make_shared<MSharp::DynamicType>(
        MSharp::DynamicType::Tag::Tuple,
        name,
        MSharp::DynamicType::Arguments(fields, types));
  }
  template <typename T>
  C10_ERASE static MSharp::DynamicTypePtr createNamed(const std::string& name) {
    return std::make_shared<MSharp::DynamicType>(
        MSharp::DynamicTypeTrait<T>::tagValue(),
        name,
        MSharp::DynamicType::Arguments{});
  }
  template <typename T>
  C10_ERASE static MSharp::DynamicTypePtr get() {
    return DynamicTypeTrait<T>::getBaseType();
  }
  static const std::unordered_map<std::string, MSharp::TypePtr>& basePythonTypes();
};

using DynamicTypeFactory = TypeFactoryBase<MSharp::DynamicType>;

// Helper functions for constructing DynamicTypes inline.
template <
    typename T,
    std::enable_if_t<DynamicTypeTrait<T>::isBaseType, int> = 0>
C10_ERASE DynamicTypePtr dynT() {
  return DynamicTypeFactory::get<T>();
}

template <
    typename T,
    typename... Args,
    std::enable_if_t<!DynamicTypeTrait<T>::isBaseType, int> = 0>
C10_ERASE DynamicTypePtr dynT(Args&&... args) {
  return DynamicTypeFactory::create<T>(std::forward<Args>(args)...);
}

template <>
struct TORCH_API TypeFactoryBase<MSharp::Type> {
  template <typename T, typename... Args>
  static MSharp::TypePtr create(TypePtr ty, Args&&... args) {
    return T::create(std::move(ty), std::forward<Args>(args)...);
  }
  template <typename T>
  static MSharp::TypePtr create(std::vector<MSharp::TypePtr> types) {
    return T::create(std::move(types));
  }
  static MSharp::TypePtr createNamedTuple(
      const std::string& name,
      const std::vector<std::string_view>& fields,
      const std::vector<MSharp::TypePtr>& types);
  template <typename T>
  C10_ERASE static MSharp::TypePtr createNamed(const std::string& name) {
    return T::create(name);
  }
  static const std::unordered_map<std::string, MSharp::TypePtr>& basePythonTypes();
  template <typename T>
  C10_ERASE static MSharp::TypePtr get() {
    return T::get();
  }
};

using DefaultTypeFactory = TypeFactoryBase<MSharp::Type>;

using PlatformType =
#ifdef C10_MOBILE
    MSharp::DynamicType
#else
    MSharp::Type
#endif
    ;

using TypeFactory = TypeFactoryBase<PlatformType>;

} // namespace MSharp
