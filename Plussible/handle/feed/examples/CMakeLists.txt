# Feed examples
add_executable(feed_ex feed_ex.cpp)
target_link_libraries(feed_ex PRIVATE pluss)

# Text and NLP processing examples (uses Feed, Catalog, and Archive)
add_executable(text_nlp_ex ../../text_nlp_ex.cpp)
target_link_libraries(text_nlp_ex PRIVATE pluss)
