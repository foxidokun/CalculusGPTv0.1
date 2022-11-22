#!/usr/bin/fish

cd render

for file in *.tex
	pdflatex $file
end
