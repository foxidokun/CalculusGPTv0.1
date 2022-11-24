#include <assert.h>

#include "common.h"
#include "tree.h"
#include "tree_output.h"

// -------------------------------------------------------------------------------------------------

#include "tex_consts.h"
const int FRAMES_OFFSET = 2;
const int MAX_CHILD_CNT = 48;

const int MAX_CMD_LEN   = 150;

// -------------------------------------------------------------------------------------------------

#define EMIT_MAIN(str, ...)   fprintf (render->main_file,     str, ##__VA_ARGS__);
#define EMIT_APDX(str, ...)   fprintf (render->appendix_file, str, ##__VA_ARGS__);
#define EMIT_SPCH(str, ...)   fprintf (render->speech_file, str, ##__VA_ARGS__);

#define isTYPE(node, node_type) (node->type == tree::node_type_t::node_type)
#define isOPTYPE(node, op_type) (node->op == tree::op_t::op_type)

#define isALPHA(node) (node->alpha_index != 0)

#define NOT_SPLITTED_DIV(node) !isALPHA(node) && isTYPE(node, OP) && isOPTYPE(node, DIV)

typedef void (*dump_f)(tree::node_t *node, FILE *stream);

// -------------------------------------------------------------------------------------------------

static int split_subtree (render::render_t *render, tree::node_t *node);

static void dump_splitted (render::render_t *render, tree::node_t *node, FILE *stream);

static void dfs_dump (tree::node_t *node, FILE *stream, dump_f pre_exec,
                                                        dump_f in_exec,   
                                                        dump_f post_exec);

static void dump_node_pre  (tree::node_t *node, FILE *stream);
static void dump_node_in   (tree::node_t *node, FILE *stream);
static void dump_node_post (tree::node_t *node, FILE *stream);

static void dump_node_content  (FILE *stream, tree::node_t *node);
static void dump_node_operator (FILE *stream, tree::node_t *node);

static bool need_parentheses (tree::node_t *operator_node, tree::node_t *operand_node);

// -------------------------------------------------------------------------------------------------
// PUBLIC SECTION
// -------------------------------------------------------------------------------------------------

int render::render_ctor (render_t *render, const char *main_filename, const char *appendix_filename,
                                                                      const char *speech_filename)
{
    assert (render            != nullptr && "invalid call");
    assert (main_filename     != nullptr && "invalid pointer");
    assert (appendix_filename != nullptr && "invalid pointer");
    assert (speech_filename   != nullptr && "invalid pointer");
    
    render->main_file         = fopen (main_filename,     "w"); if (!render->main_file) return ERROR;
    render->appendix_file     = fopen (appendix_filename, "w"); if (!render->appendix_file) return ERROR;
    render->speech_file       = fopen (speech_filename,   "w"); if (!render->speech_file) return ERROR;
    render->main_filename     = main_filename;
    render->appendix_filename = appendix_filename;
    render->speech_filename   = speech_filename;
    render->frame_cnt         = FRAMES_OFFSET;
    render->last_alpha_indx   = 0;

    EMIT_MAIN (MAIN_BEGIN);
    EMIT_APDX (APPENDIX_BEGIN);
    EMIT_SPCH (SPEECH_BEGIN);

    return 0;
}

// -------------------------------------------------------------------------------------------------

void render::render_dtor (render_t *render)
{        
    assert (render != nullptr && "invalid pointer");

    EMIT_MAIN (MAIN_END);
    EMIT_APDX (APPENDIX_END);
    
    fclose (render->main_file);
    fclose (render->appendix_file);
    fclose (render->speech_file);

    const char cmd_fmt[] = "pdflatex -output-directory='render/' %s > /dev/null &&"
                           "pdflatex -output-directory='render/' %s > /dev/null";
    char cmd[MAX_CMD_LEN] = "";

    sprintf (cmd, cmd_fmt, render->main_filename, render->appendix_filename);
    system  (cmd);

    sprintf (cmd, "./generate_video '%s'", render->speech_filename);
}

// -------------------------------------------------------------------------------------------------

void render::push_diff_frame (render_t *render, tree::node_t* lhs, tree::node_t *rhs)
{
    assert (render != nullptr && "invalid pointer");
    assert (lhs != nullptr && "invalid pointer");

    EMIT_MAIN (FRAME_BEG);
    EMIT_APDX (APDX_FRAME_BEG);

    EMIT_MAIN ("\\frac {\\partial}{\\partial x} \\left(");
    dump_splitted (render, lhs, render->main_file);
    EMIT_MAIN ("\\right) = ");
    dump_splitted (render, rhs, render->main_file);

    EMIT_MAIN (FRAME_END);
    EMIT_APDX (APDX_FRAME_END);

    EMIT_SPCH ("%s\n", PHRASES[rand() % NUM_PHRASES]);

    render->frame_cnt++;
}

void render::push_calculation_frame (render_t *render, tree::node_t* tree)
{
    assert (render != nullptr && "invalid pointer");
    assert (tree   != nullptr && "invalid pointer");

    EMIT_MAIN (FRAME_BEG);
    EMIT_APDX (APDX_FRAME_BEG);

    EMIT_MAIN (" = ");
    dump_splitted (render, tree, render->main_file);

    EMIT_MAIN (FRAME_END);
    EMIT_APDX (APDX_FRAME_END);

    EMIT_SPCH ("%s\n", PHRASES[rand() % NUM_PHRASES]);

    render->frame_cnt++;
}

void render::push_section (render_t *render, const char *name)
{
    assert (render != nullptr && "invalid pointer");
    assert (name   != nullptr && "invlaid pointer");

    EMIT_MAIN ("\\section {%s}\n", name);
    EMIT_APDX ("\\section {%s}\n", name);
}

void render::push_subsection (render_t *render, const char *name)
{
    assert (render != nullptr && "invalid pointer");
    assert (name   != nullptr && "invlaid pointer");

    EMIT_MAIN ("\\subsection {%s}\n", name);
    EMIT_APDX ("\\subsection {%s}\n", name);
}

void render::push_raw_frame (render_t *render, const char *content, const char *speaker_text)
{
    assert (render       != nullptr  && "invalid pointer");
    assert (content      != nullptr && "invalid pointer");
    assert (speaker_text != nullptr && "invalid pointer");

    EMIT_MAIN (FRAME_BLOCK_BEG);

    EMIT_MAIN ("%s", content);

    EMIT_MAIN (FRAME_BLOCK_END);
    EMIT_SPCH ("%s\n", speaker_text);
    render->frame_cnt++;
}

// -------------------------------------------------------------------------------------------------
// STATIC SECTION
// -------------------------------------------------------------------------------------------------

static int split_subtree (render::render_t *render, tree::node_t *node)
{
    assert (node   != nullptr && "invalid pointer");
    assert (render != nullptr && "invalid pointer");

    if (isALPHA(node) || node->type != tree::node_type_t::OP)
    {
        return 1;
    }

    int left_cnt  = (node->left ) ? split_subtree (render, node->left ) : 0;
    int right_cnt = (node->right) ? split_subtree (render, node->right) : 0;

    int cur_cnt   = left_cnt + right_cnt + 1;

    if (cur_cnt > MAX_CHILD_CNT)
    {
        EMIT_APDX (FORMULA_BEG);
        EMIT_APDX ("\\alpha_{%d} = ", render->last_alpha_indx + 1);

        dfs_dump (node, render->appendix_file,  dump_node_pre, 
                                                dump_node_in,  
                                                dump_node_post);

        node->alpha_index = ++render->last_alpha_indx;

        EMIT_APDX ("\n");
        EMIT_APDX (FORMULA_END);

        return 1;
    }

    return cur_cnt;
}

// -------------------------------------------------------------------------------------------------

static void dump_splitted (render::render_t *render, tree::node_t *node, FILE *stream)
{
    assert (render != nullptr && "invalid pointer");
    assert (node   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    split_subtree (render, node);

    dfs_dump (node, stream, dump_node_pre,
                            dump_node_in, 
                            dump_node_post);
}

// -------------------------------------------------------------------------------------------------

static void dfs_dump (tree::node_t *node, FILE *stream, dump_f pre_exec,
                                                        dump_f in_exec,   
                                                        dump_f post_exec)
{
    assert (node        != nullptr && "invalid pointer");
    assert (stream      != nullptr && "invalid pointer");

    if (pre_exec != nullptr)
    {
        pre_exec (node, stream);
    }

    if (!isALPHA(node) && node->left != nullptr)
    {
        dfs_dump (node->left, stream,   pre_exec,
                                        in_exec,
                                        post_exec);
    }

    if (in_exec != nullptr)
    {
        in_exec (node, stream);

    }

    if (!isALPHA(node) && node->right != nullptr)
    {
        dfs_dump (node->right, stream,  pre_exec,
                                        in_exec,
                                        post_exec);
    }

    if (post_exec != nullptr)
    {
        post_exec (node, stream);
    }
}

// -------------------------------------------------------------------------------------------------

static void dump_node_pre (tree::node_t *node, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    if (NOT_SPLITTED_DIV (node))
    {
        fprintf (stream, " \\frac { ");
        return;
    }

    fprintf (stream, "{");

    if (need_parentheses (node, node->left))
    {
        fprintf (stream, " \\left( ");
    }

    return;
}

static void dump_node_in (tree::node_t *node, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    if (NOT_SPLITTED_DIV (node))
    {
        fprintf (stream, " }{");
        return;
    }

    if (need_parentheses (node, node->left)) {
        fprintf (stream, " \\right) ");
    }

    fprintf (stream, "}");
    dump_node_content (stream, node);
    fprintf (stream, "{");

    if (need_parentheses (node, node->right)) {
        fprintf (stream, " \\left( ");
    }

    return;
}


static void dump_node_post (tree::node_t *node, FILE *stream)
{
    assert (node   != nullptr && "invalid pointer");
    assert (stream != nullptr && "invalid pointer");

    if (NOT_SPLITTED_DIV (node))
    {
        fprintf (stream, "}");
        return;
    }
    
    if (need_parentheses (node, node->right))
    {
        fprintf (stream, " \\right)");
    }

    fprintf (stream, "}");
    return;
}

// -------------------------------------------------------------------------------------------------

static void dump_node_content (FILE *stream, tree::node_t *node)
{
    assert (stream != nullptr && "invalid pointer");
    assert (node   != nullptr && "invalid pointer");

    if (isALPHA(node))
    {
        fprintf (stream, "\\alpha_{%d}", node->alpha_index);
        return;
    }

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

    if (operand_node == nullptr) {
        return false;
    }

    if (isALPHA(operator_node) || isALPHA(operand_node) != 0) {
        return false;
    }

    if (!isTYPE(operand_node, OP))
    {
        if (isTYPE (operand_node, VAL) && operand_node->val < 0) {
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

// -------------------------------------------------------------------------------------------------