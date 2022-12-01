#include <assert.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "common.h"
#include "file.h"
#include "lib/log.h"

#include "tree_parsing.h"
#include "tree.h"

// -------------------------------------------------------------------------------------------------
// CONST SECTION
// -------------------------------------------------------------------------------------------------

#define RECURSIVE_LOAD

const int REASON_LEN   = 50;

const char PREFIX[] = "digraph {\nnode [shape=record,style=\"filled\"]\nsplines=spline;\n";
static const size_t DUMP_FILE_PATH_LEN = 20;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

// -------------------------------------------------------------------------------------------------
// STATIC PROTOTYPES SECTION
// -------------------------------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param);

static bool node_codegen (tree::node_t *node, void *stream_void, bool cont);

#ifndef RECURSIVE_LOAD
static bool load_node    (tree::node_t *node, void *stream_void, bool);
static bool create_childs_if_needed (tree::node_t *node, void *stream_void, bool);
static bool eat_closing_bracket (tree::node_t *node, void *stream_void, bool cont);
#endif

static const char *get_op_name (tree::op_t op);
static void format_node (char *buf, const tree::node_t *node);


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

    del_node (tree->head_node);
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

bool tree::dfs_exec (node_t *node, walk_f pre_exec,  void *pre_param,
                                   walk_f in_exec,   void *in_param,
                                   walk_f post_exec, void *post_param)
{
    assert (node != nullptr && "invalid pointer");

    return dfs_recursion (node, pre_exec,  pre_param,
                                in_exec,   in_param,
                                post_exec, post_param);
}

// -------------------------------------------------------------------------------------------------

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

void tree::change_node (node_t *node, char var)
{
    assert (node != nullptr && "invalid pointer");

    node->var  = var;
    node->type = node_type_t::VAR;
}

// -------------------------------------------------------------------------------------------------

void tree::move_node (node_t *dest, node_t *src)
{
    assert (dest != nullptr && "invalid pointer");
    assert (src  != nullptr && "invalid pointer");

    memcpy (dest, src, sizeof (node_t));

    free (src);
}

// -------------------------------------------------------------------------------------------------

#define NEW_NODE_IN_CASE(type, field)               \
    case tree::node_type_t::type:                   \
        node_copy = tree::new_node (node->field);   \
        if (node_copy == nullptr) return nullptr;   \
        break;

tree::node_t *tree::copy_subtree (tree::node_t *node)
{
    assert (node != nullptr && "invalid pointer");

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
    } else {
        node_copy->right = nullptr;
    }

    if (node->left != nullptr) {
        node_copy->left = copy_subtree (node->left);
    } else {
        node_copy->left = nullptr;
    }

    return node_copy;
}

#undef NEW_NODE_IN_CASE

// -------------------------------------------------------------------------------------------------

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

#ifdef RECURSIVE_LOAD


tree::tree_err_t tree::load (tree_t *tree, FILE *dump)
{
    assert (dump != nullptr && "pointer can't be null");
    assert (tree != nullptr && "pointer can't be null");
    assert (tree->head_node == nullptr && "non empty tree");

    int fd = fileno (dump);
    size_t filesize = (size_t) file_size (dump);
    const char *file = (const char *) mmap (NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
                                                                                    fd, 0);

    if (file == MAP_FAILED)
    {
        assert (0 && "mmap failure");
        return MMAP_FAILURE;
    }

    tree->head_node = parse_dump (file);

    if (tree->head_node == nullptr) {
        return INVALID_DUMP;
    } else {
        return OK;
    }
}

#else

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

#endif

// -------------------------------------------------------------------------------------------------

int tree::graph_dump (tree_t *tree, const char *reason_fmt, ...)
{
    assert (tree       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (tree->head_node, reason_fmt, args);

    va_end (args);
    return res;
}


int tree::graph_dump (node_t *node, const char *reason_fmt, ...)
{
    assert (node       != nullptr && "pointer can't be nullptr");
    assert (reason_fmt != nullptr && "pointer can't be nullptr");

    va_list args;
    va_start (args, reason_fmt);

    int res = graph_dump (node, reason_fmt, args);

    va_end (args);
    return res;
}

int tree::graph_dump (node_t *node, const char *reason_fmt, va_list args)
{
    assert (node       != nullptr && "pointer can't be nullptr");
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

    dfs_exec (node, node_codegen, dump_file,
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

    #if HTML_LOGS
        FILE *stream = get_log_stream ();

        fprintf  (stream, "<h2>List dump: ");
        vfprintf (stream, reason_fmt, args);
        fprintf  (stream, "</h2>");

        fprintf (stream, "\n\n<img src=\"%s.png\">\n\n", filepath);
    #else
        char buf[REASON_LEN] = "";
        vsprintf (buf, reason_fmt, args);
        LOG (log::INF, "Dump path: %s.png, reason: %s", filepath, buf);
    #endif

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
    node->alpha_index  = 0; 

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

tree::node_t *tree::new_node (char var)
{
    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t), 1);
    if (node == nullptr) { return nullptr; }

    node->type = node_type_t::VAR;
    node->var  = var;    

    return node;
}

// -------------------------------------------------------------------------------------------------

void tree::del_node (node_t *start_node)
{
    if (start_node == nullptr)
    {
        return;
    }

    tree::walk_f free_node_func = [](node_t* node, void *, bool){ free(node); return true; };

    dfs_recursion (start_node, nullptr,        nullptr,
                               nullptr,        nullptr,
                               free_node_func, nullptr);
}

void tree::del_left  (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->left);
    node->left = nullptr;
}

void tree::del_right (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    node->right = nullptr;
}

void tree::del_childs (node_t *node)
{
    assert (node != nullptr && "invalid pointer");

    del_node (node->right);
    del_node (node->left);
    node->right = nullptr;
    node->left  = nullptr;
}

// -------------------------------------------------------------------------------------------------
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

    fprintf (stream, "node_%p [label = \"%s | {l: %p | r: %p} | alpha_index = %d\"]\n",
                                                        node, buf,
                                                        node->left, node->right, node->alpha_index);

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

#ifndef RECURSIVE_LOAD

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

// -------------------------------------------------------------------------------------------------

#define CASE_OP(str, op)                                        \
    case str[0]:                                                \
        while (str[++tmp_indx] != '\0'&&                        \
                    (c = getc (stream)) == str[tmp_indx]) {};   \
                                                                \
        if (str[tmp_indx] == '\0') {                            \
            tree::change_node (node, tree::op_t::op);           \
            return true;                                        \
        } else {                                                \
            return false;                                       \
        }

static bool load_node (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream       = (FILE *) stream_void;
    int c              = getc (stream);

    SKIP_SPACES();

    size_t tmp_indx = 0;

    switch (c) {
        CASE_OP("+", ADD)
        CASE_OP("-", SUB)
        CASE_OP("*", MUL)
        CASE_OP("/", DIV)
        CASE_OP("sin", SIN)
        CASE_OP("cos", COS)
        CASE_OP("exp", EXP)
        CASE_OP("^", POW)
        CASE_OP("ln", LOG)
        default:
            break; // It's not operator
    }

    if (isalpha (c))
    {
        node->type = tree::node_type_t::VAR;
        node->var  = (char) c;
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

#endif

// -------------------------------------------------------------------------------------------------

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

#ifndef RECURSIVE_LOAD

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

#endif