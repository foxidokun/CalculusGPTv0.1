#include <stdio.h>
#include "tree.h"
#include "diff_calc.h"

int main()
{
    FILE *dump = fopen ("inp.txt", "r");

    tree::tree_t tree;
    tree::ctor (&tree);

    tree::load (&tree, dump);

    tree::graph_dump (&tree, "test");

    tree::tree_t res = calc_diff (&tree);

    tree::graph_dump (&res, "after diff");

    tree::dtor(&tree);
}