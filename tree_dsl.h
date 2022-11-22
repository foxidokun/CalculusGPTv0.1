#ifndef TREE_DSL_H
#define TREE_DSL_H

#include "tree.h"

tree::node_t *add (tree::node_t *lhs, tree::node_t *rhs);
tree::node_t *sub (tree::node_t *lhs, tree::node_t *rhs);
tree::node_t *div (tree::node_t *lhs, tree::node_t *rhs);
tree::node_t *mul (tree::node_t *lhs, tree::node_t *rhs);
tree::node_t *pow (tree::node_t *lhs, tree::node_t *rhs);
tree::node_t *sin (tree::node_t *arg);
tree::node_t *cos (tree::node_t *arg);
tree::node_t *log (tree::node_t *arg);
tree::node_t *exp (tree::node_t *arg);

#endif