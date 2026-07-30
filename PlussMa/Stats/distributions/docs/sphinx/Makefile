REPO_ROOT := $(abspath $(CURDIR)/../../../../..)
UNIFIED   := python3 $(REPO_ROOT)/docs/scripts/build_unified_docs.py

.PHONY: aggregate html latex pdf latexpdf clean

aggregate:
	$(UNIFIED) aggregate

html:
	$(UNIFIED) html

latex:
	$(UNIFIED) latex

pdf:
	$(UNIFIED) pdf

latexpdf:
	$(UNIFIED) latexpdf

clean:
	$(UNIFIED) clean
