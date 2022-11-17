#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include <stdio.h>

namespace tree
{
    enum class node_type_t
    {
        OP,
        VAL,
        VAR
    };

    enum class op_type_t
    {
        ADD,
        SUB,
        DIV,
        MUL,
        SIN,
    };

    union node_data_t
    {
            double val;
            op_type_t op;
            unsigned char var;
    };

    struct node_t
    {
        node_type_t type;
        node_data_t data;        

        node_t *left;
        node_t *right;
        bool present;
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

    tree_err_t insert_left  (tree_t *tree, node_t *node, op_type_t type, node_data_t data);
    tree_err_t insert_right (tree_t *tree, node_t *node, op_type_t type, node_data_t data);

    bool dfs_exec (tree_t *tree, walk_f pre_exec,  void *pre_param,
                                 walk_f in_exec,   void *in_param,
                                 walk_f post_exec, void *post_param);


    void store (tree_t *tree, FILE *stream);
    tree::tree_err_t load (tree_t *tree, FILE *dump);

    int graph_dump (tree_t *tree, const char *reason_fmt, ...);

    tree::node_t *new_node (op_type_t type, node_data_t data);
}

#endif //TREE_H
