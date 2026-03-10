SPHINXBUILD ?= sphinx-build
SOURCEDIR   := .
BUILDDIR    := _build

.PHONY: html latex pdf latexpdf clean

html:
	$(SPHINXBUILD) -b html $(SOURCEDIR) $(BUILDDIR)/html

latex:
	$(SPHINXBUILD) -b latex $(SOURCEDIR) $(BUILDDIR)/latex

pdf: latex
	cd $(BUILDDIR)/latex && latexmk -pdf -quiet *.tex

latexpdf:
	$(SPHINXBUILD) -b latexpdf $(SOURCEDIR) $(BUILDDIR)/latex

clean:
	rm -rf $(BUILDDIR)
