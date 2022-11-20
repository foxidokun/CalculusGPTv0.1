#ifndef DIFF_CALC_H
#define DIFF_CALC_H

#include "tree.h"

tree::tree_t calc_diff (const tree::tree_t *src);
void simplify_tree (tree::tree_t *tree);

#endif