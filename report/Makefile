# IEEE LaTeX Report Build System

.PHONY: all clean view watch debug

LATEXMK  := $(shell command -v latexmk  2>/dev/null)
TECTONIC := $(shell command -v tectonic 2>/dev/null)
PDFLATEX := $(shell command -v pdflatex 2>/dev/null)

all:
ifneq ($(LATEXMK),)
	latexmk -pdf -jobname=report main.tex
else ifneq ($(TECTONIC),)
	tectonic main.tex && mv main.pdf report.pdf
else ifneq ($(PDFLATEX),)
	pdflatex -jobname=report main.tex
	bibtex report
	pdflatex -jobname=report main.tex
	pdflatex -jobname=report main.tex
else
	$(error No LaTeX compiler found: install latexmk, tectonic, or pdflatex)
endif

watch:
ifneq ($(LATEXMK),)
	latexmk -pvc -jobname=report main.tex
else
	$(error watch requires latexmk: tectonic and pdflatex have no watch mode)
endif

view:
	xdg-open report.pdf

clean:
	./clean-up.sh
	rm -f report.pdf

debug:
ifneq ($(LATEXMK),)
	latexmk -pdf -jobname=report -interaction=scrollmode main.tex
else ifneq ($(TECTONIC),)
	tectonic main.tex && mv main.pdf report.pdf
else ifneq ($(PDFLATEX),)
	pdflatex -jobname=report -interaction=scrollmode main.tex
else
	$(error No LaTeX compiler found: install latexmk, tectonic, or pdflatex)
endif
