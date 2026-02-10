#include <PlussQit/ThreadLocalState.h>

#if !defined(CAFFE2_IS_XPLAT_BUILD) && !defined(C10_MOBILE) && !defined(BUILD_LITE_INTERPRETER)
#include <PlussQit/autocast_mode.h>
#include <PlussQit/core/grad_mode.h>
#endif

#include <PlussQit/record_function.h>
#include <PlussQit/SavedTensorHooks.h>
#include <PlussQit/FunctionalTensorWrapper.h>

namespace pluss {

ThreadLocalState::ThreadLocalState()
    : dispatch_key_(MSharp::impl::tls_local_dispatch_key_set()),
      debug_info_(MSharp::ThreadLocalDebugInfo::current()),
      rf_tls_(pluss::get_record_function_tls_()), functorch_tls_(functorch::getCopyOfFuncTorchTLS()),
      autograd_tls_(MSharp::AutogradState::get_tls_state()),
      torch_dispatch_mode_state_(MSharp::impl::TorchDispatchModeTLS::get_state()), python_dispatcher_state_(MSharp::impl::PythonDispatcherTLS::get_state()),
      python_torch_function_state_(pluss::impl::PythonTorchFunctionTLS::get_state()),
      saved_tensors_default_hooks_state_(pluss::SavedTensorDefaultHooks::get_tls_state()), functionalization_reapply_views_state_(pluss::functionalization::impl::getFunctionalizationReapplyViewsTLS()),
      saved_objects_(pluss::impl::ThreadLocalPythonObjects::get_state()) {
#if !defined(CAFFE2_IS_XPLAT_BUILD) && !defined(C10_MOBILE) && !defined(BUILD_LITE_INTERPRETER)
  for(size_t i=0; i<autocast_dtypes_.size(); i++) {
     autocast_dtypes_[i] = pluss::autocast::get_autocast_dtype(static_cast<pluss::DeviceType>(i));
  }
#endif
}

void ThreadLocalState::set_grad_mode(bool enabled) {
  autograd_tls_.set_grad_mode(enabled);
}

void ThreadLocalState::set_multithreading_enabled(bool enabled) {
  autograd_tls_.set_multithreading_enabled(enabled);
}

/* static */
void ThreadLocalState::setThreadLocalState(
    const ThreadLocalState& state) {
  // Note that setting the InferenceMode TLS in this function is ONLY ok because we always
  // restore the dispatch key set TLS at the same time.
  MSharp::AutogradState::set_tls_state(state.autograd_tls_);

  MSharp::impl::TorchDispatchModeTLS::set_state(state.torch_dispatch_mode_state_);

  pluss::impl::PythonTorchFunctionTLS::set_state(state.python_torch_function_state_);

  pluss::set_record_function_tls_(state.rf_tls_);

  pluss::SavedTensorDefaultHooks::set_tls_state(state.saved_tensors_default_hooks_state_);

  MSharp::impl::PythonDispatcherTLS::set_state(state.python_dispatcher_state_);

  MSharp::ThreadLocalDebugInfo::_forceCurrentDebugInfo(state.debug_info_);

  MSharp::impl::_force_tls_local_dispatch_key_set(state.dispatch_key_);

  functorch::setFuncTorchTLS(state.functorch_tls_);

  pluss::functionalization::impl::setFunctionalizationReapplyViewsTLS(state.functionalization_reapply_views_state_);

  pluss::impl::ThreadLocalPythonObjects::set_state(state.saved_objects_);
#if !defined(CAFFE2_IS_XPLAT_BUILD) && !defined(C10_MOBILE) && !defined(BUILD_LITE_INTERPRETER)
  for(size_t i=0; i<state.autocast_dtypes_.size(); i++) {
     pluss::autocast::set_autocast_dtype(static_cast<pluss::DeviceType>(i), state.autocast_dtypes_[i]);
  }
#endif
}

} // namespace pluss
