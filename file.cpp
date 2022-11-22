#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include "file.h"

struct text *read_text (FILE *stream)
{
    assert (stream != NULL && "pointer can't be NULL");

    ssize_t file_len_tmp = file_size (stream);
    _UNWRAP_ERR_NULL (file_len_tmp);
    size_t file_len      = (size_t) file_len_tmp;

    struct text *file    = (struct text *) calloc (1, sizeof (struct text)); _UNWRAP_NULL (file);
    file->content        = read_file (stream);                               _UNWRAP_NULL (file->content);
    file->content_size   = file_len;

    unsigned int n_lines = count_lines (file->content);
    file->lines          = (struct line *) calloc (n_lines, sizeof (struct line));
    file->n_lines        = n_lines; _UNWRAP_NULL (file->lines);

    create_index (file);

    return file;
}

char *read_file (FILE *stream)
{
    assert (stream != NULL && "pointer can't be NULL");

    ssize_t file_len_tmp = file_size (stream);
    if (file_len_tmp == ERROR) { return NULL; }

    size_t file_len      = (size_t) file_len_tmp;

    char *file_buf = (char *) calloc (file_len + 1, sizeof (char));
    _UNWRAP_NULL (file_buf);

    if (fread (file_buf, sizeof (char), file_len, stream) != file_len || ferror (stream))
    {
        return NULL;
    }

    file_buf[file_len] = '\0';

    return file_buf;
}

int write_lines (const struct text *text, FILE *stream)
{
    assert (text   != NULL && "pointer can't be NULL");
    assert (stream != NULL && "pointer can't be NULL");

    const struct line *cur_line = NULL;

    for (unsigned int i = 0; i < text->n_lines; ++i)
    {
        cur_line = text->lines + i;

        fputs (cur_line->content, stream);
        putc ('\n', stream);
    }

    return 0;
}

int write_buf (const struct text *text, FILE *stream)
{
    assert (text   != NULL && "pointer can't be NULL");
    assert (stream != NULL && "pointer can't be NULL");

    const char  *content = text->content;
    unsigned int n_lines = text->n_lines;

    for (unsigned int i = 0; i < n_lines; ++i)
    {
        fputs (content, stream);
        fputc ('\n',    stream);

        if (ferror (stream)) { return -1; }

        content = strchr (content, '\0') + 1;
    }

    return 0;
}

ssize_t file_size (FILE *stream)
{
    assert (stream != NULL && "pointer can't be NULL");

    int fd = fileno (stream);
    if (fd == -1) { return ERROR; }

    struct stat fstats = {};

    int ret = fstat (fd, &fstats);
    if (ret == -1) { return ERROR; }

    return fstats.st_size;
}

unsigned int count_lines (const char *text)
{
    assert (text != NULL && "pointer can't be NULL");

    unsigned int n_lines = 0;

    while ((text = strchr (text, '\n')) != NULL)
    {
        n_lines++;
        text++;
    }

    return n_lines;
}

void create_index (struct text *text)
{
    assert (text != NULL && "pointer can't be NULL");

    unsigned int n_line   = 0;
    char *line_start      = text->content;
    line *lines           = text->lines;
    char *cur             = line_start;

    while ((cur = strchr (line_start, '\n')) != NULL)
    {
        cur[0] = '\0';
        cur++;  // Point to first character of next line

        assert (lines != NULL && "pointer can't be NULL");

        lines[n_line].content = line_start;
        lines[n_line].len     = (size_t) (cur - line_start);

        n_line++;
        line_start = cur;
    }
}

void free_text (struct text *text)
{
    assert (text != NULL && "pointer can't be NULL");

    free (text->content);
    free (text->lines);
    free (text);
}
