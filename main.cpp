#include <cassert>
#include <stdio.h>
#include "tree.h"
#include "diff_calc.h"
#include "tree_output.h"

int main()
{
    FILE *dump = fopen ("inp.txt", "r");

    tree::tree_t tree       = {};
    render::render_t render = {};

    tree::ctor (&tree);
    render::render_ctor (&render, "render/main.tex", "render/apndx.tex");

    tree::load (&tree, dump);

    assert (tree.head_node != nullptr);

    tree::graph_dump (&tree, "test");

    tree::tree_t res = calc_diff (&tree, &render);

    tree::graph_dump (&res, "after diff");

    tree::simplify (&res, &render);
    tree::graph_dump (&res, "simplified");

    tree::dtor(&tree);
    tree::dtor(&res);
    render::render_dtor (&render);
}