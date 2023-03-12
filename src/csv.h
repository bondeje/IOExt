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

/*
TODO: need to be able to track the max field count in the CSVFile struct so that I can validate enough fields exist SOMEWHERE
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
#define DEFAULT_N_RECORDS 32
#define DEFAULT_N_FIELDS 8
#define DEFAULT_MEM_SCALE 2
#define DEFAULT_LINE_ENDING "\r\n"

// modes
#define CSV_READER 'r'
#define CSV_WRITER 'w'
#define CSV_AMENDER 'a'

#define CSV_CELL_BUFFER_SIZE 32768

#define FIELD_BUFFER_SIZE 32

#define select_CSVFileIterator_iter(_1,_2,_3,_4, NAME,...) NAME
#define CSVFileIterator_iter(...) Select_CSVFileIterator_iter(__VA_ARGS__, CSVFileIterator_new, CSVFileIterator_iter3, NONE, NONE, UNUSED)(__VA_ARGS__)

enum csv_status {
    CSV_INDEX_ERROR = -4,
    CSV_MEMORY_ERROR = -3,
    CSV_READ_ERROR = -2,
    CSV_FAILURE = -1,
    CSV_SUCCESS = 0,
};

enum csv_axis {
    CSV_COLUMN,
    CSV_ROW,
};

typedef struct CSVRecord {
    // replace with a stack of size_t
    size_t * field_pos; // positions of fields. allocation size if n_fields_alloc + 1, First value is start of record, each subsequent value is the end of a field
    // replace with a stack of c strings
    char ** fields; // array of c strings for fields. For writing only
    size_t n_fields; // number of fields found
    size_t n_fields_alloc; // number of fields allocated. 
} CSVRecord;

typedef struct CSVFile {
    FILE * handle;
    FILE * handle_file_out; // only used in "amend" mode
    CSVRecord ** records; // array of records
    size_t n_records; // number of records
    size_t n_records_alloc; // N_RECORDS allocation
    size_t line_ending_size ;
    char * filename;
    char * file_out;
    char * line_ending; // normally just "\r\n"
    char mode; // read, write, amend
    bool has_header;
} CSVFile;

typedef struct CSVFileIterator {
    CSVFile * csv;
    char * next;
    size_t next_size;
    size_t index;
    size_t axis_index;
    size_t start_;
    size_t stop_;
    size_t step_;
    enum csv_axis axis;
    enum iterator_status stop;
} CSVFileIterator, CSVFileIteratorIterator;

CSVFile * CSVFile_new(char * filename, char mode, bool has_header, char * line_ending, char * file_out);
void CSVFile_init(CSVFile * csv, char * filename, char mode, bool has_header, char * line_ending, char * file_out);
void CSVFile_del(CSVFile * csv);
enum csv_status CSVFile_append_record(CSVFile * csv, size_t pos);

CSVFileIterator * CSVFileIterator_new(CSVFile * csv, size_t index, enum csv_axis axis, size_t next_buffer_size);
CSVFileIterator * CSVFileIterator_iter3(CSVFile * csv, size_t index, enum csv_axis axis);
void CSVFileIterator_init(CSVFileIterator * csv_iter, CSVFile * csv, size_t index, enum csv_axis axis, size_t next_buffer_size);
void CSVFileIterator_del(CSVFileIterator * csv_iter);
// NULL means failure or stop condition and NOT an empty field
char * CSVFileIterator_next(CSVFileIterator * csv_iter);
enum iterator_status CSVFileIterator_stop(CSVFileIterator * csv_iter);
CSVFileIterator * CSVFileIteratorIterator_iter(CSVFileIterator * csv_iter);
char * CSVFileIteratorIterator_next(CSVFileIteratorIterator * csv_iter);
enum iterator_status CSVFileIteratorIterator_stop(CSVFileIteratorIterator * csv_iter);

CSVRecord * CSVRecord_new(char mode, size_t start, size_t init_field_alloc);
void CSVRecord_init(CSVRecord * csvr, char mode, size_t start, size_t init_field_alloc);
void CSVRecord_del(CSVRecord * csvr);
// only use in CSV_READER mode or when adding a new record, otherwise do not use in CSV_AMENDER mode
enum csv_status CSVRecord_append_field_pos(CSVRecord * csvr, size_t pos);

enum csv_status CSVFile_read(CSVFile * csv);
int CSVFile_write(CSVFile * csv);

// for builders
/*
int CSVFile_append_record(CSVFile * csv, char ** record, size_t n_fields);
int CSVFile_append_field(CSVFile * csv, size_t record, char * format, ...);
*/

int CSVFile_set_cell(CSVFile * csv, size_t record, size_t field, char * format, ...);
// NOTE: to actually read a cell into a single string, format should be %[^\0] as just %s will stop at the first space. %[^\0] will collect all characters until string terminator
enum csv_status CSVFile_get_cell(CSVFile * csv, size_t record, size_t field, char * format, ...);
CSVFileIterator * CSVFile_get_column(CSVFile * csv, size_t icolumn);
CSVFileIterator * CSVFile_get_column_slice(CSVFile * csv, size_t icolumn, size_t start, size_t stop, size_t step);
CSVFileIterator *  CSVFile_get_row(CSVFile * csv, size_t irow);
CSVFileIterator * CSVFile_get_row_slice(CSVFile * csv, size_t irow, size_t start, size_t stop, size_t step);

#endif // CSV_H