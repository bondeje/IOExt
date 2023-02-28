#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/IOExt.h"

// debugging shorthands
#ifndef NDEBUG
	#define DEBUG_EXPR(x) x
	#define DEBUG_PRINT(x) printf x
#else
	#define DEBUG_EXPR(x) do {} while (0)
	#define DEBUG_PRINT(x) do {} while (0)
#endif

#define ASSERT(stmt, ...) assert((stmt) || !printf(__VA_ARGS__))

#define TEST_SUCCESS 0
#define TEST_FAILURE -1

#define N_TEST_FILES 5

const char * test_files[N_TEST_FILES] =     { 
                                            "./data/lines.txt",
                                            "./data/lines_realloc.txt",
                                            "./data/lines_realloc_nolf.txt",
                                            "./data/lines_empty_final_line.txt",
                                            "./data/does_not_exist.txt"
                                            };

const int file_exists[N_TEST_FILES] = {1, 1, 1, 1, 0};

const size_t n_lines[N_TEST_FILES] =    {5, 5, 3, 4, 0};

// yes, this is column-major so that I can initialize it properly
const size_t line_lengths[N_TEST_FILES][5] =    {  
                                                {3, 3, 3, 3, 1}, 
                                                {3, 3, 529, 3, 1},
                                                {3, 3, 527},
                                                {3, 3, 3, 3, 3},
                                                {0}
                                                };

const size_t buf_sizes[N_TEST_FILES] =  {LINE_BUFFER_SIZE, 3, 5, 5, 0};

int test_LineIterator(void) {
    printf("test_LineIterator...");
    FILE * file = NULL;
    char * line = NULL;
    size_t line_count = 0;
    LineIterator * lines = NULL;
    for (int i = 0; i < N_TEST_FILES; i++) {
        file = fopen(test_files[i], "rb");
        if (file_exists[i]) {
            ASSERT(file, "\nfailed to open file %s in test_LineLiterator.", test_files[i]);
        }
        line_count = 0;
        if (buf_sizes[i] == LINE_BUFFER_SIZE) {
            lines = LineIterator_new(file, 0);
        } else {
            lines = LineIterator_new(file, buf_sizes[i]);
        }
        if (file_exists[i]) {
            ASSERT(lines, "\nfailed to dynamically allocate LineIterator in test_LineIterator.");
            ASSERT(lines->next_buf_size == buf_sizes[i], "\nfailed to allocate default buffer size %zu in test_LineIterator, found %zu", buf_sizes[i], lines->next_buf_size);
            ASSERT(strlen(lines->next) == 0, "\nfailed to initialize next line to 0-length string, found %zu", strlen(lines->next));
            ASSERT(lines->stop == ITERATOR_GO, "\nfailed to initialize LineIterator to be able to start in test_LineIterator.");
            
            line = NULL;
            line = LineIterator_next(lines);
            while (lines->stop != ITERATOR_STOP) {
                ASSERT(line, "\nLineIterator_next failed to return line in test_LineIterator on line %zu in test file %s.", line_count, test_files[i]);
                ASSERT(lines->next_buf_size > line_lengths[i][line_count], "\ninsufficient buffer size allocated in test_LineIterator for line %zu in test_files %s, expected > %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], lines->next_buf_size);
                ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_LineIterator for line %zu in test file %s, expected %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], strlen(line));
                line = NULL;
                line = LineIterator_next(lines);
                line_count++;
            }
            LineIterator_del(lines);
            lines = NULL;
            fclose(file);
        } else {
            ASSERT(!lines, "\nfailed to retun null LineIterator in test_LineIterator for non-existent file %s.", test_files[i]);
        }
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in the file, expected %zu, found %zu.", n_lines[i], line_count);
        
        file = NULL;
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_FileLineIterator(void) {
    printf("test_FileLineIterator...");
    char * line = NULL;
    size_t line_count = 0;
    FileLineIterator * file_iter = NULL;
    for (int i = 0; i < N_TEST_FILES; i++) {
        line_count = 0;
        if (buf_sizes[i] == LINE_BUFFER_SIZE) {
            file_iter = FileLineIterator_new(test_files[i], "rb", 0);
        } else {
            file_iter = FileLineIterator_new(test_files[i], "rb", buf_sizes[i]);
        }
        if (file_exists[i]) {
            ASSERT(file_iter, "\nfailed to open file %s in test_FileLineIterator.", test_files[i]);
            ASSERT(file_iter->lines, "\nfailed to dynamically allocate LineIterator in test_FileLineIterator.");
            ASSERT(file_iter->lines->next_buf_size == buf_sizes[i], "\nfailed to allocate default buffer size %zu in test_FileLineIterator, found %zu", buf_sizes[i], file_iter->lines->next_buf_size);
            ASSERT(strlen(file_iter->lines->next) == 0, "\nfailed to initialize next line to 0-length string, found %zu", strlen(file_iter->lines->next));
            ASSERT(file_iter->lines->stop == ITERATOR_GO, "\nfailed to initialize LineIterator to be able to start in test_FileLineIterator.");

            line = NULL;
            line = FileLineIterator_next(file_iter);
            while (file_iter->lines->stop != ITERATOR_STOP) {
                ASSERT(line, "\nLineIterator_next failed to return line in test_FileLineIterator on line %zu in test file %s.", line_count, test_files[i]);
                ASSERT(file_iter->lines->next_buf_size > line_lengths[i][line_count], "\ninsufficient buffer size allocated in test_FileLineIterator for line %zu in test_files %s, expected > %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], file_iter->lines->next_buf_size);
                ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_FileLineIterator for line %zu in test file %s, expected %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], strlen(line));
                line = NULL;
                line = FileLineIterator_next(file_iter);
                line_count++;
            }
            FileLineIterator_del(file_iter);
        } else {
            ASSERT(!file_iter, "\nfailed to retun null FileLineIterator in test_FileLineIterator for non-existent file %s.", test_files[i]);
        } 
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_FileLineIterator in file %s, expected %zu, found %zu.", test_files[i], n_lines[i], line_count);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_for_each(void) {
    printf("test_for_each...");
    size_t line_count = 0;
    for (int i = 0; i < N_TEST_FILES; i++) {
        line_count = 0;
        // this should work for all, including the files that don't exist
        for_each(char, line, FileLine, test_files[i]) {
            ASSERT(line, "\nLineIterator_next failed to return line in test_for_each on line %zu in test file %s.", line_count, test_files[i]);
            ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_for_each for line %zu in test file %s, expected %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], strlen(line));
            line_count++;
        }
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_for_each in file %s, expected %zu, found %zu.", test_files[i], n_lines[i], line_count);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_for_each_enumerate(void) {
    printf("test_for_each_enumerate...");
    size_t line_count = 0;
    for (int i = 0; i < N_TEST_FILES; i++) {
        line_count = 0;
        // this should work for all, including the files that don't exist
        for_each_enumerate(char, line, FileLine, test_files[i]) {
            ASSERT(line.val, "\nFileLineIterator_next failed to return line in test_for_each_enumerate on line %zu in test file %s.", line.i, test_files[i]);
            ASSERT(line_count == line.i, "\nfailed to enumerate the lines in test_for_each_enumerate in file %s. Lines read - 1 = %zu, enumeration = %zu.", test_files[i], line_count, line.i);
            ASSERT(strlen(line.val) == line_lengths[i][line_count], "\nline length does not match expected output in test_for_each_enumerate for line %zu in test file %s, expected %zu, found %zu.", line_count, test_files[i], line_lengths[i][line_count], strlen(line.val));
            line_count++;
        }
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_for_each_enumerate in file %s, expected %zu, found %zu.", test_files[i], n_lines[i], line_count);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int main() {
    test_LineIterator();
    test_FileLineIterator();
    test_for_each();
    test_for_each_enumerate();

    return 0;

    /*
    FileLineIterator * file = FileLineIterator_new("./data/line_enum.txt", "rb", 0);
    
    printf("iterating using while loop\n");
    {
    char * line = FileLineIterator_next(file);
    while (!FileLineIterator_stop(file)) {
        printf("%zu, %s", strlen(line), line);
        line = FileLineIterator_next(file);
    }
    }

    printf("\n\niterating using for loop\n");
    
    file = FileLineIterator_new("./data/line_enum.txt", "rb", 0);
    {
    for (char * line = FileLineIterator_next(file); !FileLineIterator_stop(file); line = FileLineIterator_next(file)) {
        printf("%zu, %s", strlen(line), line);
    }
    }

    printf("\n\niterating using for_each\n");
    {
    for_each(char, line, FileLine, "./data/line_enum.txt") {
        printf("%zu, %s", strlen(line), line);
    }
    }

    printf("\n\niterating using for_each with additional constructor arguments\n");
    {
    for_each(char, line, FileLine, "./data/line_enum.txt", "rb") {
        printf("%zu, %s", strlen(line), line);
    }
    }

    printf("\n\niterating using for_each_enumerate with additional constructor arguments\n");
    {
    for_each_enumerate(char, line, FileLine, "./data/line_enum.txt", "rb") {
        printf("%zu: %zu, %s", line.i, strlen(line.val), line.val);
    }
    }

    //FileLineIterator_del(file);
    */
}