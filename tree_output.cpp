#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "common.h"
#include "lib/log.h"
#include "tree.h"
#include "tree_output.h"

#include "tex_consts.h"

// -------------------------------------------------------------------------------------------------

const char TEX_FILENAME_FORMAT[] = "render/%u.tex";
const char PDF_FILENAME_FORMAT[] = "render/%u.pdf";
const int  FILENAME_LEN          = sizeof (PDF_FILENAME_FORMAT) + 4;
const int  CMD_LEN               = FILENAME_LEN + 40;

// -------------------------------------------------------------------------------------------------
// HEADER SECTION
// -------------------------------------------------------------------------------------------------

static void get_files (FILE **tex, char *pdf);
static bool dump_node_pre  (tree::node_t *node, void *stream_void, bool);
static bool dump_node_in   (tree::node_t *node, void *stream_void, bool);
static bool dump_node_post (tree::node_t *node, void *stream_void, bool);

static void dump_node_content  (FILE *stream, tree::node_t *node);
static void dump_node_operator (FILE *stream, tree::node_t *node);

static bool need_parentheses (tree::node_t *operator_node, tree::node_t *operand_node);

// -------------------------------------------------------------------------------------------------
// DEFINE SECTION
// -------------------------------------------------------------------------------------------------

#define isTYPE(node, node_type) (node->type == tree::node_type_t::node_type)

#define isOPTYPE(node, op_type) (node->op == tree::op_t::op_type)

// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

void tree::render_tex (tree_t *trees[], size_t n_trees)
{
    assert (trees != nullptr && "invalid pointer");

    FILE *tex_file = nullptr;
    char tex_filename[FILENAME_LEN] = "";

    get_files (&tex_file, tex_filename);
    if (tex_file == nullptr)
    {
        LOG (log::ERR, "Failed to open file %s", tex_filename);
    }

    fprintf (tex_file, TEX_BEGIN);

    for (size_t i = 0; i < n_trees; ++i)
    {
        fprintf (tex_file, FRAME_BEG);

        dfs_exec (trees[i], dump_node_pre,  tex_file,
                            dump_node_in,   tex_file,
                            dump_node_post, tex_file);

        fprintf (tex_file, FRAME_END);
    }

    fprintf (tex_file, TEX_END);
    fclose (tex_file);

    char cmd[CMD_LEN] = "";
    sprintf (cmd, "pdflatex -output-directory='render/' %s", tex_filename);
    system (cmd);
}

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

static void get_files (FILE **tex_file, char* tex_filename)
{
    assert (tex_file     != nullptr && "invalid pointer");
    assert (tex_filename != nullptr && "invalid pointer");

    static unsigned int cnt = 0;
    cnt++;

    sprintf (tex_filename, TEX_FILENAME_FORMAT, cnt);
    *tex_file = fopen (tex_filename, "w");
}

// -------------------------------------------------------------------------------------------------

static bool dump_node_pre (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;

    if (isTYPE(node, OP) && isOPTYPE(node, DIV))
    {
        fprintf (stream, " \\frac { ");
        return true;
    }

    if (need_parentheses (node, node->left))
    {
        fprintf (stream, " {\\left( ");
    }

    return true;
}

static bool dump_node_in (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;

    if (isTYPE(node, OP) && isOPTYPE(node, DIV))
    {
        fprintf (stream, " }{");
        return true;
    }

    if (need_parentheses (node, node->left)) {
        fprintf (stream, " \\right)} ");
    }

    dump_node_content (stream, node);

    if (need_parentheses (node, node->right)) {
        fprintf (stream, " {\\left( ");
    }

    return true;
}


static bool dump_node_post (tree::node_t *node, void *stream_void, bool)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream_void != nullptr && "invalid pointer");

    FILE *stream = (FILE *) stream_void;

    if (isTYPE(node, OP) && isOPTYPE(node, DIV))
    {
        fprintf (stream, "}");
        return true;
    }

    if (need_parentheses (node, node->right))
    {
        fprintf (stream, " \\right)} ");
    }

    return true;
}

// -------------------------------------------------------------------------------------------------

static void dump_node_content (FILE *stream, tree::node_t *node)
{
    assert (stream != nullptr && "invalid pointer");
    assert (node   != nullptr && "invalid pointer");

    switch (node->type)
    {
        case tree::node_type_t::OP:
            dump_node_operator (stream, node);
            break;

        case tree::node_type_t::VAL:
            fprintf (stream, "%lg", node->val);
            break;

        case tree::node_type_t::VAR:
            fprintf (stream, "%c", node->var);
            break;

        case tree::node_type_t::NOT_SET:
            assert (0 && "invalid node");

        default:
            assert (0 && "Unexpected type");
    }
}

// -------------------------------------------------------------------------------------------------

#define OP_FORMAT(type, format)     \
    case tree::op_t::type:          \
        fprintf (stream, format);   \
        break;

static void dump_node_operator (FILE *stream, tree::node_t *node)
{
    assert (stream != nullptr && "invalid pointer");
    assert (node   != nullptr && "invalid pointer");
    assert (node->type == tree::node_type_t::OP && "Invalid call");

    switch (node->op)
    {
        OP_FORMAT (ADD, " + ")
        OP_FORMAT (SUB, " - ")
        OP_FORMAT (MUL, " \\cdot ")
        OP_FORMAT (SIN, " \\sin ")
        OP_FORMAT (COS, " \\cos ")
        OP_FORMAT (EXP, " e^ ")
        OP_FORMAT (POW, " ^ ")
        OP_FORMAT (LOG, " \\ln ")

        case tree::op_t::DIV:
            assert (0 && "Can't dump pre order operator in in order func");

        default:
            assert (0 && "Unexpected op");
    }
}

// -------------------------------------------------------------------------------------------------

static bool need_parentheses (tree::node_t *operator_node, tree::node_t *operand_node)
{
    assert (operator_node != nullptr && "invalid pointer");

    if (operand_node == nullptr)
    {
        return false;
    }

    if (!isTYPE(operand_node, OP))
    {
        if (isTYPE (operand_node, VAL) && operand_node->val < 0)
        {
            return true;
        }

        return false;
    }

    switch (operator_node->op)
    {
        case tree::op_t::ADD:
        case tree::op_t::SUB:
            return false;

        case tree::op_t::DIV:
        case tree::op_t::MUL:
            return (isOPTYPE(operand_node, ADD) || isOPTYPE(operand_node, SUB));
        
        case tree::op_t::SIN:
        case tree::op_t::COS:
        case tree::op_t::LOG:
            return true;
        
        case tree::op_t::POW:
            if (operand_node == operator_node->right) {
                return false;
            } else {
                return true;
            }

        case tree::op_t::EXP:
            return false;

        default:
            assert (0 && "unexpeted op");
    }
}