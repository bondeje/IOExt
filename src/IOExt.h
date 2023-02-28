#include <stdio.h>
#include <stdlib.h>
#include "iterators.h"

#ifndef IOEXT_H
#define IOEXT_H

#define DEFAULT_READ_MODE "rb"
#define DEFAULT_WRITE_MODE "wb"

#ifndef IO_MALLOC
#define IO_MALLOC malloc
#endif // IO_MALLOC

#ifndef IO_REALLOC
#define IO_REALLOC realloc
#endif // IO_REALLOC

#ifndef IO_FREE
#define IO_FREE free
#endif // IO_FREE

#define Select_FileLineIterator_iter(_1,_2,_3,NAME,...) NAME
#define FileLineIterator_iter(...) Select_FileLineIterator_iter(__VA_ARGS__, FileLineIterator_new, FileLineIterator_iter2, FileLineIterator_iter1, UNUSED)(__VA_ARGS__)

#define Select_LineIterator_iter(_1,_2,NAME,...) NAME
#define LineIterator_iter(...) Select_LineIterator_iter(__VA_ARGS__, LineIterator_new, LineIterator_iter1, UNUSED)(__VA_ARGS__)

// the context manager/callers owns closing the FILE handle _h
typedef struct LineIterator {
    FILE * _h;
    char * next;
    size_t next_buf_size;
    enum iterator_status stop;
} LineIterator;

// a wrapper for LineIterator that manages the file stream from an input string filename and mode
typedef struct FileLineIterator {
    LineIterator * lines;
    const char * filename;
    const char * mode;
} FileLineIterator;

LineIterator * LineIterator_new(FILE * fstr, size_t next_buf_size);
LineIterator * LineIterator_iter1(FILE * fstr);
void LineIterator_init(LineIterator * lines, FILE * fstr, size_t next_buf_size);
void LineIterator_del(LineIterator * lines);
char * LineIterator_next(LineIterator * lines);
enum iterator_status LineIterator_stop(LineIterator * lines);

FileLineIterator * FileLineIterator_new(const char * filename, const char * mode, size_t next_buf_size);
FileLineIterator * FileLineIterator_iter2(const char * filename, const char * mode);
FileLineIterator * FileLineIterator_iter1(const char * ffilename);
void FileLineIterator_init(FileLineIterator * file_iter, const char * filename, const char * mode, size_t next_buf_size);
void FileLineIterator_del(FileLineIterator * lines);
char * FileLineIterator_next(FileLineIterator * lines);
enum iterator_status FileLineIterator_stop(FileLineIterator * lines);

#endif // IOEXT_H