#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cl_iterators.h"

#ifndef IOEXT_H
#define IOEXT_H

// CONSIDER making a String_join, which takes an iterable object of strings and creates 1 string with delimiter

// Platform depending line EOL character sequences
#if defined(Macintosh)
    #define LINE_ENDING "\r"
    #define HAS_LINE_FEED 0
#elif defined(_WIN32)
    #define LINE_ENDING "\r\n"
    #define HAS_LINE_FEED 1
#else // use for basically all other cases for now
    #define _posix_ 1
    #define LINE_ENDING "\n"
    #define HAS_LINE_FEED 1
#endif // LINE_ENDING definitions

#define DEFAULT_READ_MODE "rb"
#define DEFAULT_WRITE_MODE "wb"

#ifndef LINE_BUFFER_SIZE
#define LINE_BUFFER_SIZE BUFSIZ
#endif // LINE_BUFFER_SIZE

#ifndef TOKEN_BUFFER_SIZE
#define TOKEN_BUFFER_SIZE 32
#endif // TOKEN_BUFFER_SIZE

#ifndef IO_MALLOC
#define IO_MALLOC malloc
#endif // IO_MALLOC

#ifndef IO_REALLOC
#define IO_REALLOC realloc
#endif // IO_REALLOC

#ifndef IO_FREE
#define IO_FREE free
#endif // IO_FREE

/*
#define Select_TokenIterator_iter(_1,_2,_3,NAME,...) NAME
#define TokenIterator_iter(...) Select_TokenIterator_iter(__VA_ARGS__, TokenIterator_new, TokenIterator_iter2, TokenIterator_iter1, UNUSED)(__VA_ARGS__)

#define Select_FileLineIterator_iter(_1,_2,_3,NAME,...) NAME
#define FileLineIterator_iter(...) Select_FileLineIterator_iter(__VA_ARGS__, FileLineIterator_new, FileLineIterator_iter2, FileLineIterator_iter1, UNUSED)(__VA_ARGS__)

#define Select_LineIterator_iter(_1,_2,NAME,...) NAME
#define LineIterator_iter(...) Select_LineIterator_iter(__VA_ARGS__, LineIterator_new, LineIterator_iter1, UNUSED)(__VA_ARGS__)
*/
extern char * WHITESPACE;

// the context manager/callers owns closing the FILE handle _h
typedef struct LineIterator {
    FILE * handle;                  // NOT owned by the LineIterator
    char * next;                // owned by LineIterator
    size_t buffer_size;
    enum iterator_status stop;
    bool buffer_reclaim;
} LineIterator;

// a wrapper for LineIterator that manages the file stream from an input string filename and mode
typedef struct FileLineIterator {
    //LineIterator * lines;       // owned by FileLineIterator
    LineIterator lines;
    const char * filename;      // NOT owned by FileLineIterator
    const char * mode;          // NOT owned by FileLineIterator
} FileLineIterator;

typedef struct TokenIterator {
    char * string;              // NOT owned by the TokenIterator
    char * delimiters;          // NOT owned by the TokenIterator
    char * next;                // owned by TokenIterator
    size_t buffer_size;
    long long int loc;          // location index of last delimiter, -1 means not yet found
    enum iterator_status stop;
    bool group;                // internal, do not set
    bool buffer_reclaim;
} TokenIterator;

bool String_ends_with(char * string, char * ending);

bool String_starts_with(const char * string, const char * prefix);

// removes whitespace from the right
char * String_rstrip(char * string);

// removes whitespace from the left
char * String_lstrip(char * string);

// removes whitespace from both sides
char * String_strip(char * string);


LineIterator * LineIterator_new(FILE * handle, size_t buffer_size);
//LineIterator * LineIterator_iter1(FILE * fstr);
void LineIterator_init(LineIterator * lines, FILE * handle, char * buffer, size_t buffer_size);
void LineIterator_del(LineIterator * lines);
char * LineIterator_next(LineIterator * lines);
enum iterator_status LineIterator_stop(LineIterator * lines);

FileLineIterator * FileLineIterator_new(const char * filename, const char * mode, size_t buffer_size);
//FileLineIterator * FileLineIterator_iter2(const char * filename, const char * mode);
//FileLineIterator * FileLineIterator_iter1(const char * filename);
void FileLineIterator_init(FileLineIterator * file_iter, const char * filename, const char * mode, char * buffer, size_t buffer_size);
void FileLineIterator_del(FileLineIterator * file_iter);
char * FileLineIterator_next(FileLineIterator * file_iter);
enum iterator_status FileLineIterator_stop(FileLineIterator * file_iter);

TokenIterator * TokenIterator_new(char * string, char * delimiters, size_t buffer_size);
//TokenIterator * TokenIterator_iter2(char * string, char * delimiters);
//TokenIterator * TokenIterator_iter1(char * string);
void TokenIterator_init(TokenIterator * tokens, char * string, char * delimiters, char * buffer, size_t buffer_size);
void TokenIterator_del(TokenIterator * tokens);
char * TokenIterator_next(TokenIterator * tokens);
enum iterator_status TokenIterator_stop(TokenIterator * tokens);

#endif // IOEXT_H