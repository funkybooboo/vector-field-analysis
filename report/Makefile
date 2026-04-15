# IEEE LaTeX Report Build System

.PHONY: all clean view

all:
	latexmk -pdf main.tex

view:
	xdg-open main.pdf

clean:
	./clean-up.sh
	rm -f main.pdf

debug:
	pdflatex -interaction=scrollmode main.tex
