#pragma once

#include <PlussQit/core/dispatch/Dispatcher.h>

// TODO: this can probably live in c10


namespace pluss::impl {

class TORCH_API PythonOpRegistrationTrampoline final {
  static std::atomic<MSharp::impl::PyInterpreter*> interpreter_;

public:
  //  Returns true if you successfully registered yourself (that means
  //  you are in the hot seat for doing the operator registrations!)
  static bool registerInterpreter(MSharp::impl::PyInterpreter*);

  // Returns nullptr if no interpreter has been registered yet.
  static MSharp::impl::PyInterpreter* getInterpreter();
};

} // namespace pluss::impl
