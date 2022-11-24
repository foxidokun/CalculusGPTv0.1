#ifndef TREE_OUTPUT_H
#define TREE_OUTPUT_H

#include "tree.h"

namespace render
{
    struct render_t
    {
        FILE *main_file;
        FILE *appendix_file;
        FILE *speech_file;
        const char *main_filename;
        const char *appendix_filename;
        const char *speech_filename;
        int frame_cnt;
        int last_alpha_indx;
    };

    int render_ctor (render_t *render, const char *main_file, const char *appendix_file,
                                                              const char *speech_filename);

    void render_dtor (render_t *render);

    void push_diff_frame        (render_t *render, tree::node_t* lhs, tree::node_t *rhs, char var = 'x');
    void push_simplify_frame    (render_t *render, tree::node_t* tree);
    void push_taylor_frame      (render_t *render, tree::node_t *orig, tree::node_t *series, int order);
    void push_calculation_frame (render_t *render, tree::node_t *orig, double answer);
    
    void push_section    (render_t *render, const char *name);
    void push_subsection (render_t *render, const char *name);
    
    void push_raw_frame  (render_t *render, const char *content, const char *speaker_text);
}

#endif