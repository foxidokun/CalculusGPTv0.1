#ifndef DIFF_CALC_H
#define DIFF_CALC_H

#include "tree.h"

namespace tree {
    tree_t calc_diff (const tree_t *src);

    void simplify (tree_t *tree);
}

#endif