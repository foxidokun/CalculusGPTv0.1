#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <stdio.h>

namespace tree
{
    enum class node_type_t
    {
        NOT_SET,
        OP,
        VAL,
        VAR
    };

    enum class op_t
    {
        ADD,
        SUB,
        DIV,
        MUL,
        SIN,
        COS,
        EXP,
        POW,
        LOG
    };

    struct node_t
    {
        node_type_t type = node_type_t::NOT_SET;
        union
        {
            double val;
            op_t op;
            unsigned char var;
        };

        node_t *left    = nullptr;
        node_t *right   = nullptr;
    };

    struct tree_t
    {
        node_t *head_node;
    };

    enum tree_err_t
    {
        OK = 0,
        OOM,
        INVALID_DUMP,
    };

    typedef bool (*walk_f)(node_t *node, void *param, bool cont);

    void ctor (tree_t *tree);
    void dtor (tree_t *tree);

    bool dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                 walk_f in_exec,   void *in_param,
                                 walk_f post_exec, void *post_param);
    bool dfs_exec (node_t *node, walk_f pre_exec,  void *pre_param,
                                 walk_f in_exec,   void *in_param,
                                 walk_f post_exec, void *post_param);

    void change_node (node_t *node, double val);
    void change_node (node_t *node, op_t   op);
    void change_node (node_t *node, unsigned char var);

    void move_node (node_t *dest, node_t *src);

    void store (tree_t *tree, FILE *stream);
    tree::tree_err_t load (tree_t *tree, FILE *dump);

    int graph_dump (tree_t *tree, const char *reason_fmt, ...);

    tree::node_t *new_node ();
    tree::node_t *new_node (double val);
    tree::node_t *new_node (op_t   op);
    tree::node_t *new_node (unsigned char var);

    void del_node (node_t *node);

    void del_left   (node_t *node);
    void del_right  (node_t *node);
    void del_childs (node_t *node);
}

#endif //TREE_H
