set(NODEB_EXAMPLES
  nodeb_textcat_mvp
  nodeb_linguistics_overview_demo
  nodeb_sentiment_end_to_end
  nodeb_tagger_sequence_end_to_end
  nodeb_ner_end_to_end
  nodeb_embed_end_to_end
  nodeb_contextual_generation_end_to_end
  nodeb_vectorize_end_to_end
  nodeb_full_nlp_workflow_demo
  nodeb_docs_reference
  nodeb_core_docs_reference
  nodeb_tasks_docs_reference
  nodeb_embed_docs_reference
)

foreach(demo ${NODEB_EXAMPLES})
  add_executable(plussai_${demo}
    ${demo}.cpp
  )
  target_link_libraries(plussai_${demo} PRIVATE pluss)
  target_compile_features(plussai_${demo} PRIVATE cxx_std_20)
endforeach()
