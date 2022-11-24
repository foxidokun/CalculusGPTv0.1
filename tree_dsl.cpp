#include "tree_dsl.h"
#include "tree.h"

static tree::node_t *op_with_childs (tree::node_t *lhs, tree::node_t *rhs, tree::op_t op);

tree::node_t *add (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs (lhs, rhs, tree::op_t::ADD);
}

tree::node_t *sub (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::SUB);
}

tree::node_t *div (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::DIV);
}

tree::node_t *mul (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::MUL);
}

tree::node_t *pow (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::POW);
}

tree::node_t *sin (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::SIN);
}

tree::node_t *cos (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::COS);
}

tree::node_t *log (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::LOG);
}

tree::node_t *exp (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::EXP);
}

tree::node_t *fact (int n)
{
    tree::node_t *cur_node = tree::new_node(1.0);

    for (int i = 2; i <= n; ++i)
    {
        cur_node = mul (cur_node, tree::new_node ((double) i));
    }

    return cur_node;
}
// ----------------------------------------------------------------------------

static tree::node_t *op_with_childs (tree::node_t *lhs, tree::node_t *rhs, tree::op_t op)
{
    tree::node_t *op_node = tree::new_node(op);
    if (op_node == nullptr) return nullptr;

    op_node->left  = lhs;
    op_node->right = rhs;

    return op_node;
}