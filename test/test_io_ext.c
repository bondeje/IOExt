#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../src/io_ext.h"
#include "../src/csv.h"

/*
TODO list:
--create malformed csv file and test that reader exits and dumps memory properly
----unescaped quotes
----incorrect quote escaping
--finish writer
--finish amender
*/

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

const char * test_line_files[N_TEST_FILES] =     { 
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
        file = fopen(test_line_files[i], "rb");
        if (file_exists[i]) {
            ASSERT(file, "\nfailed to open file %s in test_LineLiterator.", test_line_files[i]);
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
                ASSERT(line, "\nLineIterator_next failed to return line in test_LineIterator on line %zu in test file %s.", line_count, test_line_files[i]);
                ASSERT(lines->next_buf_size > line_lengths[i][line_count], "\ninsufficient buffer size allocated in test_LineIterator for line %zu in test_line_files %s, expected > %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], lines->next_buf_size);
                ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_LineIterator for line %zu in test file %s, expected %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], strlen(line));
                line = NULL;
                line = LineIterator_next(lines);
                line_count++;
            }
            LineIterator_del(lines);
            lines = NULL;
            fclose(file);
        } else {
            ASSERT(!lines, "\nfailed to retun null LineIterator in test_LineIterator for non-existent file %s.", test_line_files[i]);
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
            file_iter = FileLineIterator_new(test_line_files[i], "rb", 0);
        } else {
            file_iter = FileLineIterator_new(test_line_files[i], "rb", buf_sizes[i]);
        }
        if (file_exists[i]) {
            ASSERT(file_iter, "\nfailed to open file %s in test_FileLineIterator.", test_line_files[i]);
            ASSERT(file_iter->lines, "\nfailed to dynamically allocate LineIterator in test_FileLineIterator.");
            ASSERT(file_iter->lines->next_buf_size == buf_sizes[i], "\nfailed to allocate default buffer size %zu in test_FileLineIterator, found %zu", buf_sizes[i], file_iter->lines->next_buf_size);
            ASSERT(strlen(file_iter->lines->next) == 0, "\nfailed to initialize next line to 0-length string, found %zu", strlen(file_iter->lines->next));
            ASSERT(file_iter->lines->stop == ITERATOR_GO, "\nfailed to initialize LineIterator to be able to start in test_FileLineIterator.");

            line = NULL;
            line = FileLineIterator_next(file_iter);
            while (file_iter->lines->stop != ITERATOR_STOP) {
                ASSERT(line, "\nLineIterator_next failed to return line in test_FileLineIterator on line %zu in test file %s.", line_count, test_line_files[i]);
                ASSERT(file_iter->lines->next_buf_size > line_lengths[i][line_count], "\ninsufficient buffer size allocated in test_FileLineIterator for line %zu in test_line_files %s, expected > %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], file_iter->lines->next_buf_size);
                ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_FileLineIterator for line %zu in test file %s, expected %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], strlen(line));
                line = NULL;
                line = FileLineIterator_next(file_iter);
                line_count++;
            }
            FileLineIterator_del(file_iter);
        } else {
            ASSERT(!file_iter, "\nfailed to retun null FileLineIterator in test_FileLineIterator for non-existent file %s.", test_line_files[i]);
        } 
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_FileLineIterator in file %s, expected %zu, found %zu.", test_line_files[i], n_lines[i], line_count);
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
        for_each(char, line, FileLine, test_line_files[i]) {
            ASSERT(line, "\nLineIterator_next failed to return line in test_for_each on line %zu in test file %s.", line_count, test_line_files[i]);
            ASSERT(strlen(line) == line_lengths[i][line_count], "\nline length does not match expected output in test_for_each for line %zu in test file %s, expected %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], strlen(line));
            line_count++;
        }
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_for_each in file %s, expected %zu, found %zu.", test_line_files[i], n_lines[i], line_count);
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
        for_each_enumerate(char, line, FileLine, test_line_files[i]) {
            ASSERT(line.val, "\nFileLineIterator_next failed to return line in test_for_each_enumerate on line %zu in test file %s.", line.i, test_line_files[i]);
            ASSERT(line_count == line.i, "\nfailed to enumerate the lines in test_for_each_enumerate in file %s. Lines read - 1 = %zu, enumeration = %zu.", test_line_files[i], line_count, line.i);
            ASSERT(strlen(line.val) == line_lengths[i][line_count], "\nline length does not match expected output in test_for_each_enumerate for line %zu in test file %s, expected %zu, found %zu.", line_count, test_line_files[i], line_lengths[i][line_count], strlen(line.val));
            line_count++;
        }
        ASSERT(line_count == n_lines[i], "\nfailed to collect all lines in test_for_each_enumerate in file %s, expected %zu, found %zu.", test_line_files[i], n_lines[i], line_count);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_string_ends_with(void) {
    printf("test_string_ends_with...");
    // need to do this because array dims must be const, but const keyword does not suffice...
    // must be literal. Only way to use variable name is through enum
    enum {nstrings=4, nends=7}; 
    char * strings[nstrings] = {
                                "abc\r\n",
                                "abc\n",
                                "abc\r",
                                "abc"
                                };
    char * ends[nends] = {
                        "\r\n",
                        "\r",
                        "\n",
                        "c",
                        "ac",
                        "abc",
                        "abcd"
                        };
    bool results[nstrings][nends] = {
                                    {true, false, true, false, false, false, false},
                                    {false, false, true, false, false, false, false},
                                    {false, true, false, false, false, false, false},
                                    {false, false, false, true, false, true, false}
                                    };

    for (size_t i = 0; i < nstrings; i++) {
        for (size_t j = 0; j < nends; j++) {
            ASSERT(String_ends_with(strings[i], ends[j]) == results[i][j], "\nFailed to correctly identifying that %s ends with %s in test_string_ends_with, should be %s", strings[i], ends[j], (results[i][j]) ? "true" : "false");
        }
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_string_rstrip(void) {
    printf("test_string_rstrip...");
    // need to do this because array dims must be const, but const keyword does not suffice...
    // must be literal. Only way to use variable name is through enum
    enum {nstrings=10, max_length=7};
    char * strings[nstrings] = {
                                "abc\n",
                                "abc\r",
                                "abc",
                                "abc\t",
                                "abc\f",
                                "abc\v",
                                "abc\r\n",
                                " abc\r\n",
                                "abc \r\n",
                                "abc\nd"
                                };
    char * a = "abc";
    char * sa = " abc";
    char * results[nstrings] = {
                                a,
                                a,
                                a,
                                a,
                                a,
                                a,
                                a,
                                sa,
                                a,
                                "abc\nd"
                                };

    for (size_t i = 0; i < nstrings; i++) {
        char out[max_length] = {'\0'};
        memcpy(out, strings[i], strlen(strings[i]) + 1);
        String_rstrip(out);
        ASSERT(!strcmp(out, results[i]), "\nFailed to strip white space from %s in test_string_rstrip. expected %s, found %s", strings[i], results[i], out);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_string_lstrip(void) {
    printf("test_string_lstrip...");
    // need to do this because array dims must be const, but const keyword does not suffice...
    // must be literal. Only way to use variable name is through enum
    enum {nstrings=9, max_length=10};
    char * strings[nstrings] = {
                                "\nabc\n",
                                "\rabc\r",
                                "abc",
                                "\tabc\t",
                                "\fabc\f",
                                "\vabc\v",
                                "\r\nabc\r\n",
                                "\r\n abc \r\n",
                                "d\nabc\nd"
                                };
    char * results[nstrings] = {
                                "abc\n",
                                "abc\r",
                                "abc",
                                "abc\t",
                                "abc\f",
                                "abc\v",
                                "abc\r\n",
                                "abc \r\n",
                                "d\nabc\nd"
                                };

    for (size_t i = 0; i < nstrings; i++) {
        char out[max_length] = {'\0'};
        memcpy(out, strings[i], strlen(strings[i]) + 1);
        String_lstrip(out);
        ASSERT(!strcmp(out, results[i]), "\nFailed to strip white space from %s in test_string_rstrip. expected %s, found %s", strings[i], results[i], out);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_string_strip(void) {
    printf("test_string_strip...");
    // need to do this because array dims must be const, but const keyword does not suffice...
    // must be literal. Only way to use variable name is through enum
    enum {nstrings=9, max_length=10};
    char * strings[nstrings] = {
                                "\nabc\n",
                                "\rabc\r",
                                "abc",
                                "\tabc\t",
                                "\fabc\f",
                                "\vabc\v",
                                "\r\nabc\r\n",
                                "\r\n abc \r\n",
                                "d\nabc\nd"
                                };
    char * a = "abc";
    char * results[nstrings] = {
                                a,
                                a,
                                a,
                                a,
                                a,
                                a,
                                a,
                                a,
                                "d\nabc\nd"
                                };

    for (size_t i = 0; i < nstrings; i++) {
        char out[max_length] = {'\0'};
        memcpy(out, strings[i], strlen(strings[i]) + 1);
        String_strip(out);
        ASSERT(!strcmp(out, results[i]), "\nFailed to strip white space from %s in test_string_rstrip. expected %s, found %s", strings[i], results[i], out);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_TokenIterator(void) {
    printf("test_TokenIterator...");
    char test[256] = {'\0'};
    char string[256] = {'\0'};
    bool string_is_null = false;
    char delimiters[16] = {'\0'};
    size_t ith_token;

    FileLineIterator * file_iter = FileLineIterator_iter("./data/test_tokens.txt");
    char * line = "";
    while (strcmp(FileLineIterator_next(file_iter), "#endheader\r\n")) {}

    line = FileLineIterator_next(file_iter);
    while (FileLineIterator_stop(file_iter) != ITERATOR_STOP) {
        // skip empty lines
        String_rstrip(line);
        while (!strcmp("", line)) {
            line = FileLineIterator_next(file_iter);
            if (line) {
                String_rstrip(line);
            } else {
                break;
            }
        }

        if (!line) {
            continue;
        }

        // description
        memcpy(test, line, strlen(line)+1);
        ith_token = 0;

        //string
        string_is_null = false;
        line = FileLineIterator_next(file_iter);
        String_rstrip(line);
        if (!strcmp(line, "NULL")) {
            string_is_null = true;
        } else {
            memcpy(string, line, strlen(line)+1);
        }
        
        //delimiters
        line = FileLineIterator_next(file_iter);
        String_rstrip(line);

        // create token iterator
        TokenIterator * tokens = NULL;
        if (strcmp("NULL", line)) {
            memcpy(delimiters, line, strlen(line)+1);
            tokens = TokenIterator_iter(string_is_null ? NULL : string, delimiters);
        } else {
            memcpy(delimiters, WHITESPACE, strlen(WHITESPACE)+1);
            tokens = TokenIterator_iter(string_is_null ? NULL : string, NULL);
        }
        
        char * token = NULL;
        line = FileLineIterator_next(file_iter);
        String_rstrip(line);
        token = TokenIterator_next(tokens);
        while (strcmp("#endtest", line)) {
            ASSERT(!strcmp(line, token), "\nFailed to tokenize %zu-th token in %s/%s with delimiters %s in test_token_iterator. Expected %s, found %s", ith_token, test, string, delimiters, line, token);
            ith_token++;
            line = FileLineIterator_next(file_iter);
            String_rstrip(line);            
            token = TokenIterator_next(tokens);
        }
        
        ASSERT(TokenIterator_stop(tokens) == ITERATOR_STOP, "\nTokenIterator failed stop in test_token_iterator. On string %s/%s with delimiters %s after %zu-th token. Last token %s", test, string, delimiters, ith_token, token);
        tokens = NULL;
        
        line = FileLineIterator_next(file_iter);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_array_iterators(void) {
    printf("test_array_iterators...");
    char test[256] = {'\0'};
    int arr[256] = {0};
    char type[32] = {'\0'};

    FileLineIterator * file_iter = FileLineIterator_iter("./data/test_array_iterators.txt");
    char * line = "";
    while (strcmp(FileLineIterator_next(file_iter), "#endheader\r\n")) {}

    line = FileLineIterator_next(file_iter);
    while (FileLineIterator_stop(file_iter) != ITERATOR_STOP) {
        String_rstrip(line);
        while (!strcmp("", line)) {
            line = FileLineIterator_next(file_iter);
            if (line) {
                String_rstrip(line);
            } else {
                break;
            }
        }

        if (!line) {
            continue;
        }

        memcpy(test, line, strlen(line)+1);
        //printf("\ntest: %s", test);

        sscanf(String_rstrip(FileLineIterator_next(file_iter)), "%s", type);
        size_t num_arr = 0;
        {
        for_each_enumerate(char, elem, Token, String_rstrip(FileLineIterator_next(file_iter))) {
            if (!strcmp(type, "int")) {
                sscanf(elem.val, "%d", arr + elem.i);
                num_arr++;
            }
        }
        }

        line = String_rstrip(FileLineIterator_next(file_iter));
        if (!strcmp("NULL", line)) {
            // do slicing

            line = String_rstrip(FileLineIterator_next(file_iter)); // skip result line

            size_t num_found = 0;
            for_each(int, v, int, arr, num_arr) {
                ASSERT(arr[num_found] == *v, "\nfailed to slice into array in test_array_iterators, test %s, result index %zu, expected %d, found %d", test, num_found, arr[num_found], *v);
                num_found++;
            }

            ASSERT(num_found == num_arr, "\nfailed to find the same number of elements in slice as expected results in test_array_iterators, test %s, expected %zu, found %zu", test, num_arr, num_found);
        } else {
            int res[256] = {0};
            size_t num_res = 0;
            size_t start;
            size_t stop;
            long long int step;
            sscanf(line, "%zu, %zu, %lld", &start, &stop, &step);
            //printf("\nstart: %zu, stop: %zu, step: %lld", start, stop, step);
            {
            for_each_enumerate(char, elem, Token, String_rstrip(FileLineIterator_next(file_iter))) {
                if (!strcmp(type, "int")) {
                    sscanf(elem.val, "%d", res + elem.i);
                    num_res++;
                }
            }
            }

            size_t num_found = 0;
            for_each(int, v, intIterator, int_slice(arr, num_arr, start, stop, step)) {
                ASSERT(res[num_found] == *v, "\nfailed to slice into array in test_array_iterators, test %s, result index %zu, expected %d, found %d", test, num_found, res[num_found], *v);
                num_found++;
            }

            ASSERT(num_found == num_res, "\nfailed to find the same number of elements in slice as expected results in test_array_iterators, test %s, expected %zu, found %zu", test, num_res, num_found);
        }

        line = String_rstrip(FileLineIterator_next(file_iter)); // #endtest
        line = FileLineIterator_next(file_iter); // next line
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int test_csv_reader(void) {
    printf("test_csv_reader...");
    CSVFile * csv;
    char test[256] = {'\0'};
    char file_path[256] = {'\0'};
    char format[32] = {'\0'};

    enum {MAX_INPUT_STRING_SIZE = 256};

    FileLineIterator * file_iter = FileLineIterator_iter("./data/test_csv_reader.txt");
    char * line = "";
    while (strcmp(FileLineIterator_next(file_iter), "#endheader\r\n")) {}

    line = FileLineIterator_next(file_iter);
    while (FileLineIterator_stop(file_iter) != ITERATOR_STOP) {
        String_rstrip(line);
        while (!strcmp("", line)) {
            line = FileLineIterator_next(file_iter);
            if (line) {
                String_rstrip(line);
            } else {
                break;
            }
        }

        if (!line) {
            continue;
        }

        memcpy(test, line, strlen(line)+1);
        //printf("\ntest: %s", test);

        line = String_strip(FileLineIterator_next(file_iter));
        memcpy(file_path, line, strlen(line)+1);
        //printf("\nfile_path: %s", file_path);

        line = String_strip(FileLineIterator_next(file_iter));

        csv = CSVFile_new(file_path, CSV_READER, !strcmp(line, "true"), NULL, NULL);

        line = String_strip(FileLineIterator_next(file_iter));
        memcpy(format, line, strlen(line)+1);
        //printf("\nformat: %s", format);

        line = String_strip(FileLineIterator_next(file_iter));
        TokenIterator * tokens = NULL;
        while (strcmp("#endtest", line)) {
            tokens = TokenIterator_iter(line, ",");
            char * func = TokenIterator_next(tokens);
            if (!strcmp(func, "get_cell")) {
                size_t irec = 0, ifie = 0;
                sscanf(String_strip(TokenIterator_next(tokens)), "%zu", &irec);
                sscanf(String_strip(TokenIterator_next(tokens)), "%zu", &ifie);
                if (!strcmp(format, "%d")) {
                    int expected = 0, found = 0;
                    int err = CSVFile_get_cell(csv, irec, ifie, format, &found);
                    char * result = String_strip(TokenIterator_next(tokens));
                    if (!strcmp(result, "NULL")) {
                        ASSERT(err, "\nfailed to find an empty field in test_csv_reader in test %s for file %s at (%zu, %zu)", test, file_path, irec, ifie);
                    } else {
                        sscanf(result, format, &expected);
                        ASSERT(expected==found, "\nfailed to find populated field in test_csv_reader in test %s for file %s at (%zu, %zu), expected: %d, found %d", test, file_path, irec, ifie, expected, found);
                    }
                    
                } else if (!strcmp(format, "%[^\\0]")) {
                    char expected[MAX_INPUT_STRING_SIZE] = {'\0'};
                    char found[MAX_INPUT_STRING_SIZE] = {'\0'};
                    int err = CSVFile_get_cell(csv, irec, ifie, format, found);
                    //printf("\nerr: %i, n_fields %zu in record %zu", err, csv->records[irec]->n_fields, irec);
                    char * result = String_strip(TokenIterator_next(tokens));
                    if (!strcmp(result, "NULL")) {
                        ASSERT(err, "\nfailed to find an empty field in test_csv_reader in test %s for file %s at (%zu, %zu)", test, file_path, irec, ifie);
                    } else if (!strcmp(result, "PRINT")) {
                        #ifndef NDEBUG
                        printf("\nfound: %s, (len: %zu)", found, strlen(found));
                        #endif // NDEBUG
                    } else {
                        //printf("\nfound: %s", found);
                        sscanf(result, format, &expected);
                        ASSERT(!strcmp(expected, found), "\nfailed to find populated field in test_csv_reader in test %s for file %s at (%zu, %zu), expected: %s, found %s", test, file_path, irec, ifie, expected, found);
                    }
                } else {
                    printf("\nformat %s for test not supported in test %s with file %s", format, test, file_path);
                }
            }
            

            TokenIterator_del(tokens);

            line = String_strip(FileLineIterator_next(file_iter));
        }

        CSVFile_del(csv);

        line = FileLineIterator_next(file_iter);
    }

    printf("PASS\n");

    return TEST_SUCCESS;
}

int main() {
    test_LineIterator();
    test_FileLineIterator();
    test_TokenIterator();
    test_for_each();
    test_for_each_enumerate();
    test_string_ends_with();
    test_string_rstrip();
    test_string_lstrip();
    test_string_strip();
    test_array_iterators();

    test_csv_reader();
    
    return 0;
}