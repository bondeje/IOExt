#include <stdio.h>
#include <string.h>
#include "csv.h"

/*
TODO:
*/

#define RESIZE_SCALE 2

/*
READER STATE MACHINE:
-1 - FAILURE - malformed CSV
0 - IN_FIELD - indicates a field that is not enclosed with quotes. Can be followed by IN_FIELD, END_FIELD, END_RECORD, END_CSV
1 - END_FIELD - indicates a comma not part of a quoted field. Can be followed by IN_FIELD, END_FIELD (empty fields) IN_QUOTES, END_RECORD (empty fields), END_CSV
2 - IN_QUOTES - indicates a field that is enclosed with quotes. Can be followed by IN_QUOTES, ESCAPING_QUOTES
3 - ESCAPING_QUOTES - indicates currently in field that is enclosed with quotes and a quote is encountered. Can be followed by IN_QUOTES (successfully escaped quote), END_FIELD, END_RECORD, END_CSV
4 - END_RECORD - indicates a line is ending. Can be followed by END_RECORD, IN_FIELD, or IN_QUOTES, END_CSV
5 - END_CSV

*/
enum reader_states {
    FAILURE = -2,
    UNINITIALIZED,
    IN_FIELD,
    END_FIELD,
    IN_QUOTES,
    ESCAPING_QUOTES,
    END_RECORD,
    END_CSV
};

static char cell_buffer[CSV_CELL_BUFFER_SIZE] = {'\0'};

CSVRecord * CSVRecord_new(char mode, size_t start, size_t init_field_alloc) {
    if (!init_field_alloc) {
        init_field_alloc = DEFAULT_N_FIELDS;
    }
    CSVRecord * new_record = (CSVRecord * ) IO_MALLOC(sizeof(CSVRecord));
    if (!new_record) {
        goto failed_csvrecord_alloc;
    }

    if (mode == CSV_READER || mode == CSV_AMENDER) {
        new_record->field_pos = (size_t *) IO_MALLOC(sizeof(size_t) * (init_field_alloc + 1));
        if (!new_record->field_pos) {
            goto failed_field_pos_alloc;
        }
    } else {
        new_record->field_pos = NULL;
    }

    if (mode == CSV_WRITER || mode == CSV_AMENDER) {
        new_record->fields = (char **) IO_MALLOC(sizeof(size_t) * (init_field_alloc));
        if (!new_record->fields) {
            goto failed_fields_alloc;
        }
    } else {
        new_record->fields = NULL;
    }

    CSVRecord_init(new_record, mode, start, init_field_alloc);

    return new_record;

failed_fields_alloc:
    if (new_record->field_pos) {
        IO_FREE(new_record->field_pos);
    }
failed_field_pos_alloc:
    IO_FREE(new_record);
failed_csvrecord_alloc:
    return NULL;
}

void CSVRecord_init(CSVRecord * csvr, char mode, size_t start, size_t init_field_alloc) {
    if (!init_field_alloc) {
        csvr->n_fields_alloc = DEFAULT_N_FIELDS;
    } else {
        csvr->n_fields_alloc = init_field_alloc;
    }
    if (mode == CSV_READER || mode == CSV_AMENDER) {
        for (size_t i = 0; i <= csvr->n_fields_alloc; i++) {
            csvr->field_pos[i] = 0;
        }
    }
    if (csvr->n_fields_alloc) {
        csvr->field_pos[0] = start;
    }

    if (mode == CSV_WRITER || mode == CSV_AMENDER) {
        for (size_t i = 0; i < csvr->n_fields_alloc; i++) {
            csvr->fields[i] = NULL;
        }
    }

    csvr->n_fields = 0;
}

void CSVRecord_del(CSVRecord * csvr) {
    if (csvr->field_pos) {
        IO_FREE(csvr->field_pos);
    }
    if (csvr->fields) {
        IO_FREE(csvr->fields);
    }
    IO_FREE(csvr);
}

// only use in CSV_READER mode or when adding a new record, otherwise do not use in CSV_AMENDER mode
enum csv_status CSVRecord_append_field_pos(CSVRecord * csvr, size_t pos) {
    if (csvr->n_fields == csvr->n_fields_alloc) {
        bool result;
        RESIZE_REALLOC(result, size_t, csvr->field_pos, csvr->n_fields_alloc*RESIZE_SCALE + 1)
        if (result) {
            csvr->n_fields_alloc *= RESIZE_SCALE;
            for (size_t i = csvr->n_fields; i < csvr->n_fields_alloc; i++) {
                csvr->field_pos[i+1] = 0;
            }
        } else {
            return CSV_MEMORY_ERROR;
        }
    }
    csvr->field_pos[++csvr->n_fields] = pos;
    return CSV_SUCCESS;
}

CSVFile * CSVFile_new(char * filename, char mode, bool has_header, char * line_ending, char * file_out) {
    if (!(mode == CSV_READER || mode == CSV_WRITER || mode == CSV_AMENDER)) {
        goto failed_mode;
    }
    CSVFile * new_csv = (CSVFile *) IO_MALLOC(sizeof(CSVFile));
    if (!new_csv) {
        goto failed_csvfile_alloc;
    }

    new_csv->n_records_alloc = DEFAULT_N_RECORDS;

    // available for READER/WRITER/AMENDER
    new_csv->records = (CSVRecord **) IO_MALLOC(sizeof(CSVRecord *) * new_csv->n_records_alloc);
    if (!new_csv->records) {
        goto failed_records_alloc;
    }
    for (size_t i = 0; i < new_csv->n_records_alloc; i++) {
        new_csv->records[i] = NULL;
    }

    if (!line_ending) {
        line_ending = DEFAULT_LINE_ENDING;
    }

    CSVFile_init(new_csv, filename, mode, has_header, line_ending, file_out);
    if (!new_csv->handle) {
        goto failed_init;
    }

    return new_csv;
   
failed_init:
    IO_FREE(new_csv->records);

failed_records_alloc:
    IO_FREE(new_csv);

failed_csvfile_alloc:
failed_mode:
    return NULL;
    
}

void CSVFile_init(CSVFile * csv, char * filename, char mode, bool has_header, char * line_ending, char * file_out) {
    csv->handle = NULL;
    csv->handle_file_out = NULL;
    csv->filename = filename;
    csv->file_out = file_out;
    if (mode == CSV_READER) {
        csv->handle = fopen(filename, "rb");
        if (!csv->handle) {
            return;
        }
        file_out = NULL;
    } else if (mode == CSV_WRITER) {
        csv->handle = fopen(filename, "wb");
        if (!csv->handle) {
            return;
        }
        csv->handle_file_out = NULL;
    } else if (mode == CSV_AMENDER) {
        if (file_out) {
            csv->handle = fopen(filename, "rb");
            if (!csv->handle) {
                return;
            }
            csv->handle_file_out = fopen(file_out, "wb");
            if (!csv->handle_file_out) {
                fclose(csv->handle);
                csv->handle = NULL;
                return;
            }
        } else {
            csv->handle = fopen(filename, "rb+");
            csv->handle_file_out = NULL;
        }
    }

    csv->line_ending = line_ending;
    csv->line_ending_size = strlen(line_ending);
    csv->mode = mode;
    csv->has_header = has_header;
    csv->n_records = 0;

    if (mode == CSV_READER || mode == CSV_AMENDER) {
        CSVFile_read(csv);
    }
}

enum csv_status CSVFile_append_record(CSVFile * csv, size_t pos) {
    if (csv->n_records == csv->n_records_alloc) {
        int res = true;
        RESIZE_REALLOC(res, CSVRecord *, csv->records, csv->n_records_alloc * RESIZE_SCALE)
        if (!res) {
            return CSV_MEMORY_ERROR;
        }
        csv->n_records_alloc *= RESIZE_SCALE;
    }
    csv->records[csv->n_records] = CSVRecord_new(csv->mode, pos, 0);
    if (!csv->records[csv->n_records]) {
        return CSV_MEMORY_ERROR;
    }
    
    csv->n_records++;
    return CSV_SUCCESS;
}

CSVRecord * CSVFile_pop_record(CSVFile * csv) {
    if (!csv->n_records) {
        return NULL;
    }
    CSVRecord * out = csv->records[csv->n_records-1];
    csv->records[--csv->n_records] = NULL;
    return out;
}

static enum reader_states sm_get_next_state(CSVFile * csv, int state) {
    int ch = fgetc(csv->handle);
    switch (state) {
        case IN_FIELD: {
            if (ch == ',') {
                return END_FIELD;
            } else if (ch == csv->line_ending[0]) {
                size_t ct = 1;
                // need to test that the fgetc is not executed when ct == csv->line_ending_size
                while (ct < csv->line_ending_size && (ch = fgetc(csv->handle)) == csv->line_ending[ct]) {
                    ct++;
                }
                if (ct != csv->line_ending_size) { // failed to find line_ending
                    fseek(csv->handle, -1, SEEK_CUR); // move back one since we fgetc'd a character that was not a line_ending
                    return IN_FIELD;
                }
                return END_RECORD;
            } else if (ch == EOF) {
                return END_CSV;
            } else if (ch == '"') { // malformed csv
                #ifndef NDEBUG
                printf("\nmalformed csv file, double quotes in unquoted string cell at %ld", ftell(csv->handle));
                #endif
                return FAILURE;
            }
            return IN_FIELD;
        }
        case END_FIELD: {
            if (ch == '"') {
                return IN_QUOTES;
            } else if (ch == ',') {
                return END_FIELD;
            } else if (ch == csv->line_ending[0]) {
                size_t ct = 1;
                // need to test that the fgetc is not executed when ct == csv->line_ending_size
                while (ct < csv->line_ending_size && (ch = fgetc(csv->handle)) == csv->line_ending[ct]) {
                    ct++;
                }
                if (ct != csv->line_ending_size) { // failed to find line_ending
                    if (ch == '"') {
                        return IN_QUOTES;
                    } else if (ch == ',') {
                        return END_FIELD;
                    }
                    return IN_FIELD;
                }
                return END_RECORD;
            } else if (ch == EOF) {
                return END_CSV;
            }
            return IN_FIELD;
        }
        case IN_QUOTES: {
            if (ch == '"') {
                return ESCAPING_QUOTES;
            } else if (ch == EOF) { // malformed csv EOF within field
                #ifndef NDEBUG
                printf("\nmalformed csv file, tried to end file within quotes at %ld", ftell(csv->handle));
                #endif
                return FAILURE;
            }
            return IN_QUOTES;
        }
        case ESCAPING_QUOTES: {
            if (ch == ',') {
                return END_FIELD;
            } else if (ch == '"') {
                return IN_QUOTES;
            } else if (ch == csv->line_ending[0]) {
                size_t ct = 1;
                // need to test that the fgetc is not executed when ct == csv->line_ending_size
                while (ct < csv->line_ending_size && (ch = fgetc(csv->handle)) == csv->line_ending[ct]) {
                    ct++;
                }
                if (ct != csv->line_ending_size) { // failed to find line_ending, this cannot actually happen in a well-formed csv file
                    #ifndef NDEBUG
                    printf("\nmalformed csv file, invalid character outside of field (partial line-ending) at %ld", ftell(csv->handle));
                    #endif
                    return FAILURE;
                }
                return END_RECORD;
            } else if (ch == EOF) {
                return END_CSV;
            }
            #ifndef NDEBUG
            printf("\nmalformed csv file, invalid character after double quotes at %ld", ftell(csv->handle));
            #endif
            return FAILURE; // any other condition than the 4 above is a malformed csv
        }
        case END_RECORD: {
            if (ch == '"') {
                return IN_QUOTES;
            } else if (ch == ',') {
                return END_FIELD;
            } else if (ch == EOF) {
                return END_CSV;
            }
            return IN_FIELD; // any other character after these should indicate a new field
        }
        case END_CSV: {
            return END_CSV;
        }
        case UNINITIALIZED: {
            if (ch == '"') {
                return IN_QUOTES;
            }
            return IN_FIELD;
        }
        case FAILURE: {
            // fall through
        }
        default: {
            // do nothing, will return failure
        }
    }

    return FAILURE;
}

enum csv_status CSVFile_read(CSVFile * csv) {
    //printf("\nreading file %s", csv->filename);
    enum reader_states state = UNINITIALIZED;
    int res = CSVFile_append_record(csv, 0);
    if (res) {
        return res;
    }
    while (state != END_CSV) {
        state = sm_get_next_state(csv, state);
        switch (state) {
            case END_FIELD: {
                // record a new field position
                // use for CSV_READER only. TODO: make case for CSV_APPENDER
                //printf("\ncompleted field at %zu", ftell(csv->handle));
                if ((res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], ftell(csv->handle)))) {
                    return res;
                }
                break;
            }
            case END_RECORD: {
                // end the last field and record a new record
                //printf("\ncompleted record at %zu", ftell(csv->handle));
                size_t field_end = ftell(csv->handle) - csv->line_ending_size + 1;
                // use for CSV_READER only. TODO: make case for CSV_APPENDER
                if ((res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], field_end))) {
                    return res;
                }
                if ((res = CSVFile_append_record(csv, field_end + csv->line_ending_size - 1))) {
                    return res;
                }
                break;
            }
            case END_CSV: {
                // cleanup records
                //printf("\ncompleted file at %zu", ftell(csv->handle));
                size_t loc = ftell(csv->handle);
                //if (csv->records[csv->n_records-1]->n_fields) { // if csv has single column/field count, this misses last entry if no line-ending
                if (loc > csv->records[csv->n_records-1]->field_pos[0]) { // add a field if the current cursor is not at the beginning of a record
                    // final record ended without a line ending. TODO: need to check that this actually includes the last character or if it cuts off the last one
                    if ((res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], loc + 1))) {
                        return res;
                    }
                } else {
                    // if last record has zero fields, pop it and destroy. This will happend if final real record ending with a line ending
                    CSVRecord_del(CSVFile_pop_record(csv));
                    csv->records[csv->n_records] = NULL;

                    // if mode is reader, try to free extraneous memory
                }
                if (csv->mode == CSV_READER) {
                    RESIZE_REALLOC(res, CSVRecord *, csv->records, csv->n_records)
                    if (res) {
                        csv->n_records_alloc = csv->n_records;
                    }
                    
                    for (size_t i = 0; i < csv->n_records; i++) {
                        // probably should have a function to hide the ->field_pos member
                        //printf("\nallocation before %zu, number of positions %zu", csv->records[i]->n_fields_alloc+1, csv->records[i]->n_fields+1);
                        RESIZE_REALLOC(res, size_t, csv->records[i]->field_pos, csv->records[i]->n_fields+1)
                        if (res) {
                            csv->records[i]->n_fields_alloc = csv->records[i]->n_fields;
                            //printf("\nallocation after %zu, number of positions %zu", csv->records[i]->n_fields_alloc+1, csv->records[i]->n_fields+1);
                        }
                    }
                    
                }
                break;
            }
            case FAILURE: {
                return CSV_READ_ERROR;
            }
            default: {
                // do nothing
            }
        }
    }
    /*
    printf("csv summary:\n");
    for (size_t irec = 0; irec < csv->n_records; irec++) {
        printf("record %zu: %zu fields found:\n", irec, csv->records[irec]->n_fields);
        for (size_t ifie = 0; ifie < csv->records[irec]->n_fields + 1; ifie++) {
            printf("%zu ", csv->records[irec]->field_pos[ifie]);
        }
        printf("\n");
    }
    */
    return CSV_SUCCESS;
}

int CSVFile_write(CSVFile * csv) {
    // TODO;
    return CSV_SUCCESS;
}

// for builders
/*
int CSVFile_append_record(CSVFile * csv, char ** record, size_t n_fields) {
    // TODO;
    return CSV_SUCCESS;
}

// for builders
int CSVFile_append_field(CSVFile * csv, size_t record, char * format, ...) {
    // TODO;
    return CSV_SUCCESS;
}
*/

enum csv_status CSVFile_set_cell(CSVFile * csv, size_t record, size_t field, char * format, ...) {
    // TODO;
    // sprintf into the cell, realloc'ing memory as needed to ensure (record, field) exist or reallocing the existing field
    return CSV_SUCCESS;
}

// returns '\0' for bypass, otherwise returns cand
// state must be initialized to a negative number
static char strip_quotes(int * state, char cand) {
    /*
    state < 0 // init
    state = 1 // encountered odd number of double quotes
    state = 0 // encountered even number of double quotes

    state transition rules:
    if first character is not a double quote:
        return the string
    else:
        subtract 1
    when encountering all subsequent double quotes:
        if state == 0, record the double quote
        flip the bit on state/set to 1
    */
    
    if (cand == '"') {
        if (!*state) {
            *state = 1;
            return cand;
        } else {
            if (*state < 0) {
                *state = 1;
            } else {
                *state ^= 1;
            }
            return '\0';
        }
    }
    return cand;
}

// use sscanf after some minor pre-formatting
enum csv_status CSVFile_get_cell(CSVFile * csv, size_t record, size_t field, char * format, ...) {
    // TODO;
    // get field at (record, field) by fseek and reading in fgetc until next delimiter in to cell_buffer
    // process by removing extraneous quotes
    // pass to sscanf with format and output values    
    if (record >= csv->n_records || field >= csv->records[record]->n_fields) {
        return CSV_INDEX_ERROR;
    }
    size_t start = csv->records[record]->field_pos[field];// + ((field) ? 1 : 0);
    size_t size = csv->records[record]->field_pos[field+1] - start - 1;
    
    fseek(csv->handle, start, SEEK_SET);
    size_t j = 0;
    char cand = '\0';
    int quote_state = -1;
    for (size_t i = 0; i < size; i++) {
        cand = strip_quotes(&quote_state, fgetc(csv->handle));
        if (cand != '\0') {
            cell_buffer[j++] = cand;
        }
    }
    //cell_buffer[size] = '\0';
    cell_buffer[j] = '\0';
    //printf("start: %zu, size: %zu: %s\n", start, size, cell_buffer);
    va_list arg;
    va_start(arg, format);
    int res = vsscanf(cell_buffer, format, arg);
    va_end(arg);
    if (res == EOF) {
        return CSV_READ_ERROR;
    }
    return CSV_SUCCESS;
}

CSVFileIterator * CSVFileIterator_new(CSVFile * csv, size_t index, enum csv_axis axis, size_t buffer_size) {
    if (!csv) {
        return NULL;
    } else if (axis == CSV_ROW && index >= csv->n_records) {
        return NULL;
    } else if (axis == CSV_COLUMN) {
        // when I can track the maximum number of fields, simplify this case
        size_t max_n_fields = 0;
        for (size_t irec = 0; irec < csv->n_records; irec++) {
            if (csv->records[irec]->n_fields > max_n_fields) {
                max_n_fields = csv->records[irec]->n_fields;
            }
        }
        if (index >= max_n_fields) {
            return NULL;
        }
    }
    CSVFileIterator * csv_iter = (CSVFileIterator *) IO_MALLOC(sizeof(CSVFileIterator));
    if (!csv_iter) {
        return NULL;
    }

    if (!buffer_size) {
        buffer_size = FIELD_BUFFER_SIZE;
    }

    char * buffer = (char *) IO_MALLOC(sizeof(char)*buffer_size);
    if (!buffer) {
        IO_FREE(csv_iter);
        return NULL;
    }

    CSVFileIterator_init(csv_iter, csv, index, axis, buffer, buffer_size);
    csv_iter->buffer_reclaim = true;

    return csv_iter;
}
/*
CSVFileIterator * CSVFileIterator_iter3(CSVFile * csv, size_t index, enum csv_axis axis) {
    return CSVFileIterator_new(csv, index, axis, FIELD_BUFFER_SIZE);
}
*/

void CSVFileIterator_init(CSVFileIterator * csv_iter, CSVFile * csv, size_t index, enum csv_axis axis, char * buffer, size_t buffer_size) {
    csv_iter->csv = csv;
    csv_iter->index = index;
    csv_iter->axis = axis;
    csv_iter->stop = ITERATOR_GO;
    csv_iter->buffer_size = buffer_size;
    csv_iter->next = NULL;
    if (!buffer) {
        if (!buffer_size) {
            buffer_size = FIELD_BUFFER_SIZE;
        }
        buffer = (char*) IO_MALLOC(sizeof(char) * buffer_size);
        if (!buffer) {
            csv_iter->stop = ITERATOR_STOP;
            return;
        }
        csv_iter->buffer_reclaim = true;
    } else {
        csv_iter->buffer_reclaim = false;
    }
    csv_iter->next = buffer;
    for (size_t i = 0; i < csv_iter->buffer_size; i++) {
        csv_iter->next[i] = '\0';
    }

    if (axis == CSV_COLUMN) {
        if (csv->has_header) {
            csv_iter->start = 1;
            csv_iter->axis_index = 1;
        } else {
            csv_iter->start = 0;
            csv_iter->axis_index = 0;
        }
        csv_iter->end = csv->n_records;
    } else {
        csv_iter->start = 0;
        csv_iter->axis_index = 0;
        csv_iter->end = csv->records[csv_iter->index]->n_fields;
    }

    csv_iter->step = 1;
}

void CSVFileIterator_del(CSVFileIterator * csv_iter) {
    IO_FREE(csv_iter->next);
    csv_iter->next = NULL;
    IO_FREE(csv_iter);
}

// NULL means failure or stop condition and NOT an empty field
char * CSVFileIterator_next(CSVFileIterator * csv_iter) {
    if (!csv_iter) {
        return NULL;
    }

    if (csv_iter->axis_index >= csv_iter->end) {
        csv_iter->stop = ITERATOR_STOP;
        return NULL;
    }

    size_t record, field;
    if (csv_iter->axis == CSV_COLUMN) {
        record = csv_iter->axis_index;
        field = csv_iter->index;
    } else {
        record = csv_iter->index;
        field = csv_iter->axis_index;
    }
    csv_iter->axis_index += csv_iter->step;
    size_t size = csv_iter->csv->records[record]->field_pos[field+1] - csv_iter->csv->records[record]->field_pos[field] - 1;

    // realloc if csv_iter->next is too small to receive the field
    int res = false;
    if (size + 1 >= csv_iter->buffer_size) {
        size_t new_size = (size + 1 > 2*csv_iter->buffer_size) ? size + 1 : 2*csv_iter->buffer_size;
        RESIZE_REALLOC(res, char, csv_iter->next, new_size);
        if (!res) {
            return NULL;
        }
        csv_iter->buffer_size = new_size;
    }

    // retrieve the cell
    res = CSVFile_get_cell(csv_iter->csv, record, field, "%s", csv_iter->next);
    if (res) {
        return NULL;
    }

    return csv_iter->next;
}

enum iterator_status CSVFileIterator_stop(CSVFileIterator * csv_iter) {
    if (!csv_iter) {
        return ITERATOR_STOP;
    }
    if (csv_iter->stop == ITERATOR_STOP && csv_iter->buffer_reclaim) {
        IO_FREE(csv_iter->next);
        csv_iter->next = NULL;
        csv_iter->buffer_reclaim = false;
    }
    /*
    if (csv_iter->stop == ITERATOR_STOP) {
        CSVFileIterator_del(csv_iter);
        return ITERATOR_STOP;
    }
    */
    return csv_iter->stop;
}

CSVFileIterator * CSVFile_get_column(CSVFile * csv, size_t icolumn) {
    return CSVFileIterator_new(csv, icolumn, CSV_COLUMN, 0);
}

CSVFileIterator * CSVFile_get_column_slice(CSVFile * csv, size_t icolumn, size_t start, size_t stop, size_t step) {
    CSVFileIterator * out = CSVFile_get_column(csv, icolumn);
    out->start = start;
    out->axis_index = start;
    out->end = stop;
    out->step = step;
    return out;
}

CSVFileIterator *  CSVFile_get_row(CSVFile * csv, size_t irow) {
    return CSVFileIterator_new(csv, irow, CSV_ROW, 0);
}

CSVFileIterator * CSVFile_get_row_slice(CSVFile * csv, size_t irow, size_t start, size_t stop, size_t step) {
    CSVFileIterator * out = CSVFile_get_row(csv, irow);
    out->start = start;
    out->axis_index = start;
    out->end = stop;
    out->step = step;
    return out;
}
/*
CSVFileIterator * CSVFileIteratorIterator_iter(CSVFileIterator * csv_iter) {
    return csv_iter;
}
*/
char * CSVFileIteratorIterator_next(CSVFileIteratorIterator * csv_iter) {
    return CSVFileIterator_next(csv_iter);
}
enum iterator_status CSVFileIteratorIterator_stop(CSVFileIteratorIterator * csv_iter) {
    return CSVFileIterator_stop(csv_iter);
}

void CSVFile_del(CSVFile * csv) {
    fclose(csv->handle);
    if (csv->file_out) {
        fclose(csv->handle_file_out);
    }
    for (size_t i = 0; i < csv->n_records; i++) {
        CSVRecord_del(csv->records[i]);
    }
    IO_FREE(csv->records);
    IO_FREE(csv);
}