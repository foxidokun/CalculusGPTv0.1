#include <assert.h>
#include <math.h>

#include "tree.h"
#include "tree_dsl.h"
#include "diff_calc.h"

// ----------------------------------------------------------------------------
// CONST SECTION
// ----------------------------------------------------------------------------

///@brief Floating point calculations accuracy
const double DBL_ERROR = 1e-11;

// ----------------------------------------------------------------------------
// STATIC HEADER SECTION
// ----------------------------------------------------------------------------

static tree::node_t *diff_subtree (tree::node_t *node);
static tree::node_t *diff_op      (tree::node_t *node);

static tree::node_t *copy_subtree (tree::node_t *node);

static bool simplify_const_subtree     (tree::node_t *node);
static bool simplify_primitive_subtree (tree::node_t *node);

static bool simplify_primitive_add     (tree::node_t *node);
static bool simplify_primitive_sub     (tree::node_t *node);
static bool simplify_primitive_add_sub (tree::node_t *node);
static bool simplify_primitive_mul     (tree::node_t *node);
static bool simplify_primitive_div     (tree::node_t *node);
static bool simplify_primitive_sin     (tree::node_t *node);
static bool simplify_primitive_cos     (tree::node_t *node);
static bool simplify_primitive_exp     (tree::node_t *node);
static bool simplify_primitive_pow     (tree::node_t *node);
static bool simplify_primitive_log     (tree::node_t *node);

static bool is_const_subtree (tree::node_t *start_node);

static bool iseq (double lhs, double rhs);

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

#define NEW(x) tree::new_node(x)

#define Lval node->left ->val
#define Rval node->right->val
#define Aval node->right->val

#define LL  node->left ->left
#define LR  node->left ->right
#define RL  node->right->left
#define RR  node->right->right
#define LLR node->left ->left ->right
#define LRR node->left ->right->right
#define RLR node->right->left ->right
#define RRR node->right->right->right

#define hasLeft  node->left  != nullptr
#define hasRight node->right != nullptr

#define notOP(node)  node->type != tree::node_type_t::OP
#define notVAL(node) node->type != tree::node_type_t::VAL

#define isOP(node)  node->type == tree::node_type_t::OP
#define isVAL(node) node->type == tree::node_type_t::VAL
#define isVAR(node) node->type == tree::node_type_t::VAR

#define isOP_TYPE(node, op_type) (node->type == tree::node_type_t::OP && node->op == tree::op_t::op_type)
#define isSINX(node) (isOP_TYPE (node, SIN) && (isVAR(node->right) && node->right->var == 'x'))
#define isCOSX(node) (isOP_TYPE (node, COS) && (isVAR(node->right) && node->right->var == 'x'))

#define diff_complex(func_diff) mul (func_diff, dA)

// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

tree::tree_t tree::calc_diff (const tree::tree_t *src)
{
    assert (src != nullptr);
    tree::tree_t res = {};
    tree::ctor (&res);

    res.head_node = diff_subtree (src->head_node);
    
    return res;
}

// -------------------------------------------------------------------------------------------------

void tree::simplify (tree::tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    bool not_simplified = true;

    while (not_simplified)
    {
        not_simplified &= simplify_const_subtree (tree->head_node);
        
        tree::graph_dump (tree, "Simplification round: const");
        
        not_simplified &= simplify_primitive_subtree (tree->head_node);

        tree::graph_dump (tree, "Simplification round: primitive");
    }
}

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

static tree::node_t *diff_subtree (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    switch (node->type)
    {
        case tree::node_type_t::VAL:
            return NEW (0.0);

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

// -------------------------------------------------------------------------------------------------

static tree::node_t *diff_op (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::OP && "invalid node");

// diff func доьавляет dA
    switch (node->op)
    {
        case tree::op_t::ADD:  return add (dL, dR);
        case tree::op_t::SUB:  return sub (dL, dR);
        
        case tree::op_t::DIV:
            return div ( sub(mul(dL, cR), mul(cL, dR)),
                                mul(cR, cR)
                       );

        case tree::op_t::MUL:
            return add (mul (dL, cR), mul(cL, dR));

        case tree::op_t::SIN:
            return diff_complex (cos (cA));

        case tree::op_t::COS:
            return diff_complex (mul (NEW(-1.0), sin (cA)));

        case tree::op_t::EXP:
            return diff_complex (cS);

        case tree::op_t::POW:
            if (is_const_subtree (node->right)) {
                return mul (mul (cR, 
                                pow (cL, sub (cR, NEW(1.0)))
                                ),
                           dL
                           );
            }
            else if (is_const_subtree (node->left)) {
                return mul (log (node->left), 
                            mul (cS, dR));
            } else {
                return mul (cS, 
                            add (
                                mul (dR, log(cL)),
                                mul (div(cR, cL), dL)
                            )
                        );
            }

        case tree::op_t::LOG:
            return diff_complex (div (NEW(1.0), cS));

        default:
            assert(0 && "Unexpected op type");
            break;
        }
}

// -------------------------------------------------------------------------------------------------

#define SIMPLIFY_BINARY_OP(type, op)             \
    case tree::op_t::type:                       \
        assert (hasLeft);                        \
        tree::change_node (node, Lval op Rval);  \
        break;

#define SIMPLIFY_UNARY_OP(type, func)            \
    case tree::op_t::type:                       \
        tree::change_node (node, func (Rval));   \
        break;

static bool simplify_const_subtree (tree::node_t *node)
{
    assert (node != nullptr);
    assert (node->type != tree::node_type_t::NOT_SET && "Incomplete node");

    if (notOP (node)) return false;

    bool simplified = false;

    if (hasLeft) {
        simplified |= simplify_const_subtree (node->left);
    }
    simplified |= simplify_const_subtree (node->right);

    if ((hasLeft && notVAL (node->left)) || notVAL (node->right)) {
        return simplified;
    }

    switch (node->op)
    {
        SIMPLIFY_BINARY_OP(ADD, +)
        SIMPLIFY_BINARY_OP(SUB, -)
        SIMPLIFY_BINARY_OP(DIV, /)
        SIMPLIFY_BINARY_OP(MUL, *)

        SIMPLIFY_UNARY_OP(SIN, sin)
        SIMPLIFY_UNARY_OP(COS, cos)
        SIMPLIFY_UNARY_OP(EXP, exp)
        SIMPLIFY_UNARY_OP(LOG, log)
        
        case tree::op_t::POW:
            tree::change_node (node, pow (Lval, Rval));
            break;

        default:
            assert(0 && "Unexpected op type");
            break;
    }

    tree::del_childs (node);

    return true;
}

#undef SIMPLIFY_BINARY_OP
#undef SIMPLIFY_UNARY_OP

// -------------------------------------------------------------------------------------------------

#define SIMPLIFY_OP(op_type, func)      \
case tree::op_t::op_type:               \
            is_smpled = func (node);    \
            break;

static bool simplify_primitive_subtree (tree::node_t *node)
{
    assert (node != nullptr);
    assert (node->type != tree::node_type_t::NOT_SET && "Incomplete node");

    if (notOP(node)) return false;

    bool is_smpled = false;

    switch (node->op)
    {
        case tree::op_t::ADD:
            is_smpled = simplify_primitive_add_sub (node) || simplify_primitive_add (node);
            break;

        case tree::op_t::SUB:
            is_smpled = simplify_primitive_add_sub (node) || simplify_primitive_sub (node);
            break;

        SIMPLIFY_OP (DIV, simplify_primitive_div)
        SIMPLIFY_OP (MUL, simplify_primitive_mul)
        SIMPLIFY_OP (SIN, simplify_primitive_sin)
        SIMPLIFY_OP (COS, simplify_primitive_cos)
        SIMPLIFY_OP (EXP, simplify_primitive_exp)
        SIMPLIFY_OP (POW, simplify_primitive_pow)
        SIMPLIFY_OP (LOG, simplify_primitive_log)

        default:
            break;
    }

    if (is_smpled)
    {
        return true;
    }

    if (node->left != nullptr) {
        is_smpled |= simplify_primitive_subtree (node->left);
    }
    is_smpled |= simplify_primitive_subtree (node->right);

    return is_smpled;
}

// -------------------------------------------------------------------------------------------------

#define CLEAN_AND_RETURN()  \
{                           \
    del_childs (node);      \
    return true;            \
}

static bool simplify_primitive_add_sub (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");
    assert (isOP(node) && "invalid node");

    if (isVAL(node->left) && iseq (Lval, 0)) {
        del_left  (node);
        move_node (node, node->right);
        return true;
    } else if (isVAL(node->right) && iseq (Rval, 0)) {
        del_right (node);
        move_node (node, node->left);
        return true;
    }

    return false;
}

static bool simplify_primitive_add (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    //sin^2 + cos ^2
    if (isOP_TYPE(node->left, MUL) && isOP_TYPE(node->right, MUL))
    {
        if (isSINX (LL) && isSINX (LR) && isCOSX (RL) && isCOSX(RR))
        {
            change_node (node, 1.0);
            CLEAN_AND_RETURN();
        }

        if (isCOSX (LL) && isCOSX (LR) && isSINX (RL) && isSINX(RR))
        {
            change_node (node, 1.0);
            CLEAN_AND_RETURN();
        }
    }

    return false;
}

static bool simplify_primitive_sub (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    // cos^x - sin^x
    if (isOP_TYPE(node->left, MUL) && isOP_TYPE(node->right, MUL))
    {
        if (isCOSX (LL) && isCOSX (LR) &&
            isSINX (RL) && isSINX (RR))
        {
            change_node (node, tree::op_t::COS);
            change_node (node->right, tree::op_t::MUL);
            change_node (RL, 2.0);
            change_node (RR, (unsigned char)'x');
            del_left (node);
            return true;
        }
    }

    return false;
}

static bool simplify_primitive_mul (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->left) && iseq(Lval, 0)) {
        change_node (node, 0.0);
        CLEAN_AND_RETURN();
    } else if (isVAL(node->right) && iseq(Rval, 0)) {
        change_node (node, 0.0);
        CLEAN_AND_RETURN();
    }

    if (isVAL(node->left) && iseq(Lval, 1)) {
        del_left  (node);
        move_node (node, node->right);
        return node;
    } else if (isVAL(node->right) && iseq(Rval, 1)) {
        del_right (node);
        move_node (node, node->left);
        return true;
    }

    return false;
}

static bool simplify_primitive_div (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->left) && iseq (Lval, 0)) {
        change_node (node, 0.0);
        CLEAN_AND_RETURN();
    }

    if (isVAL(node->right) && iseq (Rval, 1)) {
        del_right (node);
        move_node (node, node->left);
        return true;
    }

    return false;
}

static bool simplify_primitive_sin (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->right) && iseq (Aval, 0)) {
        change_node (node, 0.0);
        CLEAN_AND_RETURN();
    }

    return false;
}

static bool simplify_primitive_cos (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->right) && iseq (Aval, 0)) {
        change_node (node, 1.0);
        CLEAN_AND_RETURN();
    }

    return false;
}


static bool simplify_primitive_exp (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->right) && iseq (Aval, 0)) {
        change_node (node, 1.0);
        CLEAN_AND_RETURN();
    }

    return false;
}


static bool simplify_primitive_pow (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->right) && iseq (Aval, 1)) { //isArg
        tree::del_right (node);
        move_node (node, node->left);
        return true;
    }

    return false;
}


static bool simplify_primitive_log (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    if (isVAL(node->right) && iseq (Aval, 1)) {
        change_node (node, 0.0);
        CLEAN_AND_RETURN();
    }

    return false;
}

#undef CLEAN_AND_RETURN

// -------------------------------------------------------------------------------------------------

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

    if (hasRight) {
        node_copy->right = copy_subtree (node->right);
    }

    if (hasLeft) {
        node_copy->left  = copy_subtree (node->left);
    }

    return node_copy;
}

#undef NEW_NODE_IN_CASE

// -------------------------------------------------------------------------------------------------

static bool is_const_subtree (tree::node_t *start_node)
{
    assert (start_node != nullptr);

    tree::walk_f check_is_const = [](tree::node_t* node, void *, bool) { return !(isVAR (node)); };

    return tree::dfs_exec (start_node,  check_is_const, nullptr,
                                        nullptr,        nullptr,
                                        nullptr,        nullptr);
}

// ----------------------------------------------------------------------------

static bool iseq (double lhs, double rhs)
{
    return fabs (lhs - rhs) < DBL_ERROR;
}
