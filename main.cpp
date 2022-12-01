#include <cassert>
#include <cmath>
#include <cstdlib>
#include <time.h>
#include <stdio.h>
#include "common.h"
#include "tree.h"
#include "diff_calc.h"
#include "tree_output.h"

// Исправить и будет 11

// В начале выводится функция, с которой работаем
// Несколько формул на слайде, если они короткие
// Перенос формул внутри слайда
// Опустить ненужные cdot и скобочки аргументов функций
// Если возможно, сделать обозначения буквами
// Скрипт генерирует хардсабы и встраивает в файл
// Расставить ударения


// МОЖЕТ БЫТЬ allowbreak  не работает внутри скобочек

extern void subtree_dump (tree::node_t *node, FILE *stream); // REMOVE THIS 

int demonstrate_diff   (render::render_t *render);
int demonstrate_taylor (render::render_t *render);
int demonstrate_calc   (render::render_t *render);

const char DIFF_FILENAME[]   = "diff.txt";
const char TAYLOR_FILENAME[] = "diff.txt";
const char CALC_FILENAME[]   = "diff.txt";

const int TAYLOR_ORDER = 3;

#define TRY(expr)           \
{                           \
    if ((expr) == ERROR)    \
    {                       \
        return ERROR;       \
    }                       \
}

#include "tree_dsl.h"

int main()
{
    srand ((unsigned int) time(NULL));

    render::render_t render = {};
    render::render_ctor (&render, "render/main.tex", "render/apndx.tex", "render/voice.txt");

    render::push_section (&render, "Дифференцирование");
    TRY (demonstrate_diff     (&render));

    render::push_section (&render, "Тейлор");
    TRY (demonstrate_taylor   (&render));

    render::push_section (&render, "Подсчет третьего выражения");
    TRY (demonstrate_calc     (&render));
    
    render::push_section(&render,    "kawaii");
    render::push_subsection(&render, "koto");
    render::push_raw_frame (&render, "Спасибо за потраченное время ;/", "И так коллеги, выжившие есть?...");

    render::render_dtor (&render);
}

int demonstrate_diff (render::render_t *render)
{
    assert (render != nullptr && "invalid pointer");

    FILE *dump = fopen (DIFF_FILENAME, "r");

    if (dump == nullptr) {
        printf ("Failed to open file\n");
        return ERROR;
    }

    tree::tree_t tree       = {};
    tree::ctor (&tree);
    tree::load (&tree, dump);

    if (tree.head_node == nullptr) {
        printf ("Invalid input file\n");
        return ERROR;
    }

    render::push_subsection (render, "Дифференцирование");
    tree::tree_t res = calc_diff (&tree, 'x', render, true);
    
    render::push_subsection (render, "Упрощение");
    tree::simplify (&res, render);

    tree::dtor(&tree);
    tree::dtor(&res);
    return 0;
}

int demonstrate_taylor (render::render_t *render)
{
    assert (render != nullptr && "invalid pointer");

    FILE *dump = fopen (TAYLOR_FILENAME, "r");

    if (dump == nullptr) {
        printf ("Failed to open file\n");
        return ERROR;
    }

    tree::tree_t tree       = {};
    tree::ctor (&tree);
    tree::load (&tree, dump);

    if (tree.head_node == nullptr) {
        printf ("Invalid input file\n");
        return ERROR;
    }

    tree::tree_t res = tree::taylor_series (&tree, TAYLOR_ORDER, render);

    // tree::graph_dump (&res, "after taylor");

    tree::dtor(&tree);
    tree::dtor(&res);

    return 0;
}

int demonstrate_calc (render::render_t *render)
{
    assert (render != nullptr && "invalid pointer");
    
    FILE *dump = fopen (CALC_FILENAME, "r");

    if (dump == nullptr) {
        printf ("Failed to open file");
        return ERROR;
    }

    tree::tree_t tree = {};
    tree::ctor (&tree);
    tree::load (&tree, dump);

    if (tree.head_node == nullptr) {
        printf ("Invalid input file\n");
        return ERROR;
    }

    double x_val = 0;
    // printf ("Введите значение x для третьего уравнения: ");
    // while (scanf ("%lg", &x_val) != 1)
    // {
    //     printf ("\n Неверный ввод, повторите");
    //     while (getc (stdin) != '\n') ;
    // }

    tree::calc_tree (&tree, x_val, render);

    tree::dtor(&tree);
    return 0;
}