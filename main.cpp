#include <cassert>
#include <cstdlib>
#include <time.h>
#include <stdio.h>
#include "common.h"
#include "tree.h"
#include "diff_calc.h"
#include "tree_output.h"

int main()
{
    srand ((unsigned int) time(NULL));

    FILE *dump = fopen ("inp.txt", "r");

    tree::tree_t tree       = {};
    tree::ctor (&tree);
    tree::load (&tree, dump);

    if (tree.head_node == nullptr) {
        printf ("Invalid input file\n");
        return ERROR;
    }

    render::render_t render = {};
    render::render_ctor (&render, "render/main.tex", "render/apndx.tex", "render/voice.txt");

    render::push_section    (&render, "Разбор первого выражения");
    render::push_subsection (&render, "Диффиринцирование");
    tree::tree_t res = calc_diff (&tree, 'x',&render);
    
    render::push_subsection (&render, "Упрощение");
    tree::simplify (&res, &render);

    render::push_section (&render, "Разбор второго выражения");

    tree::tree_t taylor_tree       = {};
    tree::tree_t taylor_res        = {};
    tree::ctor (&taylor_tree);
    tree::ctor (&taylor_res);
    tree::load (&taylor_tree, fopen ("taylor.txt", "r"));

    taylor_res = tree::taylor_series (&taylor_tree, 5, &render);


    render::push_section (&render, "Подсчет третьего выражения");
    tree::tree_t calculation       = {};
    tree::ctor (&calculation);
    tree::load (&calculation, fopen ("calc.txt", "r")); 

    tree::graph_dump (&calculation, "for fun");
    tree::calc_tree (&calculation, 5, &render);

    render::push_raw_frame (&render, "Спасибо за потраченное время ;/", "И так коллеги, выжившие есть?...");

    tree::dtor(&tree);
    tree::dtor(&res);
    tree::dtor(&taylor_tree);
    tree::dtor(&taylor_res);
    tree::dtor(&calculation);
    render::render_dtor (&render);
}