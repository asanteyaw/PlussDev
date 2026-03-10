# Natural Sciences top-level dispatcher

if(PLUSSMA_NATSCI_ENABLE_PHYSICS)
  add_subdirectory(physics)
endif()
if(PLUSSMA_NATSCI_ENABLE_CHEMISTRY)
  add_subdirectory(chemistry)
endif()
if(PLUSSMA_NATSCI_ENABLE_BIOLOGY)
  add_subdirectory(biology)
endif()
if(PLUSSMA_NATSCI_ENABLE_COMMON)
  add_subdirectory(common)
endif()
