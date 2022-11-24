#ifndef DIFF_CALC_H
#define DIFF_CALC_H

#include "tree_output.h"
#include "tree.h"

namespace tree {
    tree_t calc_diff (const tree_t *src, char var = 'x', render::render_t *render = nullptr);

    void simplify (tree_t *tree, render::render_t *render = nullptr);
    void simplify (node_t *node, render::render_t *render = nullptr);

    tree_t taylor_series (const tree_t *src, int order, render::render_t *render = nullptr);
}

#endif