#include <stdio.h>
#include "tree.h"
#include "diff_calc.h"
#include "tree_output.h"

int main()
{
    FILE *dump = fopen ("inp.txt", "r");

    tree::tree_t tree;
    tree::ctor (&tree);

    tree::load (&tree, dump);

    tree::graph_dump (&tree, "test");

    tree::tree_t res = calc_diff (&tree);

    tree::graph_dump (&res, "after diff");

    tree::tree_t *trees[] = {&res};

    tree::render_tex (trees, 1);

    tree::simplify (&res);
    tree::graph_dump (&res, "simplified");

    tree::dtor(&tree);
    tree::dtor(&res);
}