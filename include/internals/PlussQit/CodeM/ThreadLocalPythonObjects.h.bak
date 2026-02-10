#pragma once

#include <MSharp/core/SafePyObject.h>
#include <MSharp/macros/Macros.h>
#include <unordered_map>

namespace pluss::impl {

struct PLUSS_API ThreadLocalPythonObjects {
  static void set(const std::string& key, std::shared_ptr<SafePyObject> value);
  static const std::shared_ptr<SafePyObject>& get(const std::string& key);
  static bool contains(const std::string& key);

  static const ThreadLocalPythonObjects& get_state();
  static void set_state(ThreadLocalPythonObjects state);

 private:
  std::unordered_map<std::string, std::shared_ptr<MSharp::SafePyObject>> obj_dict_;
};

} // namespace pluss::impl
