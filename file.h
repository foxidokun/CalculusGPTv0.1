#ifndef FILE_H
#define FILE_H
#include <stdio.h>
#include "common.h"

/**
 * @brief      File content 
 */
struct text
{
    char *content        = NULL;  /// Content of file, but '\n' are replaced with '\0'
    struct line* lines   = NULL;  /// Array of lines, but line::content is pointer to file_lines::content
    unsigned int n_lines = 0;     /// Number of lines
    size_t content_size  = 0;     /// Size of content
};

/**
 * @brief      Store one line and it's len
 */
struct line
{
    char  *content  = NULL;   /// Line content ('\0' terminated)
    size_t     len  = 0;      /// Line length
};

/**
 * @brief              Read stream and create file struct
 *
 * @param[in]  stream  The stream to read data from
 *
 * @return             File
 */
struct text *read_text (FILE *stream);

/**
 * @brief      Read text from file
 *
 * @param      stream  Imput stream
 *
 * @return     pointer to allocated buffer or NULL if failed to read file or OOM
 */
char *read_file (FILE *stream);

/**
 * @brief      Free memory, used by given file struct
 */
void free_text (struct text *f_lines);

/**
 * @brief      Write lines into given stream
 *
 * @param      text    Text
 * @param      stream  The stream write lines to
 *
 * @return     Non-zero value on error
 */
int write_lines (const struct text *text, FILE *stream);

/**
 * @brief      Write original buffer into stream
 *
 * @param[in]  text    Text 
 * @param      stream  The stream write lines to
 *
 * @return     Non-zero value on error
 */
int write_buf (const struct text *text, FILE *stream);

/**
 * @brief      Return stream size in bytes
 *
 * @param      stream  Stream
 *
 * @return     File size in bytes or ERROR if error occured
 */
ssize_t file_size (FILE *stream);

/**
 * @brief      Counts number of lines in text
 *
 * @param[in]  text      Null-terminated string
 *
 * @return     Number of lines.
 */
unsigned int count_lines (const char *text);

/**
 * @brief      Point text::lines to line first characters in text::content and change '\n' to '\0' in text::content
 *
 * @param      text  text
 */
void create_index (struct text *text);

#endif