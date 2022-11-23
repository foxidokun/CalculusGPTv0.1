const char MAIN_BEGIN[] = 
"\\documentclass[8pt]{beamer}\n"
"\\usepackage{amsmath,amsthm,amssymb,amsfonts}\n"
"\\usepackage[utf8]{inputenc}\n"
"\\usepackage[T1]{fontenc}\n"
"\\usepackage[english,russian]{babel}\n"
"\\usepackage{breqn}\n"
"\n"
"\\title{Введение в сорты матанализа}\n"
"\\author{Гладышев Илья / Б05-233}\n"
"\\institute{MIPT}\n"
"\\graphicspath{ {assets} }\n"
"\\logo{\\includegraphics[height=1cm]{rei}}\n"
"\n"
"\\begin{document}\n"
"\n"
"\\frame{\\titlepage}\n"
"\n"
"\\begin{frame}{Introduction}\n"
"    \\begin {itemize}\n"
"        \\item Матанализ это весело\n"
"        \\item LaTeX круто\n"
"        \\item Прога позволяет их объединить\n"
"    \\end {itemize}\n"
"\\end{frame}\n"
"\n"
;

const char MAIN_END[] = "\\end{document}\n";

const char APPENDIX_BEGIN[] =
"\\documentclass[12pt,a4paper]{article}\n"
"\\usepackage{amsmath,amsthm,amssymb,amsfonts}\n"
"\\usepackage[utf8]{inputenc}\n"
"\\usepackage[T1]{fontenc}\n"
"\\usepackage[english,russian]{babel}\n"
"\n"
"\\title{Введение в сорты матанализа}\n"
"\\author{Гладышев Илья / Б05-233}\n"
"\n"
"\\begin{document}\n"
"\\maketitle\n";

const char APPENDIX_END[]   = "\\end{document}\n";

const char FRAME_BEG[] = "\\begin{frame}\n\\begin{dmath}\n";
const char FRAME_END[] = "\n\\end{dmath}\n\\end{frame}\n";

const char APDX_FRAME_BEG[] = "\n";
const char APDX_FRAME_END[] = "\n";

const char FORMULA_BEG[]    = "\\[";
const char FORMULA_END[]    = "\\]";