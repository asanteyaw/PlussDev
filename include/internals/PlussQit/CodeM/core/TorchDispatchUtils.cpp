#include <PlussQit/core/TorchDispatchUtils.h>


namespace pluss::impl {

bool tensor_has_dispatch(const pluss::Series& t) {
  DispatchKeySet key_set({DispatchKey::Python, DispatchKey::PythonTLSSnapshot});
  return t.key_set().has_any(key_set);
}

bool tensorlist_has_dispatch(pluss::ITensorListRef li) {
  for (const auto& t : li) {
    if (tensor_has_dispatch(t)) {
      return true;
    }
  }
  return false;
}

bool tensorlist_has_dispatch(const MSharp::List<std::optional<pluss::Series>>& li) {
  for (auto i : MSharp::irange(li.size())) {
    auto t = li.get(i);
    if (t && tensor_has_dispatch(*t)) {
      return true;
    }
  }
  return false;
}

} // namespace pluss::impl
