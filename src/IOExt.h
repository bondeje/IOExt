#include <stdio.h>
#include <stdlib.h>
#include "iterators.h"

#ifndef IOEXT_H
#define IOEXT_H

#ifndef IO_MALLOC
#define IO_MALLOC malloc
#endif // IO_MALLOC

#ifndef IO_REALLOC
#define IO_REALLOC realloc
#endif // IO_REALLOC

#ifndef IO_FREE
#define IO_FREE free
#endif // IO_FREE

#define Select_FileLineIterator_constructor(_1,_2,_3,NAME,...) NAME
#define FileLineIterator_constructor(...) Select_FileLineIterator_constructor(__VA_ARGS__, FileLineIterator_new, FileLineIterator_iter_mode, FileLineIterator_iter, UNUSED)(__VA_ARGS__)

#define Select_LineIterator_constructor(_1,_2,NAME,...) NAME
#define LineIterator_constructor(...) Select_LineIterator_constructor(__VA_ARGS__, LineIterator_new, LineIterator_iter, UNUSED)(__VA_ARGS__)

typedef struct LineIterator LineIterator;
// a wrapper for LineIterator that manages the file stream from an input string filename and mode
typedef struct FileLineIterator FileLineIterator;

LineIterator * LineIterator_new(FILE * fstr, size_t next_buf_size);
LineIterator * LineIterator_iter(FILE * fstr);
void LineIterator_init(LineIterator * lines, FILE * fstr, size_t next_buf_size);
void LineIterator_del(LineIterator * lines);
char * LineIterator_next(LineIterator * lines);
extern char * (*LineIterator_start) (LineIterator *);
enum iterator_status LineIterator_stop(LineIterator * lines);

FileLineIterator * FileLineIterator_new(const char * filename, const char * mode, size_t next_buf_size);
FileLineIterator * FileLineIterator_iter_mode(const char * filename, const char * mode);
FileLineIterator * FileLineIterator_iter(const char * ffilename);
void FileLineIterator_init(FileLineIterator * file_iter, const char * filename, const char * mode, size_t next_buf_size);
void FileLineIterator_del(FileLineIterator * lines);
char * FileLineIterator_next(FileLineIterator * lines);
extern char * (*FileLineIterator_start) (FileLineIterator *);
enum iterator_status FileLineIterator_stop(FileLineIterator * lines);

#endif // IOEXT_H