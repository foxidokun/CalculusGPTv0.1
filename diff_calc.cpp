#include <assert.h>
#include <cstddef>

#include "tree.h"
#include "diff_calc.h"

// ----------------------------------------------------------------------------
// STATIC HEADER SECTION
// ----------------------------------------------------------------------------

static tree::node_t *diff_subtree (tree::node_t *node);
static tree::node_t *diff_op      (tree::node_t *node);

static tree::node_t *copy_subtree (tree::node_t *node);

static tree::node_t *op_with_childs (tree::node_t *lhs, tree::node_t *rhs, tree::op_t op);
    
    static tree::node_t *add (tree::node_t *lhs, tree::node_t *rhs);
    static tree::node_t *sub (tree::node_t *lhs, tree::node_t *rhs);
    static tree::node_t *div (tree::node_t *lhs, tree::node_t *rhs);
    static tree::node_t *mul (tree::node_t *lhs, tree::node_t *rhs);
    static tree::node_t *sin (tree::node_t *arg);
    static tree::node_t *cos (tree::node_t *arg);
    static tree::node_t *exp (tree::node_t *arg);
    static tree::node_t *log (tree::node_t *arg);
    static tree::node_t *pow (tree::node_t *lhs, tree::node_t *rhs);

// ----------------------------------------------------------------------------
// DEFINE SECTION
// ----------------------------------------------------------------------------

#define dR diff_subtree (node->right)
#define dL diff_subtree (node->left )
#define dA dR

#define cR copy_subtree (node->right)
#define cL copy_subtree (node->left )
#define cS copy_subtree (node)
#define cA cR

// ----------------------------------------------------------------------------
// PUBLIC SECTION
// ----------------------------------------------------------------------------

tree::tree_t calc_diff (const tree::tree_t *src)
{
    assert (src != nullptr);
    tree::tree_t res = {};
    tree::ctor (&res);

    res.head_node = diff_subtree (src->head_node);
    
    return res;
}

// ----------------------------------------------------------------------------
// STATIC SECTION
// ----------------------------------------------------------------------------

static tree::node_t *diff_subtree (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    switch (node->type)
    {
        case tree::node_type_t::VAL:
            return tree::new_node (0.0);

        case tree::node_type_t::VAR:
            if (node->var == 'x') return tree::new_node(1.0);
            else                  return tree::new_node(0.0);

        case tree::node_type_t::OP:
            return diff_op (node);

        case tree::node_type_t::NOT_SET:
            assert (0 && "Invalid node type for diff");
        
        default:
            assert (0 && "Unexpected node type");
    }
}

// ----------------------------------------------------------------------------

static tree::node_t *diff_op (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::OP && "invalid node");

    switch (node->op)
    {
        case tree::op_t::ADD:
            return add (dL, dR);

        case tree::op_t::SUB:
            return sub (dL, dR);
        
        case tree::op_t::DIV:
            return div ( sub(mul(dL, cR), mul(cL, dR)),
                                mul(cR, cR)
                       );

        case tree::op_t::MUL:
            return add (mul (dL, cR), mul(cL, dR));

        case tree::op_t::SIN:
            return mul (cos (cA), dA);

        case tree::op_t::COS:
            return mul (
                            mul (tree::new_node (-1.0), sin (cA)),
                            dA
                       );

        case tree::op_t::EXP:
            return mul (cS, dA);

        case tree::op_t::POW: //TODO Simplify in some cases
            return mul (
                        cS, 
                        add (
                            mul (dR, log(cL)),
                            mul (div(cR, cL), dL)
                        )
                    );

        case tree::op_t::LOG:
            return mul (
                        div (tree::new_node(1.0), cS), 
                        dA 
                       );

        default:
            assert(0 && "Unexpected op type");
            break;
        }
}

// ----------------------------------------------------------------------------

#define NEW_NODE_IN_CASE(type, field)               \
    case tree::node_type_t::type:                   \
        node_copy = tree::new_node (node->field);   \
        if (node_copy == nullptr) return nullptr;   \
        break;

static tree::node_t *copy_subtree (tree::node_t *node)
{
    assert (node != nullptr && "nvalid pointer");

    tree::node_t *node_copy = nullptr;

    switch (node->type)
    {
        NEW_NODE_IN_CASE(OP,  op);
        NEW_NODE_IN_CASE(VAL, val);
        NEW_NODE_IN_CASE(VAR, var);
        
        case tree::node_type_t::NOT_SET:
            assert (0 && "Incomplete node");
        default:
            assert (0 && "Invalid node type");
    }

    if (node->right != nullptr) {
        node_copy->right = copy_subtree (node->right);
    }

    if (node->left != nullptr) {
        node_copy->left  = copy_subtree (node->left);
    }

    return node_copy;
}

#undef NEW_NODE_IN_CASE

// ----------------------------------------------------------------------------

static tree::node_t *op_with_childs (tree::node_t *lhs, tree::node_t *rhs, tree::op_t op)
{
    tree::node_t *op_node = tree::new_node(op);
    if (op_node == nullptr) return nullptr;

    op_node->left  = lhs;
    op_node->right = rhs;

    return op_node;
}

// ----------------------------------------------------------------------------

static tree::node_t *add (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs (lhs, rhs, tree::op_t::ADD);
}

static tree::node_t *sub (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::SUB);
}

static tree::node_t *div (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::DIV);
}

static tree::node_t *mul (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::MUL);
}

static tree::node_t *sin (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::SIN);
}

static tree::node_t *cos (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::COS);
}

static tree::node_t *exp (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::EXP);
}

static tree::node_t *log (tree::node_t *arg)
{
    return op_with_childs(nullptr, arg, tree::op_t::LOG);
}

static tree::node_t *pow (tree::node_t *lhs, tree::node_t *rhs)
{
    return op_with_childs(lhs, rhs, tree::op_t::POW);
}

// ----------------------------------------------------------------------------