#include <assert.h>
#include <cctype>
#include <cstring>
#include <sys/mman.h>

#include "lib/log.h"
#include "tree.h"
#include "tree_dsl.h"
#include "tree_parsing.h"

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

static tree::node_t *GetExpression     (const char **input_str);
static tree::node_t *GetAddOperand     (const char **input_str);
static tree::node_t *GetMulOperand     (const char **input_str);
static tree::node_t *GetFuncOperand    (const char **input_str);
static tree::node_t *GetFunction       (const char **input_str);
static tree::node_t *GetGeneralOperand (const char **input_str);
static tree::node_t *GetQuant          (const char **input_str);


// -------------------------------------------------------------------------------------------------
// DEFINE SECTION
// -------------------------------------------------------------------------------------------------

#define TRY(expr)               \
{                               \
    if ((expr) == nullptr)      \
    {                           \
        del_node (node);        \
        return nullptr;         \
    }                           \
}

#define EXPECT(expr)            \
{                               \
    if (!(expr))                \
    {                           \
        del_node (node);        \
        return nullptr;         \
    }                           \
}

#define SUCCESS()               \
{                               \
    *input_str = str;           \
    return node;                \
}

#define SKIP_SPACES()           \
{                               \
    while (isspace (*str))      \
    {                           \
        str++;                  \
    }                           \
}

// -------------------------------------------------------------------------------------------------

tree::node_t *tree::parse_dump (const char *str)
{
    assert (str != nullptr && "invalid pointer");
    
    tree::node_t *node = nullptr;

    TRY (node = GetExpression (&str));

    SKIP_SPACES();
    EXPECT (*str == '\0');

    assert (node != nullptr);
    return node;
}

// -------------------------------------------------------------------------------------------------

/**
 * Tree        ::= Expression  '\0'
 * Expression  ::= AddOperand  ([+-] AddOperand )*
 * AddOperand  ::= MulOperand  ([/ *] MulOperand )*
 * MulOperand  ::= FuncOperand ([^]  FuncOperand)*
 * FuncOperand ::= Function | GeneralOperand
 * 
 * Function ::= ('sin ' | 'cos ' | 'exp ' | 'ln ') GeneralOperand
 * 
 * GeneralOperand ::= Quant | '(' Expression ')'
 * Quant ::= (<ctype alpha> | <scanf double>)
 */

static tree::node_t *GetExpression (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str        = *input_str;
    tree::node_t *node     = nullptr;
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetAddOperand (&str));

    SKIP_SPACES();
    while (*str == '+' || *str == '-')
    {
        char sign = *str;
        str++;

        TRY (node_rhs = GetAddOperand (&str));

        if (sign == '+') {
            node = add (node, node_rhs);
        } else {
            node = sub (node, node_rhs);
        }

        SKIP_SPACES();
    }

    SUCCESS();
}

static tree::node_t *GetAddOperand (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str        = *input_str;
    tree::node_t *node     = nullptr;
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetMulOperand (&str));

    SKIP_SPACES();
    while (*str == '*' || *str == '/')
    {
        char sign = *str;
        str++;

        TRY (node_rhs = GetMulOperand (&str));

        if (sign == '*') {
            node = mul (node, node_rhs);
        } else {
            node = div (node, node_rhs);
        }

        SKIP_SPACES();
    }

    SUCCESS();
}

static tree::node_t *GetMulOperand (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str        = *input_str;
    tree::node_t *node     = nullptr;
    tree::node_t *node_rhs = nullptr;

    TRY (node = GetFuncOperand (&str));

    SKIP_SPACES();
    while (*str == '^')
    {
        str++;

        TRY (node_rhs = GetFuncOperand (&str));

        node = pow (node, node_rhs);

        SKIP_SPACES();
    }

    SUCCESS();
}

static tree::node_t *GetFuncOperand (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str    = *input_str;
    tree::node_t *node = GetFunction (&str);

    if (node != nullptr)
    {
        SUCCESS();
    }

    TRY (node = GetGeneralOperand (&str));

    SUCCESS();
}

//TODO разделители между функциями

#define TRY_PARSE_FUNC(name, type)              \
if (strncmp (name, str, sizeof(name) - 1) == 0) \
{                                               \
    op_type = tree::op_t::type;                 \
    str += sizeof(name)-1;                      \
} else 

static tree::node_t *GetFunction (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str    = *input_str;
    tree::node_t *node = nullptr;
    tree::op_t op_type = tree::op_t::ADD; // It is invalid op in this situation => poison

    SKIP_SPACES();

    TRY_PARSE_FUNC ("sin ", SIN)
    TRY_PARSE_FUNC ("cos ", COS)
    TRY_PARSE_FUNC ("exp ", EXP)
    TRY_PARSE_FUNC ("log ", LOG)
    {
        return nullptr;
    }

    TRY (node = GetGeneralOperand (&str));

    switch (op_type)
    {
        case tree::op_t::SIN: node = sin (node); break;
        case tree::op_t::COS: node = cos (node); break;
        case tree::op_t::EXP: node = exp (node); break;
        case tree::op_t::LOG: node = log (node); break;

        case tree::op_t::ADD:
        case tree::op_t::SUB:
        case tree::op_t::DIV:
        case tree::op_t::MUL:
        case tree::op_t::POW:
            assert (0 && "Invalid situation");

        default:
            assert (0 && "Unexpected op");
        }

    SUCCESS();
}

#undef TRY_PARSE_FUNC

static tree::node_t *GetGeneralOperand (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str    = *input_str;
    tree::node_t *node = nullptr;

    SKIP_SPACES();
    if (*str == '(')
    {
        str++;

        TRY (node = GetExpression (&str));

        SKIP_SPACES();
        EXPECT (*str == ')');
        str++;
    }
    else
    {
        TRY (node = GetQuant (&str));
    }

    SUCCESS();
}

static tree::node_t *GetQuant (const char **input_str)
{
    assert (input_str  != nullptr);
    assert (*input_str != nullptr);

    const char *str    = *input_str;
    tree::node_t *node = nullptr;

    SKIP_SPACES();
    if (isalpha (*str))
    {
        node = tree::new_node ((unsigned char) *str);
        str++;

        SUCCESS ();
    }

    double val = 0;
    int n_symb = 0;
    
    EXPECT (sscanf (str, "%lg%n", &val, &n_symb) == 1);
    
    str += n_symb;
    node = tree::new_node (val);

    SUCCESS();
}