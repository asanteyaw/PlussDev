# ENGINEERING Subspace

if(PLUSSMA_ENG_ENABLE_CONTROL)
  add_subdirectory(control)
endif()
if(PLUSSMA_ENG_ENABLE_SIGNAL_PROC)
  add_subdirectory(signal_proc)
endif()
