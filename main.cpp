#include <stdio.h>
#include "tree.h"

int main()
{
    FILE *dump = fopen ("dump.txt", "r");

    tree::tree_t tree;
    tree::ctor (&tree);

    tree::load (&tree, dump);

    tree::graph_dump (&tree, "test");

    tree::dtor(&tree);
}