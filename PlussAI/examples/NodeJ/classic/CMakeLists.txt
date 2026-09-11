set(NODEJ_CLASSIC_DEMOS
  classic_core_status_example_1
  classic_core_statusor_example_1
  classic_core_statusor_example_2
  classic_core_statusor_example_3
  classic_core_randomstate_example_1
  classic_core_trainoptions_example_1
  classic_core_trainoptions_example_2
  classic_core_trainoptions_example_3
  classic_core_inferoptions_example_1
  classic_core_assessoptions_example_1
  classic_data_featurebatch_example_1
  classic_data_featurebatch_example_2
  classic_data_featurebatch_example_3
  classic_data_labeledbatch_example_1
  classic_data_labeledbatch_example_2
  classic_data_labeledbatch_example_3
  classic_data_dataframebatchoptions_example_1
  classic_data_feature_batch_from_dataframe_example_1
  classic_data_feature_batch_from_dataframe_example_2
  classic_data_feature_batch_from_dataframe_example_3
  classic_data_labeled_batch_from_dataframe_example_1
  classic_data_labeled_batch_from_dataframe_example_2
  classic_data_labeled_batch_from_dataframe_example_3
  classic_data_categorymap_example_1
  classic_families_modelspec_example_1
  classic_families_modelspec_example_2
  classic_families_modelspec_example_3
  classic_families_linearfamilyoptions_example_1
  classic_families_linearfamilyoptions_example_2
  classic_families_treefamilyoptions_example_1
  classic_families_ensemblefamilyoptions_example_1
  classic_families_neighborfamilyoptions_example_1
  classic_families_kernelfamilyoptions_example_1
  classic_families_bayesfamilyoptions_example_1
  classic_families_clusterfamilyoptions_example_1
  classic_families_decomposefamilyoptions_example_1
  classic_families_detectfamilyoptions_example_1
)

foreach(demo ${NODEJ_CLASSIC_DEMOS})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_link_libraries(plussai_${demo} PRIVATE $<TARGET_NAME_IF_EXISTS:PlussMic>)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
  target_compile_definitions(plussai_${demo} PRIVATE
    PLUSSMIC_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussMic"
    PLUSSAI_SOURCE_DIR="${CMAKE_SOURCE_DIR}/PlussAI"
  )
endforeach()
