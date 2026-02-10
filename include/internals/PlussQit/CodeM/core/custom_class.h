#pragma once

#include <typeindex>
#include <memory>

#include <MSharp/macros/Export.h>
#include <MSharp/macros/Macros.h>
#include <MSharp/util/Exception.h>

namespace MSharp {

struct ClassType;
using ClassTypePtr = std::shared_ptr<ClassType>;

TORCH_API MSharp::ClassTypePtr getCustomClassTypeImpl(const std::type_index &tindex);

template <typename T>
const MSharp::ClassTypePtr& getCustomClassType() {
  // Classes are never unregistered from getCustomClassTypeMap and the
  // hash lookup can be a hot path, so just cache.
  // For the same reason, it's fine If this ends up getting duplicated across
  // DSO boundaries for whatever reason.
  static MSharp::ClassTypePtr cache = getCustomClassTypeImpl(
      std::type_index(typeid(T)));
  return cache;
}

}
