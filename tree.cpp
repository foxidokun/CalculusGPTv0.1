#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "common.h"
#include "lib/log.h"

#include "tree.h"

// -------------------------------------------------------------------------------------------------
// CONST SECTION
// -------------------------------------------------------------------------------------------------

const int MAX_NODE_LEN = 12;
const int REASON_LEN   = 50;

const char PREFIX[] = "digraph {\nnode [shape=record,style=\"filled\"]\nsplines=spline;\n";
static const size_t DUMP_FILE_PATH_LEN = 15;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

// -------------------------------------------------------------------------------------------------
// STATIC PROTOTYPES SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param);

static bool node_codegen (tree::node_t *node, void *stream_void, bool cont);
static bool load_node    (tree::node_t *node, void *stream_void, bool);
static bool create_childs_if_needed (tree::node_t *node, void *stream_void, bool);

static const char *get_op_name (tree::op_t op);
static void format_node (char *buf, const tree::node_t *node);

static bool eat_closing_bracket (tree::node_t *node, void *stream_void, bool cont);

// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

void tree::ctor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    tree->head_node = nullptr;
}

void tree::dtor (tree_t *tree)
{
    assert (tree != nullptr && "invalid pointer");

    tree::walk_f free_node_func = [](node_t* node, void *, bool){ free(node); return true; };

    dfs_exec (tree, nullptr,        nullptr,
                    nullptr,        nullptr,
                    free_node_func, nullptr);
}

// -------------------------------------------------------------------------------------------------

bool tree::dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (tree != nullptr && "invalid pointer");
    assert (tree->head_node != nullptr && "invalid tree");

    return dfs_recursion (tree->head_node, pre_exec,  pre_param,
                                           in_exec,   in_param,
                                           post_exec, post_param);
}

// ----------------------------------------------------------------------------

void tree::change_node (node_t *node, double val)
{
    assert (node != nullptr && "invalid pointer");

    node->val  = val;
    node->type = node_type_t::VAL;
}

void tree::change_node (node_t *node, op_t op)
{
    assert (node != nullptr && "invalid pointer");

    node->op   = op;
    node->type = node_type_t::OP;
}

void tree::change_node (node_t *node, unsigned char var)
{
    assert (node != nullptr && "invalid pointer");

    node->var  = var;
    node->type = node_type_t::VAR;
}

// ----------------------------------------------------------------------------

void tree::store (tree_t *tree, FILE *stream)
{
    assert (tree   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    walk_f pre_exec = [](node_t *node, void *inner_stream, bool)
        {
            char buf[MAX_NODE_LEN] = "";
            format_node (buf, node);

            if (node->left || node->right)
            {
                fprintf ((FILE *) inner_stream, "{ '%s'\n", buf);
            }
            else
            {
                fprintf ((FILE *) inner_stream, "{ '%s' ",  buf);
            }

            return true;
        };
    
    walk_f post_exec = [](node_t *, void *inner_stream, bool)
        {   
            fprintf ((FILE *) inner_stream, "}\n");

            return true;
        };


    dfs_exec (tree, pre_exec,   stream,
                    nullptr,   nullptr,
                    post_exec,  stream);
}

// -------------------------------------------------------------------------------------------------

tree::tree_err_t tree::load (tree_t *tree, FILE *dump)
{
    assert (dump != nullptr && "pointer can't be null");
    assert (tree != nullptr && "pointer can't be null");
    assert (tree->head_node == nullptr && "non empty tree");

    tree->head_node = new_node ();
    if (tree->head_node == nullptr)
    {
        return OOM;
    }

    if (!dfs_exec (tree, create_childs_if_needed, dump,
                         load_node,               dump,
                         eat_closing_bracket,     dump))
    {
        LOG (log::ERR, "Failed to load tree");
        return INVALID_DUMP;
    }

    return OK;
}

// -------------------------------------------------------------------------------------------------

int tree::graph_dump (tree_t *tree, const char *reason_fmt, ...)
{
    assert (tree       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    static int counter = 0;
    counter++;

    char filepath[DUMP_FILE_PATH_LEN+1] = "";    
    sprintf (filepath, DUMP_FILE_PATH_FORMAT, counter);

    FILE *dump_file = fopen (filepath, "w");
    if (dump_file == nullptr)
    {
        LOG (log::ERR, "Failed to open dump file '%s'", filepath);
        return counter;
    }

    fprintf (dump_file, PREFIX);

    dfs_exec (tree, node_codegen, dump_file,
                    nullptr, nullptr,
                    nullptr, nullptr);

    fprintf (dump_file, "}\n");

    fclose (dump_file);

    char cmd[2*DUMP_FILE_PATH_LEN+20+1] = "";
    sprintf (cmd, "dot -T png -o %s.png %s", filepath, filepath);
    if (system (cmd) != 0)
    {
        LOG (log::ERR, "Failed to execute '%s'", cmd);
    }

    va_list args;
    va_start (args, reason_fmt);

    FILE *stream = get_log_stream ();

    #if HTML_LOGS
        fprintf  (stream, "<h2>List dump: ");
        vfprintf (stream, reason_fmt, args);
        fprintf  (stream, "</h2>");

        fprintf (stream, "\n\n<img src=\"%s.png\">\n\n", filepath);
    #else
        char buf[REASON_LEN] = "";
        vsprintf (buf, reason_fmt, args);
        LOG (log::INF, "Dump path: %s.png, reason: %s", filepath, buf);
    #endif

    va_end (args);

    fflush (get_log_stream ());
    return counter;
}

// -------------------------------------------------------------------------------------------------

tree::node_t *tree::new_node ()
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    const tree::node_t default_node = {};
    memcpy (node, &default_node, sizeof (tree::node_t));

    node->type    = node_type_t::NOT_SET;    

    return node;
}

tree::node_t *tree::new_node (double val)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = node_type_t::VAL;
    node->val  = val;    

    return node;
}

tree::node_t *tree::new_node (op_t op)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = node_type_t::OP;
    node->op   = op;    

    return node;
}

tree::node_t *tree::new_node (unsigned char var)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = node_type_t::VAR;
    node->var  = var;    

    return node;
}

// ----------------------------------------------------------------------------
// PRIVATE SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    bool cont = true; 

    if (pre_exec != nullptr)
    {
        cont = pre_exec (node, pre_param, cont) && cont;
    }

    if (cont && node->left != nullptr)
    {
        cont = cont && dfs_recursion (node->left, pre_exec,  pre_param,
                                   in_exec,   in_param,
                                   post_exec, post_param);
    }

    if (in_exec != nullptr)
    {
        cont = in_exec (node, in_param, cont) && cont;

    }

    if (cont && node->right != nullptr)
    {
        cont = cont && dfs_recursion (node->right, pre_exec,  pre_param,
                                    in_exec,   in_param,
                                    post_exec, post_param);
    }

    if (post_exec != nullptr)
    {
        cont = post_exec (node, post_param, cont) && cont;
    }

    return cont;
}

// -------------------------------------------------------------------------------------------------

static bool node_codegen (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;
    char buf[MAX_NODE_LEN] = "";
    format_node (buf, node);

    fprintf (stream, "node_%p [label = \"%s | {l: %p | r: %p}\"]\n",
                                                        node, buf,
                                                        node->left, node->right);

    if (node->left != nullptr)
    {
        fprintf (stream, "node_%p -> node_%p\n", node, node->left);
    }

    if (node->right != nullptr)
    {
        fprintf (stream, "node_%p -> node_%p\n", node, node->right);
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

#define SKIP_SPACES()       \
{                           \
    while (isspace (c))     \
    {                       \
        c = getc (stream);  \
    }                       \
}

#define CASE_OP(symb)           \
    case symb:                  \
        need_right = true;      \
        break;

static bool create_childs_if_needed (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream  = (FILE *) stream_void;
    int c         = getc (stream);
    SKIP_SPACES ();

    if (c != '(')
    {
        LOG (log::ERR, "Expected ( node opening, got '%c' (%d)", c, c);
        return false;
    }

    c = getc (stream);
    SKIP_SPACES ();
    if (c == '(')
    {
        node->left  = tree::new_node();
        node->right = tree::new_node();
    }
    else
    {
        bool need_right = true;

        switch (c) {
            CASE_OP('s')
            CASE_OP('c')
            CASE_OP('e')
            CASE_OP('p')
            CASE_OP('l')
            
            default:
                need_right = false;
                break;
        }

        node->left  = nullptr;

        if (need_right) node->right = tree::new_node();
        else            node->right = nullptr;
    }

    ungetc (c, stream);

    return true;
}

#undef CASE_OP

#define CASE_OP(symb, op)                           \
    case symb:                                      \
        tree::change_node (node, tree::op_t::op);   \
        return true;

static bool load_node (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream       = (FILE *) stream_void;
    int c              = getc (stream);
    char buf[MAX_NODE_LEN + 1] = ""; 

    SKIP_SPACES();

    switch (c) {
        CASE_OP('+', ADD)
        CASE_OP('-', SUB)
        CASE_OP('*', MUL)
        CASE_OP('/', DIV)
        CASE_OP('s', SIN)
        CASE_OP('c', COS)
        CASE_OP('e', EXP)
        CASE_OP('p', POW)
        CASE_OP('l', LOG)
    }

    if (isalpha (c))
    {
        node->type = tree::node_type_t::VAR;
        node->var  = c;
        return true;
    }

    ungetc (c, stream);
    if (fscanf (stream, "%lg", &node->val) != 1)
    {
        return false;
    }
    else
    {
        node->type = tree::node_type_t::VAL;
        return true;
    }

    return true;
}

#undef CASE_OP

// -------------------------------------------------------------------------------------------------

#define FORMAT(type, fmt)

static void format_node (char *buf, const tree::node_t *node) 
{
    assert (buf  != nullptr && "invalid pointer");
    assert (node != nullptr && "invalid pointer");

    switch (node->type)
    {
        case tree::node_type_t::OP:
            sprintf (buf, "%s", get_op_name(node->op));
            break;

        case tree::node_type_t::VAL:
            sprintf (buf, "%f", node->val);
            break;

        case tree::node_type_t::VAR:
            sprintf (buf, "%c", node->var);
            break;

        case tree::node_type_t::NOT_SET:
            sprintf (buf, "not set");
            break;

        default:
            LOG (log::ERR, "Ivalid node type %d lol");
            assert (0 && "unexpected node type");
    }
}

// -------------------------------------------------------------------------------------------------

#define _OP(op, name)      \
    case tree::op_t::op:   \
        return name;       \

static const char *get_op_name (tree::op_t op)
{
    switch (op) {
        _OP(ADD, "+")
        _OP(SUB, "-")
        _OP(DIV, "/")
        _OP(MUL, "*")
        _OP(SIN, "sin")
        _OP(COS, "cos")
        _OP(EXP, "exp")
        _OP(POW, "pow")
        _OP(LOG, "log")

        default:
            assert (0 && "Invalid op, possible union error");
    }
}

// -------------------------------------------------------------------------------------------------

static bool eat_closing_bracket (tree::node_t *node, void *stream_void, bool cont)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    if (!cont)
    {
        return false;
    }

    FILE *stream       = (FILE *) stream_void;

    int c = getc (stream);

    SKIP_SPACES();

    if (c != ')')
    {
        LOG (log::ERR, "Invalid dump: no closing bracket");
        return false;
    }

    return true;
}
