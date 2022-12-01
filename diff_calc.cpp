#include <assert.h>
#include <cmath>
#include <cstddef>
#include <math.h>
#include <wchar.h>

#include "tree.h"
#include "tree_dsl.h"
#include "diff_calc.h"
#include "tree_output.h"

// ----------------------------------------------------------------------------
// CONST SECTION
// ----------------------------------------------------------------------------

///@brief Floating point calculations accuracy
const double DBL_ERROR = 1e-11;

// ----------------------------------------------------------------------------
// STATIC HEADER SECTION
// ----------------------------------------------------------------------------

static tree::node_t *diff_subtree (tree::node_t *node, char var, render::render_t *render);
static tree::node_t *diff_op      (tree::node_t *node, char var, render::render_t *render);

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

static double calc_subtree (const tree::node_t *node, double x);

static void rename_variable (tree::node_t *node, char old_var, char new_var);

static bool is_const_subtree (tree::node_t *start_node);

static bool iseq (double lhs, double rhs);

// ----------------------------------------------------------------------------
// DEFINE SECTION
// ----------------------------------------------------------------------------

#define dR diff_subtree (node->right, var, render)
#define dL diff_subtree (node->left , var, render)
#define dA dR

#define cR tree::copy_subtree (node->right)
#define cL tree::copy_subtree (node->left )
#define cS tree::copy_subtree (node)
#define cA cR

#define NEW(x) tree::new_node(x)

#define Lval node->left ->val
#define Rval node->right->val
#define Aval node->right->val

#define NodeVal(node) node->val

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

#define PUSH_FRAME(type, lhs, ...)                                                      \
{                                                                                       \
    if (render != nullptr)                                                              \
    {                                                                                   \
        render::push_frame (render, render::frame_format_t::type, lhs, ##__VA_ARGS__);  \
    }                                                                                   \
}

#define IF_RENDER(command)  \
{                           \
    if (render != nullptr)  \
    {                       \
        command;            \
    }                       \
}

// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

tree::tree_t tree::calc_diff (const tree::tree_t *src, char var, render::render_t *render, bool verbose)
{
    assert (src != nullptr);
    tree::tree_t res = {};
    tree::ctor (&res);

    res.head_node = calc_diff (src->head_node, var, render, verbose);

    return res;
}

tree::node_t *tree::calc_diff (tree::node_t *src, char var, render::render_t *render, bool verbose)
{
    assert (src != nullptr);
    tree::node_t *res = nullptr;

    IF_RENDER (render::push_subsubsection (render, "Постановка задачи"));
    IF_RENDER (render::push_diff_task_frame (render, src, var));

    if (verbose) {
        IF_RENDER (render::push_subsubsection (render, "Расчеты"));
        res = diff_subtree (src, var, render);
    } else {
        IF_RENDER (render::push_subsubsection (render, ""));
        res = diff_subtree (src, var, nullptr);
    }

    simplify (res);

    IF_RENDER (render::push_subsubsection (render, "Получение ответа"));
    IF_RENDER (render::push_diff_frame (render, src, res, var))
    
    return res;
}
// -------------------------------------------------------------------------------------------------

void tree::simplify (tree::tree_t *tree, render::render_t *render)
{
    simplify (tree->head_node, render);
}

void tree::simplify (tree::node_t *node, render::render_t *render)
{
    assert (node != nullptr && "invalid pointer");

    bool not_simplified = true;

    bool const_simplified     = true;
    bool primitive_simplified = true;

    while (not_simplified)
    {
        const_simplified     = simplify_const_subtree     (node);
        primitive_simplified = simplify_primitive_subtree (node); 

        not_simplified &= const_simplified || primitive_simplified;
        
        if (not_simplified) {
            IF_RENDER (render::push_simplify_frame (render, node));
        }
    }
}

// -------------------------------------------------------------------------------------------------

tree::tree_t tree::taylor_series (const tree::tree_t *src, int order, render::render_t *render)
{
    assert (src != nullptr);

    const int buf_size             = sizeof ("Вычисление %d производной") + 10;
    char subsection_name[buf_size] = "";

    tree::node_t *current_diff  = copy_subtree (src->head_node);
    rename_variable (current_diff, 'x', 'a');

    tree::node_t *taylor_series = current_diff;

    for (int i = 1; i <= order; ++i)
    {
        IF_RENDER (sprintf (subsection_name, "Вычисление %d производной", i));
        IF_RENDER (render::push_subsection (render, subsection_name));

        current_diff = calc_diff (current_diff, 'a', render);

        taylor_series = add (taylor_series,
                             mul (
                                 current_diff,
                                 div (
                                     pow ( sub(NEW('x'), NEW('a')), NEW((double) i) ), // (x-a)^i
                                     fact (i)                                          // i!
                                 )
                             )
                        );
    }


    tree::simplify (taylor_series);

    IF_RENDER (render::push_subsection (render, "Итоговый ответ"));
    IF_RENDER (render::push_taylor_frame (render, src->head_node, taylor_series, order));

    tree::tree_t res = {};
    tree::ctor (&res);
    res.head_node = taylor_series;
    
    return res;
}

// -------------------------------------------------------------------------------------------------

double tree::calc_tree (const tree::tree_t *tree, double x, render::render_t *render)
{
    assert(tree != nullptr && "invalid pointer");

    double ans = calc_subtree (tree->head_node, x);

    IF_RENDER (render::push_subsection (render, "Вычисление значения"));
    IF_RENDER (render::push_calculation_frame (render, tree->head_node, x, ans));

    return ans;
}

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

#define RETURN(node)        \
{                           \
    res_node = node;        \
    goto dump_and_return;   \
}

static tree::node_t *diff_subtree (tree::node_t *node, char var, render::render_t *render)
{
    assert (node != nullptr && "invalid pointer");

    tree::node_t *res_node = nullptr;

    switch (node->type)
    {
        case tree::node_type_t::VAL:
            RETURN (NEW (0.0));

        case tree::node_type_t::VAR:
            if (node->var == var) RETURN (tree::new_node(1.0))
            else                  RETURN (tree::new_node(0.0))

        case tree::node_type_t::OP:
            RETURN (diff_op (node, var, render));

        case tree::node_type_t::NOT_SET:
            assert (0 && "Invalid node type for diff");
        
        default:
            assert (0 && "Unexpected node type");
    }


    dump_and_return:
        tree::simplify (res_node, nullptr);
        IF_RENDER (render::push_diff_frame (render, node, res_node, var));
        return res_node;
}

#undef RETURN

// -------------------------------------------------------------------------------------------------

static tree::node_t *diff_op (tree::node_t *node, char var, render::render_t *render)
{
    assert (node != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::OP && "invalid node");

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
        if (simplified) {
            node->alpha_index = 0;
        }
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

    node->alpha_index = 0;
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
        node->alpha_index = 0;

        return true;
    }

    if (node->left != nullptr) {
        is_smpled |= simplify_primitive_subtree (node->left);
    }
    is_smpled |= simplify_primitive_subtree (node->right);

    if (is_smpled) {
        node->alpha_index = 0;
    }

    return is_smpled;
}

#undef SIMPLIFY_OP

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
            change_node (RR, 'x');
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

    if (isVAL(node->left) && isOP_TYPE(node->right, MUL))
    {
        if (isVAL (RL)) {
            tree::change_node (node->left, NodeVal (node->left) * NodeVal(RL));
            tree::del_left (node->right);
            tree::move_node (node->right, RR);
        } else if (isVAL (RR)) {
            tree::change_node (node->left, NodeVal (node->left) * NodeVal(RR));
            tree::del_right (node->right);
            tree::move_node (node->right, RL);
        }
    }

    if (isVAL(node->right) && isOP_TYPE(node->left, MUL))
    {
        if (isVAL (LL)) {
            tree::change_node (node->right, NodeVal (node->right) * NodeVal(LL));
            tree::del_left (node->left);
            tree::move_node (node->left, LR);
        } else if (isVAL (LR)) {
            tree::change_node (node->right, NodeVal (node->right) * NodeVal(LR));
            tree::del_right (node->left);
            tree::move_node (node->left, LL);
        }
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

static double calc_subtree (const tree::node_t *node, double x)
{
    assert(node != nullptr && "invalid pointer");

    double left  = NAN;
    double right = NAN;

    if (node->left != nullptr) {
        left = calc_subtree (node->left, x);
    }

    if (node->right != nullptr) {
        right = calc_subtree (node->right, x);
    }

    switch (node->type)
    {
        case tree::node_type_t::VAL:
            return node->val;

        case tree::node_type_t::VAR:
            if (node->var == 'x') {
                return x;
            } else {
                return NAN;
            }

        case tree::node_type_t::OP:
            assert (node->right != nullptr && "Invalid op");

            switch (node->op)
            {
                case tree::op_t::ADD: return left + right;
                case tree::op_t::SUB: return left - right;
                case tree::op_t::DIV: return left / right;
                case tree::op_t::MUL: return left * right;
                case tree::op_t::SIN: return sin (right);
                case tree::op_t::COS: return cos (right);
                case tree::op_t::EXP: return exp (right);
                case tree::op_t::POW: return pow (left, right);
                case tree::op_t::LOG: return log (right);

                default:
                    assert (0 && "Unexpected op type");
            }

        case tree::node_type_t::NOT_SET:
            assert (0 && "invalid node");

        default:
            assert (0 && "unexpected node type");
    }
}

// -------------------------------------------------------------------------------------------------

struct _rename_dfs_parameters
{
    const char old_var;
    const char new_var;
};

static void rename_variable (tree::node_t *node, const char old_var, const char new_var)
{
    assert (node != nullptr && "invalid pointer");

    _rename_dfs_parameters params_struct = {old_var, new_var};

    tree::walk_f rename_node = [](tree::node_t *node_in, void* params_void, bool)
    {
        _rename_dfs_parameters *params = (_rename_dfs_parameters *) params_void;

        if (isVAR (node_in) && node_in->var == params->old_var) {
            tree::change_node (node_in, params->new_var);
        }

        return true;
    };

    tree::dfs_exec (node, nullptr,     nullptr,
                          rename_node, &params_struct,
                          nullptr,     nullptr);
}

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
