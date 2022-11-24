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

    enum class frame_format_t
    {
        DIFFIRENTIAL,
        SIMPLIFICATION,
    };

    struct frame_t
    {
        frame_format_t format   = frame_format_t::DIFFIRENTIAL;
        tree::node_t *lhs       = nullptr; // always exists
        tree::node_t *rhs       = nullptr; // nullptr if SIMPLIFICATION
    };

    int render_ctor (render_t *render, const char *main_file, const char *appendix_file,
                                                              const char *speech_filename);
    void render_dtor (render_t *render);

    void push_frame (render_t *render, frame_format_t format, tree::node_t* lhs,
                                                              tree::node_t *rhs = nullptr);
}

#endif