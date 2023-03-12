#include <stdio.h>
#include <string.h>
#include "io_ext.h"

// TODO: need to refactor so that TokenIterator->loc points to the start of the next token or 
// after the last delimiter. Using the location of the last delimiter after a token creates 
// very messy logic

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

char * WHITESPACE = " \t\r\n\v\f";

// can speed up analysis by actually implementing presence in a set
static bool is_in_delimiter_set(char cut, char * delimiters) {
    for (size_t i = 0; delimiters[i] != '\0'; i++) {
        if (delimiters[i] == cut) {
            return true;
        }
    }
    return false;
}

static bool str_ends_with(char * string, size_t length, char * ending, size_t ending_length) {
    if (length < ending_length) {
        return false;
    }
    for (size_t i = 0; i < ending_length; i++) {
        if (string[length-1-i] != ending[ending_length-1-i]) {
            return false;
        }
    }
    
    return true;
}

bool String_ends_with(char * string, char * ending) {
    return str_ends_with(string, strlen(string), ending, strlen(ending));
}

bool String_starts_with(const char * string, const char * prefix) {
    return strncmp(string, prefix, strlen(prefix)) == 0;
}

// removes whitespace 
char *  String_rstrip(char * string) {
    size_t length = strlen(string);
    while (length && is_in_delimiter_set(string[length-1], WHITESPACE)) {
        length--;
    }
    string[length] = '\0';
    return string;
}

char * String_lstrip(char * string) {
    size_t length = strlen(string);
    size_t nwhite = 0;
    // is_in_delimiter_set will fail when string[nwhite], no need to check that we've consumed the string
    while (is_in_delimiter_set(string[nwhite], WHITESPACE)) {
        nwhite++;
    }
    if (nwhite) {
        memmove(string, string+nwhite, length-nwhite);
        string[length-nwhite] = '\0';
    }
    return string;
}

char *  String_strip(char * string) {
    return String_rstrip(String_lstrip(string));
}

// fully qualified constructor for FileLineIterator object
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

// simple constructor of a FileLineIterator from a filename and read mode, uses default buffer size (stdio.BUF_SIZE)
FileLineIterator * FileLineIterator_iter2(const char * filename, const char * mode) {
    return FileLineIterator_new(filename, mode, LINE_BUFFER_SIZE);
}

// simple constructor of a FileLineIterator from only a filename. uses default read mode ("rb") and buffer size (stdio.BUF_SIZE)
FileLineIterator * FileLineIterator_iter1(const char * filename) {
    return FileLineIterator_new(filename, DEFAULT_READ_MODE, LINE_BUFFER_SIZE);
}

// initializes all the internal variables of the FileLineIterator object
void FileLineIterator_init(FileLineIterator * file_iter, const char * filename, const char * mode, size_t next_buf_size) {
    file_iter->filename = filename;
    file_iter->mode = mode;

    // LineIterator_init is done implicitly in FileLineIterator_new
    // LineIterator_init(file_iter->lines, fopen(filename, mode), LINE_BUFFER_SIZE);
}

// destroys the FileLineIterator as well as the underlying LineIterator objects
void FileLineIterator_del(FileLineIterator * file_iter) {
    fclose(file_iter->lines->_h); // FileLineIterator owns the FILE handle
    LineIterator_del(file_iter->lines);
    file_iter->lines = NULL;
    IO_FREE(file_iter);
}

// return pointer to the next line of characters, nul terminated
char * FileLineIterator_next(FileLineIterator * file_iter) {
    if (!file_iter) {
        return NULL;
    }
    return LineIterator_next(file_iter->lines);
}

// destroys the FileLineIterator if stops and tells caller whether to stop or not
enum iterator_status FileLineIterator_stop(FileLineIterator * file_iter) {
    if (!file_iter) {
        return ITERATOR_STOP;
    }
    // used file_iter->lines->stop, but after update to have file_iter->stop track this value, should be equivalent
    if (file_iter->lines->stop == ITERATOR_STOP) {
        FileLineIterator_del(file_iter);
        return ITERATOR_STOP;
    }
    return file_iter->lines->stop;
}

// fully qualified LineIterator constructor from file stream and a buffer size
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

// create a LineIterator from just a file stream
LineIterator * LineIterator_iter1(FILE * fstr) {
    return LineIterator_new(fstr, LINE_BUFFER_SIZE);
}

// initializes all the internal variables of the LineIterator
void LineIterator_init(LineIterator * lines, FILE * fstr, size_t next_buf_size) {
    lines->_h = fstr;
    lines->stop = ITERATOR_GO;
    for (size_t i = 0; i < next_buf_size; i++) {
        lines->next[i] = '\0';
    }
    lines->next_buf_size = next_buf_size;
}

// destroys the LineIterator object
void LineIterator_del(LineIterator * lines) {
    IO_FREE(lines->next);
    lines->next = NULL;
    IO_FREE(lines);
}

// return pointer to the next line of characters, nul terminated
char * LineIterator_next(LineIterator * lines) {
    if (!lines) {
        return NULL;
    }

#if defined(_posix_) || defined(__STDC_ALLOC_LIB__)
    if (getline(&lines->next, &lines->next_buf_size, lines->_h) <= 0) {
        if (feof(lines->_h)) {
            lines->stop = ITERATOR_STOP;
        }
        return NULL;
    }
#else // basically for Windows

    // this is the non-posix version. For posix, use getline() in stdio.h to update LineIterator
    char * test = fgets(lines->next, lines->next_buf_size, lines->_h);
    if (!test) { // fgets failed or EOF is encountered immediately
        if (feof(lines->_h)) {
            lines->stop = ITERATOR_STOP;
        }
        return NULL;
    }
    size_t nchar = strlen(lines->next); // strlen is O(n)
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

        // allocate a new buffer
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
    
#endif // _posix_ || __STDC_ALLOC_LIB__

    // either end of file was encountered or last character is a linefeed. In this case test is lines->next
    return lines->next;   
    
    // additionally need to handle the case of classic MAC? there's no line feed and so fgets fails, but not sure if it has getline()
}

// destroys the LineIterator if stops and tells caller whether to stop or not
enum iterator_status LineIterator_stop(LineIterator * lines) {
    if (!lines) {
        return ITERATOR_STOP;
    }
    if (lines->stop == ITERATOR_STOP) {
        LineIterator_del(lines);
        return ITERATOR_STOP;
    }
    return lines->stop;
}

// fully qualified constructor for TokenIterator object
// if delimiters is an empty string (strlen(delimiters) == 0) or NULL, uses WHITESPACE delimiters and _group is set to true (contiguous whitespace is treated as 1 delimiter)
TokenIterator * TokenIterator_new(char * string, char * delimiters, size_t next_buf_size) {
    if (!string) {
        return NULL;
    }
    TokenIterator * tokens = (TokenIterator *) IO_MALLOC(sizeof(TokenIterator));
    if (!tokens) {
        return NULL;
    }

    if (!next_buf_size) {
        next_buf_size = TOKEN_BUFFER_SIZE;
    }

    tokens->next = (char *) IO_MALLOC(sizeof(char) * next_buf_size);
    if (!tokens->next) {
        IO_FREE(tokens);
        return NULL;
    }

    TokenIterator_init(tokens, string, delimiters, next_buf_size);

    return tokens;
}

// simple constructor of a TokenIterator from a filename and read mode, uses default buffer size (stdio.BUF_SIZE)
TokenIterator * TokenIterator_iter2(char * string, char * delimiters) {
    return TokenIterator_new(string, delimiters, TOKEN_BUFFER_SIZE);
}

// simple constructor of a TokenIterator from only a filename. uses default read mode ("rb") and buffer size (stdio.BUF_SIZE)
TokenIterator * TokenIterator_iter1(char * string) {
    return TokenIterator_new(string, NULL, TOKEN_BUFFER_SIZE);
}

// initializes all the internal variables of the TokenIterator object
void TokenIterator_init(TokenIterator * tokens, char * string, char * delimiters, size_t next_buf_size) {
    tokens->string = string;
    if (delimiters && (strlen(delimiters) > 0)) {
        tokens->_group = false;
        tokens->delimiters = delimiters;
    } else {
        tokens->_group = true;
        tokens->delimiters = WHITESPACE;
    }
    for (size_t i = 0; i < next_buf_size; i++) {
        tokens->next[i] = '\0';
    }
    tokens->next_buf_size = next_buf_size;
    tokens->loc = -1;

    tokens->stop = ITERATOR_GO;
}

// destroys the TokenIterator as well as the underlying LineIterator objects
void TokenIterator_del(TokenIterator * tokens) {
    IO_FREE(tokens->next);
    tokens->next = NULL;
    IO_FREE(tokens);
}

// return pointer to the next line of characters, nul terminated
char * TokenIterator_next(TokenIterator * tokens) {
    if (!tokens) {
        return NULL;
    }
    // this next block my be less clumsy if one version in the tokens->group and one in the corresponding 
    // else block the idea is that NULL delimiters should not iterator for an empty string, but any other 
    // delimiters should return a single empty token. Since tokens->loc starts at -1, this block with the 
    // conditional !tokens->group ensures you get at least an empty string
    if (tokens->loc >= 0 && tokens->string[tokens->loc] == '\0') {
        tokens->stop = ITERATOR_STOP;
        return NULL;
    }
    char * start;
    size_t next_size;
    if (tokens->_group) {
        while (is_in_delimiter_set(tokens->string[tokens->loc+1], tokens->delimiters)) {
            tokens->loc++;
        }
        
        if (tokens->string[tokens->loc + 1] == '\0') {
            tokens->stop = ITERATOR_STOP;
            return NULL;
        }
        
        start = tokens->string + tokens->loc + 1;
        while (!is_in_delimiter_set(tokens->string[tokens->loc+1], tokens->delimiters) && tokens->string[tokens->loc+1] != '\0') {
            tokens->loc++;
        }
        tokens->loc++;
        
        next_size = (tokens->string + tokens->loc) - start;
    } else {
        // POSIX has a built-in feature getdelim() that should serve the purpose, but you have to find the first delimiter and account for whitespace
        // standard algorithm, find next instance of any of the delimiters and extract the substring into next, resetting the buffer if necessary
        //char * start = tokens->string + tokens->loc + 1;
        
        start = tokens->string + tokens->loc + 1;
        char * left = start;
        char * right = NULL;
        
        size_t i = 0;
        //size_t j = 0;
        //while (j < 4 && *left != '\0' && tokens->delimiters[i] != '\0') {
        while (*left != '\0' && tokens->delimiters[i] != '\0') {
            
            right = strchr(left, tokens->delimiters[i]);
            if (!right) {
                while (*left != '\0') {
                    left++;
                }
            } else {
                if (i > 0) {
                    if (right - left == 1) {
                        i++;
                        left = right;
                    } else { // not sequential delimters; start over where the current character might have been in the sequence of delimiters
                        left = right - i;
                        i = 0;
                    }
                } else {
                    i++;
                    left = right;
                }
            }
            //j++;
        }
        
        // left now either points to the end of the string or the last delimiter in delimiters (where location needs to be)

        // if left is at the end fo the string, simply use everythign from start to left, otherwise start to left - the number of delimiters
        if (*left == '\0') {
            next_size = (left - start);
            tokens->loc += next_size + 1;
        } else {
            next_size = (left - start) - i + 1;
            tokens->loc += next_size + i;
        }
    }

    //printf("next_size = %zu, tokens->loc = %lld\n", next_size, tokens->loc);

    // check whether buffer is large enough and resize if necessary
    if (next_size >= tokens->next_buf_size) {
        char * new_buf = (char *) IO_REALLOC(tokens->next, sizeof(char) * (next_size + 1)); // probably should re-alloc more intelligently to reduce number of allocations
        if (!new_buf) {
            return NULL;// TODO: CONSIDER: how to handle failures to realloc while failing to capture full line...maybe just proceed as normal?
        }

        tokens->next = new_buf;
        tokens->next_buf_size = next_size + 1;

        for (size_t i = tokens->next_buf_size; i < next_size + 1; i++) {
            tokens->next[i] = '\0';
        }
    }

    strncpy(tokens->next, start, next_size);
    tokens->next[next_size] = '\0';

    return tokens->next;
}

// destroys the TokenIterator if stops and tells caller whether to stop or not
enum iterator_status TokenIterator_stop(TokenIterator * tokens) {
    if (!tokens) {
        return ITERATOR_STOP;
    }
    // used file_iter->lines->stop, but after update to have file_iter->stop track this value, should be equivalent
    if (tokens->stop == ITERATOR_STOP) {
        TokenIterator_del(tokens);
        return ITERATOR_STOP;
    }
    return tokens->stop;
}