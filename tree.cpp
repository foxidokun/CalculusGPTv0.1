#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "common.h"
#include "lib/log.h"
#include "lib/stack/stack.h"

#include "tree.h"

// ----------------------------------------------------------------------------
// TYPE SECTION
// ----------------------------------------------------------------------------

struct load_params
{
    FILE         *stream;
    tree::tree_t *tree;
};

// ----------------------------------------------------------------------------
// CONST SECTION
// ----------------------------------------------------------------------------

const tree::node_t DEFAULT_NODE = {
    nullptr,    // value
    nullptr,    // left
    nullptr,    // right
};

const char PREFIX[] = "digraph {\nnode [shape=record,style=\"filled\"]\nsplines=spline;\n";
static const size_t DUMP_FILE_PATH_LEN = 15;
static const char DUMP_FILE_PATH_FORMAT[] = "dump/%d.grv";

// ----------------------------------------------------------------------------
// STATIC PROTOTYPES SECTION
// ----------------------------------------------------------------------------

static bool dfs_recursion (tree::node_t *node, tree::walk_f pre_exec,  void *pre_param,
                                               tree::walk_f in_exec,   void *in_param,
                                               tree::walk_f post_exec, void *post_param);

static bool node_codegen (tree::node_t *node, void *stream_void, bool cont);
static bool node_load    (tree::node_t *node, void *stream_void, bool cont);

static bool eat_closing_bracket (tree::node_t *node, void *params, bool cont);

// ----------------------------------------------------------------------------
// PUBLIC SECTION
// ----------------------------------------------------------------------------

void tree::ctor (tree_t *tree, size_t obj_size, int (*objcmp)(const void *, const void *))
{
    assert (tree   != nullptr && "invalid pointer");
    assert (objcmp != nullptr && "invalid pointer");

    tree->objcmp    = objcmp;
    tree->obj_size  = obj_size;
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

// ----------------------------------------------------------------------------

tree::tree_err_t tree::insert (tree_t *tree, const void *elem)
{
    assert (tree != nullptr && "invalid pointer");
    assert (elem != nullptr && "invalid pointer");

    node_t *allocated_node = new_node (elem, tree->obj_size);
    if (allocated_node == nullptr) { return OOM; };

    node_t *current_node = tree->head_node;

    if (current_node == nullptr)
    {
        tree->head_node = allocated_node;
        return OK;
    }

    int cmpres = 0;

    while (true)
    {
        cmpres = tree->objcmp (current_node->value, elem);

        if (cmpres > 0)
        {
            if (current_node->right != nullptr)
            {
                current_node = current_node->right;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (current_node->left != nullptr)
            {
                current_node = current_node->left;
            }
            else
            {
                break;
            }
        }
    }

    if (cmpres > 0) current_node->right = allocated_node;
    else            current_node->left  = allocated_node;

    return OK;
}

// ----------------------------------------------------------------------------

tree::tree_err_t tree::insert_left  (tree_t *tree, node_t *node, const void *elem)
{
    assert (tree != nullptr && "inVALid poointer");
    assert (node != nullptr && "inVALid poointer");
    assert (elem != nullptr && "inVALid poointer");

    assert (node->left == nullptr && "left node already exists");

    node_t *allocated_node = new_node (elem, tree->obj_size); 
    if (allocated_node == nullptr) { return OOM; }

    node->left = allocated_node;

    return OK;
}

// ----------------------------------------------------------------------------

tree::tree_err_t tree::insert_right  (tree_t *tree, node_t *node, const void *elem)
{
    assert (tree != nullptr && "inVALid poointer");
    assert (node != nullptr && "inVALid poointer");
    assert (elem != nullptr && "inVALid poointer");

    assert (node->right == nullptr && "right node already exists");

    node_t *allocated_node = new_node (elem, tree->obj_size); 
    if (allocated_node == nullptr) { return OOM; }

    node->right = allocated_node;

    return OK;
}

// ----------------------------------------------------------------------------

void tree::change_value (tree_t *tree, node_t *node, const void *elem)
{
    assert (tree != nullptr && "inVALid poointer");
    assert (node != nullptr && "inVALid poointer");
    assert (elem != nullptr && "inVALid poointer");

    memcpy (node + 1, elem, tree->obj_size);
}

// ----------------------------------------------------------------------------

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

void tree::store (tree_t *tree, FILE *stream)
{
    assert (tree   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    walk_f pre_exec = [](node_t *node, void *inner_stream, bool)
        {
            if (node->left || node->right)
            {
                fprintf ((FILE *) inner_stream, "{ '%s'\n", (char *) node->value);
            }
            else
            {
                fprintf ((FILE *) inner_stream, "{ '%s' ",   (char *) node->value);
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

// ----------------------------------------------------------------------------

tree::tree_err_t tree::load (tree_t *tree, FILE *dump)
{
    assert (dump != nullptr && "pointer can't be null");
    assert (tree != nullptr && "pointer can't be null");
    assert (tree->head_node == nullptr && "non empty tree");

    tree->head_node = new_node (DEFAULT_NODE_VALUE, OBJ_SIZE);
    if (tree->head_node == nullptr)
    {
        return OOM;
    }

    load_params params = {dump, tree};

    if (!dfs_exec (tree, node_load,             &params,
                         nullptr,               nullptr,
                         eat_closing_bracket,   &params))
    {
        log (log::ERR, "Failed to load tree");
        return INVALID_DUMP;
    }

    return OK;
}

// ----------------------------------------------------------------------------

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
        log (log::ERR, "Failed to open dump file '%s'", filepath);
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
        log (log::ERR, "Failed to execute '%s'", cmd);
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
        log (log::INF, "Dump path: %s.png", filepath);
    #endif

    va_end (args);

    fflush (get_log_stream ());
    return counter;
}

// ----------------------------------------------------------------------------

tree::node_t *tree::new_node (const void *elem, size_t obj_size)
{
    assert (elem != nullptr && "invalid pointer");
    assert (obj_size != 0 && "invalid size");

    tree::node_t *node = (tree::node_t *) calloc (sizeof (tree::node_t) + obj_size, 1);
    if (node == nullptr) { return nullptr; }

    memcpy (node, &DEFAULT_NODE, sizeof (tree::node_t));
    memcpy (node + 1, elem, obj_size);
    
    node->value   = node + 1;
    node->present = false;

    return node;
}

// ----------------------------------------------------------------------------
// PRIVATE SECTION
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

static bool node_codegen (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;

    fprintf (stream, "node_%p [label = \"%s | {l: %p | r: %p}\", fillcolor=\"%s\"]\n", node, (char *) node->value,
                                                                    node->left, node->right, node->present ? "green" : "cyan");

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

// ----------------------------------------------------------------------------

#define SKIP_SPACES()       \
{                           \
    while (isspace (c))     \
    {                       \
        c = getc (stream);  \
    }                       \
}

static bool node_load (tree::node_t *node, void *params, bool cont)
{
    assert (node   != nullptr && "invalid pointer");
    assert (params != nullptr && "invalid pointer");

    if (!cont)
    {
        return false;
    }

    FILE *stream       = ((load_params *) params)->stream;
    tree::tree_t *tree = ((load_params *) params)->tree;

    int c = getc (stream);
    char buf[OBJ_SIZE + 1] = ""; 

    SKIP_SPACES ();

    if (c == '{')
    {
        c = getc (stream);
        SKIP_SPACES ();

        if (c != '\'')
        {
            log (log::ERR, "Invalid node syntax");
            return false;
        }

        int i = 0;
        for (; i < OBJ_SIZE; ++i)
        {
            c = getc (stream);

            if (c == '\'')
            {
                break;
            }

            buf[i] = (char) c;
        }

        buf[i] = '\0';
    }

    tree::change_value (tree, node, buf);
    node->present = true;

    while (c != '{' && c != '}')
    {
        c = getc (stream);
    }

    if (c == '}')
    {
        node->left  = nullptr;
        node->right = nullptr;
    }
    else
    {
        node->left  = tree::new_node (DEFAULT_NODE_VALUE, OBJ_SIZE);
        node->right = tree::new_node (DEFAULT_NODE_VALUE, OBJ_SIZE);
    }

    ungetc (c, stream);

    SKIP_SPACES ();

    return true;
}

// ----------------------------------------------------------------------------

static bool eat_closing_bracket (tree::node_t *node, void *params, bool cont)
{
    assert (node   != nullptr && "invalid pointer");
    assert (params != nullptr && "invalid pointer");

    if (!cont)
    {
        return false;
    }

    FILE *stream       = ((load_params *) params)->stream;

    int c = getc (stream);

    SKIP_SPACES();

    if (c != '}')
    {
        log (log::ERR, "Invalid dump: no closing bracket");
        return false;
    }

    return true;
}
