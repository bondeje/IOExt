#include <stdio.h>
#include <string.h>
#include "IOExt.h"

#if defined(Macintosh)
    #define LINE_ENDING "\r"
    #define HAS_LINE_FEED 0
#elif defined(_WIN32)
    #define LINE_ENDING "\r\n"
    #define HAS_LINE_FEED 1
#else // use for basically all other cases for now
    #define LINE_ENDING "\n"
    #define HAS_LINE_FEED 1
#endif // LINE_ENDING definitions

FileLineIterator * FileLineIterator_new(const char * filename, const char * mode, size_t next_buf_size) {
    FileLineIterator * file_iter = (FileLineIterator *) IO_MALLOC(sizeof(FileLineIterator));
    if (!file_iter) {
        return NULL;
    }

    file_iter->lines = LineIterator_new(fopen(filename, mode), next_buf_size);
    if (!file_iter->lines) {
        IO_FREE(file_iter);
        return NULL;
    }

    // since we leave opening the file to FileLineIterator_init, it can fail. To check, for failures, 
    // we look at file_iter->lines->_h for a non-NULL handle
    if (!file_iter->lines->_h) {
        FileLineIterator_del(file_iter); // can just call FileLineIterator_del since all necessary components have been allocated
        return NULL;
    }

    FileLineIterator_init(file_iter, filename, mode, next_buf_size);

    return file_iter;
}

FileLineIterator * FileLineIterator_iter2(const char * filename, const char * mode) {
    return FileLineIterator_new(filename, mode, LINE_BUFFER_SIZE);
}

FileLineIterator * FileLineIterator_iter1(const char * filename) {
    return FileLineIterator_new(filename, DEFAULT_READ_MODE, LINE_BUFFER_SIZE);
}

void FileLineIterator_init(FileLineIterator * file_iter, const char * filename, const char * mode, size_t next_buf_size) {
    file_iter->filename = filename;
    file_iter->mode = mode;

    // LineIterator_init is done implicitly in FileLineIterator_new
    // LineIterator_init(file_iter->lines, fopen(filename, mode), LINE_BUFFER_SIZE);
}

void FileLineIterator_del(FileLineIterator * file_iter) {
    fclose(file_iter->lines->_h); // FileLineIterator owns the FILE handle
    LineIterator_del(file_iter->lines);
    file_iter->lines = NULL;
    IO_FREE(file_iter);
}

char * FileLineIterator_next(FileLineIterator * file_iter) {
    return LineIterator_next(file_iter->lines);
};

enum iterator_status FileLineIterator_stop(FileLineIterator * file_iter) {
    // used file_iter->lines->stop, but after update to have file_iter->stop track this value, should be equivalent
    if (file_iter->lines->stop == ITERATOR_STOP) {
        FileLineIterator_del(file_iter);
        return ITERATOR_STOP;
    }
    return file_iter->lines->stop;
}

LineIterator * LineIterator_new(FILE * fstr, size_t next_buf_size) {
    if (!fstr) {
        return NULL;
    }
    LineIterator * lines = (LineIterator *) IO_MALLOC(sizeof(LineIterator));
    if (!lines) {
        return NULL;
    }

    if (!next_buf_size) {
        next_buf_size = LINE_BUFFER_SIZE;
    }

    lines->next = (char *) IO_MALLOC(sizeof(char) * next_buf_size);
    if (!lines->next) {
        IO_FREE(lines);
        return NULL;
    }

    LineIterator_init(lines, fstr, next_buf_size);

    return lines;
}

LineIterator * LineIterator_iter1(FILE * fstr) {
    return LineIterator_new(fstr, LINE_BUFFER_SIZE);
}

void LineIterator_init(LineIterator * lines, FILE * fstr, size_t next_buf_size) {
    lines->_h = fstr;
    lines->stop = ITERATOR_GO;
    for (size_t i = 0; i < next_buf_size; i++) {
        lines->next[i] = '\0';
    }
    lines->next_buf_size = next_buf_size;
}

void LineIterator_del(LineIterator * lines) {
    IO_FREE(lines->next);
    lines->next = NULL;
    IO_FREE(lines);
}

char * LineIterator_next(LineIterator * lines) {
    // this is the non-posix version. For posix, use getline() in stdio.h to update LineIterator
    char * test = fgets(lines->next, lines->next_buf_size, lines->_h);
    if (!test) { // fgets failed or EOF is encountered immediately
        if (feof(lines->_h)) {
            lines->stop = ITERATOR_STOP;
        }
        return NULL;
    }
    size_t nchar = strlen(lines->next);
    if (!feof(lines->_h) && test[nchar-1] != '\n') { // buffer was not large enough
        long long int offset;
        // find the minimum size required for the new buffer
        size_t new_buf_size = nchar;
        while (!feof(lines->_h) && (fgetc(lines->_h) != '\n')) {
            new_buf_size++;
        }
        if (feof(lines->_h)) {
            new_buf_size += 1;
            offset = -((long long int)new_buf_size - 2); // to accommodate the '\n\0' line terminator
        } else {
            new_buf_size += 2;
            offset = -((long long int)new_buf_size - 1);
        }

        // allocate a new buffier
        char * new_buf = (char *) IO_REALLOC(lines->next, sizeof(char) * new_buf_size);
        if (!new_buf) {
            return NULL; // TODO: CONSIDER: how to handle failures to realloc while failing to capture full line...maybe just proceed as normal?
        }

        // update LineIterator
        lines->next = new_buf;
        lines->next_buf_size = new_buf_size;

        // initialize new elements in new buffer
        for (size_t i = lines->next_buf_size; i < new_buf_size; i++) {
            lines->next[i] = '\0';
        }

        // reset file stream back to beginning of current line
        fseek(lines->_h, offset, SEEK_CUR); // need to validate that this resets

        return LineIterator_next(lines); // re-try
    }

    // if _POSIX
    // getline(&lines->next, &lines->next_buf_size);
    lines->stop = (!feof(lines->_h)) ? ITERATOR_GO : ITERATOR_STOP; 

    // either end of file was encountered or last character is a linefeed. In this case test is lines->next
    return lines->next;   
    
    // additionally need to handle the case of classic MAC? there's no line feed and so fgets fails, but not sure if it has getline()
}

// destroys the LineIterator if stops
enum iterator_status LineIterator_stop(LineIterator * lines) {
    if (lines->stop == ITERATOR_STOP) {
        LineIterator_del(lines);
        return ITERATOR_STOP;
    }
    return lines->stop;
}