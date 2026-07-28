# NodeB Examples

Current end-to-end demos:

- `nodeb_linguistics_overview_demo.cpp`
  Raw text to tokens, lemmas, sentences, matcher hits, entities, and syntax phrases.
- `nodeb_sentiment_end_to_end.cpp`
  Sequence-backed document classification used as sentiment analysis.
- `nodeb_tagger_sequence_end_to_end.cpp`
  Train and infer a learned per-token POS-style tagger.
- `nodeb_ner_end_to_end.cpp`
  Train and infer a learned BIO-style sequence NER model.
- `nodeb_embed_end_to_end.cpp`
  Unsupervised static embeddings with nearest-neighbor lookup and similarity.
- `nodeb_contextual_generation_end_to_end.cpp`
  Contextual embeddings plus next-token generation.
- `nodeb_vectorize_end_to_end.cpp`
  Count/TF-IDF/hash vectorization and corpus statistics.
- `nodeb_full_nlp_workflow_demo.cpp`
  One combined workflow: tokenize, sentencize, lemmatize, learned tagger, learned NER, matcher, syntax, text classification, and save/load roundtrip.
- `nodeb_textcat_mvp.cpp`
  Minimal original text classification example.
