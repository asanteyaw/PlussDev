set(NODEJ_CORE_DEMOS
  nodej_prep_split_demo
  nodej_model_families_demo
)

foreach(demo ${NODEJ_CORE_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
endforeach()
