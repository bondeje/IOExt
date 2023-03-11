#ifndef CSV_H
#define CSV_H

#include <stdlib.h>
#include <stdarg.h>
#include "io_ext.h"

/*
Specification following https://www.rfc-editor.org/rfc/rfc4180 with 2 exceptions:
--Reader will accept ragged field counts, i.e. each row may have a differing number of columns.
----For compatibility with other parsers, writer will ensure non-ragged field counts
--Reader will accept empty fields, which, if recorded or recordable in a writer, are indicated by 
  NULL fields.
----a consequence that violates the RFC4180 is that commas can appear in sequence and immediately 
    precede a line break.
*/

#ifndef CSV_MALLOC
#define CSV_MALLOC malloc
#endif // CSV_MALLOC

#ifndef CSV_REALLOC
#define CSV_REALLOC realloc
#endif // CSV_REALLOC

#ifndef CSV_FREE
#define CSV_FREE free
#endif // CSV_FREE

// defaults
#define DEFAULT_N_RECORDS 128
#define DEFAULT_N_FIELDS 8
#define DEFAULT_LINE_ENDING "\r\n"

// modes
#define CSV_READER 'r'
#define CSV_WRITER 'w'
#define CSV_AMENDER 'a'

#define CSV_CELL_BUFFER_SIZE 32768

enum csv_status {
    CSV_INDEX_ERROR = -4,
    CSV_MEMORY_ERROR = -3,
    CSV_READ_ERROR = -2,
    CSV_FAILURE = -1,
    CSV_SUCCESS = 0,
};

typedef struct CSVRecord {
    // replace with a stack of size_t
    size_t * field_pos; // positions of fields. allocation size if _n_fields_alloc + 1, First value is start of record, each subsequent value is the end of a field
    // replace with a stack of c strings
    char ** fields; // array of c strings for fields. For writing only
    size_t n_fields; // number of fields found
    size_t _n_fields_alloc; // number of fields allocated. 
} CSVRecord;

typedef struct CSVFile {
    FILE * _handle;
    FILE * _handle_file_out; // only used in "amend" mode
    CSVRecord ** records; // array of records
    size_t n_records; // number of records
    size_t _n_records_alloc; // N_RECORDS allocation
    size_t line_ending_size ;
    char * filename;
    char * file_out;
    char * line_ending; // normally just "\r\n"
    char mode; // read, write, amend
} CSVFile;

CSVFile * CSVFile_new(char * filename, char mode, char * line_ending, char * file_out);
void CSVFile_init(CSVFile * csv, char * filename, char mode, char * line_ending, char * file_out);
void CSVFile_del(CSVFile * csv);
int CSVFile_append_record(CSVFile * csv, size_t pos);

CSVRecord * CSVRecord_new(char mode, size_t start, size_t init_field_alloc);
void CSVRecord_init(CSVRecord * csvr, char mode, size_t start, size_t init_field_alloc);
void CSVRecord_del(CSVRecord * csvr);
// only use in CSV_READER mode or when adding a new record, otherwise do not use in CSV_AMENDER mode
int CSVRecord_append_field_pos(CSVRecord * csvr, size_t pos);

int CSVFile_read(CSVFile * csv);
int CSVFile_write(CSVFile * csv);

// for builders
/*
int CSVFile_append_record(CSVFile * csv, char ** record, size_t n_fields);
int CSVFile_append_field(CSVFile * csv, size_t record, char * format, ...);
*/

int CSVFile_set_cell(CSVFile * csv, size_t record, size_t field, char * format, ...);
// NOTE: to actually read a cell into a single string, format should be %[^\0] as just %s will stop at the first space. %[^\0] will collect all characters until string terminator
int CSVFile_get_cell(CSVFile * csv, size_t record, size_t field, char * format, ...);

#endif // CSV_H