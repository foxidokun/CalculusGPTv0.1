const char MAIN_BEGIN[] = 
"\\documentclass[8pt]{beamer}\n"
"\\usepackage{amsmath,amsthm,amssymb,amsfonts}\n"
"\\usepackage[utf8]{inputenc}\n"
"\\usepackage[T1]{fontenc}\n"
"\\usepackage[english,russian]{babel}\n"
"\\usepackage{breqn}\n"
"\\geometry{paperwidth=200mm,paperheight=110mm}\n"
"\\title{Введение в сорты матанализа}\n"
"\\author{Гладышев Илья / Б05-233}\n"
"\\institute{MIPT}\n"
"\\graphicspath{ {assets} }\n"
"\n"
// "\\usepackage{tikz}\n"
// "\n"
// "\\usetikzlibrary{shapes.callouts, calc}\n"
// "\n"
// "\\newcommand\\DuckSetup[3]{\n"
// "\\foreach \\n in {1,...,#2}{\n"
// "\\pgfdeclareimage[width=#3,page=\\n]{duck\\n}{#1}}\n"
// "\\def\\ducknumberofpages{#2}}\n"
// "\n"
// "\\DuckSetup{livsi}{2}{2cm}\n"
// "\n"
// "\\newcommand\\duck{\n"
// "\\tikz[remember picture]{\\node (duck) {\n"
// "\\pgfmathparse{int(mod(\\thepage-1,\\ducknumberofpages)+1)}\n"
// "\\pgfuseimage{duck\\pgfmathresult}};}\n"
// "}\n"
// "\n"
// "\\setbeamertemplate{footline}\n"
// "{\n"
// "\\pgfmathparse{(\\thepage-1)*\\paperwidth/\\insertdocumentendpage}\n"
// "\\hspace{\\pgfmathresult pt}\n"
// "\\duck\n"
// "}\n"
// "\n"
// "\\setbeamertemplate{navigation symbols}{}\n"
"\n"
"\\newcommand<>{\\ducksez}[1]{\n"
"\\uncover#2{\\tikz[remember picture,overlay]{\\node[ellipse callout, draw, fill=white, overlay,\n"
"callout absolute pointer={($ (duck.north east) + (1,0) $)}] at ($ (duck.north east) + (3,1)\n"
"$) {#1};}}}\n"
"\n"
"\\newcommand<>{\\ducksezrev}[1]{\n"
"\\uncover#2{\\tikz[remember picture,overlay]{\\node[ellipse callout, draw, fill=white, overlay,\n"
"callout absolute pointer={(duck.north west)}] at ($ (duck.north west) + (-3,1) $) {#1};}}}\n"
"\n"
"\\begin{document}\n"
"\n"
"\\frame{\\titlepage}\n"
"\n"
"\\begin{frame}{План лекции}\n"
"    \\begin {itemize}\n"
"        \\item Покрываем себя в матанализ\n"
"        \\item LaTeX круто\n"
"        \\item Прога позволяет их объединить\n"
"    \\end {itemize}\n"
"\\end{frame}\n";

const char MAIN_END[] = "\\end{document}\n";

const char APPENDIX_BEGIN[] =
"\\documentclass[8pt,a3paper]{article}\n"
"\\usepackage{amsmath,amsthm,amssymb,amsfonts}\n"
"\\usepackage[utf8]{inputenc}\n"
"\\usepackage[T1]{fontenc}\n"
"\\usepackage[english,russian]{babel}\n"
"\\usepackage{breqn}\n"
"\\usepackage[left=1.00cm, right=1.00cm, top=1.00cm, bottom=1.50cm]{geometry}\n"
"\n"
"\\title{Введение в сорты матанализа}\n"
"\\author{Гладышев Илья / Б05-233}\n"
"\n"
"\\begin{document}\n"
"\\maketitle\n";

const char APPENDIX_END[]   = "\\end{document}\n";

const char FRAME_BEG[] = "\\begin{frame}{\\secname :: \\subsecname}\n\\begin{dmath}\n";
const char FRAME_END[] = "\n\\end{dmath}\n\\end{frame}\n";

const char FRAME_BLOCK_BEG[] = "\\begin{frame}{\\secname ::\\subsecname}\n";
const char FRAME_BLOCK_END[] = "\\end{frame}\n";

const char APDX_FRAME_BEG[] = "\n";
const char APDX_FRAME_END[] = "\n";

const char FORMULA_BEG[]    = "\\begin{dmath}";
const char FORMULA_END[]    = "\\end{dmath}";

const char SPEECH_BEGIN[]   =
"Дамы и господа, рад приветствовать на своем канале, где я пытаюсь просто и понятно объяснить основы матан+ализа "
"на простейших примерах. Прошу подписаться и поставить лайк лютому.\n"

"Чтож тут изображено краткое оглавление последубщего курса, надеюсь оно вас заинтересовало "
"достаточно, чтобы решиться на такой важный шаг, как изучение матан+ализа в первый раз. "
"Многие пытались и не осилили, но автор верит в вас\n";

const int NUM_PHRASES = 47;

const char* const PHRASES[NUM_PHRASES] = 
{
    "Береги пиво схолоду, а доказательство представлено на слайде",
    "Очевидно, что",
    "Не трудно заметить, что",
    "Отметим, что",
    "С другой стороны",
    "Таким образом",
    "Имеем",
    "Поэтому",
    "Откуда",
    "Кроме того",
    "Оказывается",
    "Положим",
    "При этом",
    "Говорят",
    "Руководствуясь базовой логикой, получаем",
    "Продвинутый читатель уже заметил, что",
    "Вы не шокированы?",
    "ИИИИЕЕЕЕсли",
    "Для любого эпсилон больше нулю очевидно, что",
    "От коробки до нк все знают, что",
    "По теореме Эскобара",
    "Используя теорему Симонайтеса-Рамануджана",
    "По лемме квадратного корня из минус 759",
    "Используя выводы из теоремы 1000-7 получаем",
    "Дураку понятно, что",
    "И хотя клуб любителей таких формул двумя блоками ниже, мы продолжаем",
    "Руководствуясь сборником 'Задачи для подготовки к поступлению в советские ясли'",
    "Здесь могла быть ваша реклама",
    "бип буп буп бииип",
    "Сегментационная ошибка (ядро сброшено)",
    "Телец в козероге, поэтому",
    "Автору приснилось, что следующее преобразование верно",
    "Обоснование этого пререхода предостовляется читателю в качестве несложного упрожнения",
    "Обоснование этого пререхода предостовляется читателю в платном DLC",
    "Обоснование этого перехода было забанено редактурой",
    "[Данные удалены]",
    "Доказательство данного факта предоставлено лицом или организацией исполняющей функции иностанного агента",
    "Я придумал поистине удивительное доказательство этого факта, но поля этой книги слишком малы...",
    "Нам не объяснили на семинаре как это делать, поэтому примем на веру",
    "Как будет доказано в следующем семестре",
    "Лёша, придумай переход. У меня идеи закончились",
    "Без комментариев",
    "Если вы не понимаете этот переход, то я вам сочувствую",
    "Единственное, что я не понимаю, так это то, зачем ты это читаешь",
    "Ну вот как это матан тебе в жизни пригодится?",
    "Если посмотреть на выражение под другим углом, можно получить",
    "Redkozubov is not in the sudoers file. This incident will be reported",
};
