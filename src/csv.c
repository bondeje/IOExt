#include <stdio.h>
#include "csv.h"

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
static enum reader_states {
    FAILURE = -1,
    IN_FIELD,
    END_FIELD,
    IN_QUOTES,
    ESCAPING_QUOTES,
    END_RECORD,
    END_CSV
};

static char cell_buffer[CSV_CELL_BUFFER_SIZE] = {'\0'};

CSVRecord * CSVRecord_new(char mode, size_t init_field_alloc) {
    if (!init_field_alloc) {
        init_field_alloc = DEFAULT_N_FIELDS;
    }
    CSVRecord * new_record = (CSVRecord * ) CSV_MALLOC(sizeof(CSVRecord));
    if (!new_record) {
        goto failed_csvrecord_alloc;
    }

    if (mode == CSV_READER || mode == CSV_AMENDER) {
        new_record->field_pos = (size_t *) CSV_MALLOC(sizeof(size_t) * (init_field_alloc + 1));
        if (!new_record->field_pos) {
            goto failed_field_pos_alloc;
        }
    } else {
        new_record->field_pos = NULL;
    }

    if (mode == CSV_WRITER || mode == CSV_AMENDER) {
        new_record->fields = (char **) CSV_MALLOC(sizeof(size_t) * (init_field_alloc));
        if (!new_record->fields) {
            goto failed_fields_alloc;
        }
    } else {
        new_record->fields = NULL;
    }

    CSVRecord_init(new_record, mode, init_field_alloc);

    return new_record;

failed_fields_alloc:
    if (new_record->field_pos) {
        CSV_FREE(new_record->field_pos);
    }
failed_field_pos_alloc:
    CSV_FREE(new_record);
failed_csvrecord_alloc:
    return NULL;
}

void CSVRecord_init(CSVRecord * csvr, char mode, size_t init_field_alloc) {
    if (!init_field_alloc) {
        csvr->_n_fields_alloc = DEFAULT_N_FIELDS;
    } else {
        csvr->_n_fields_alloc = init_field_alloc;
    }
    if (mode == CSV_READER || mode == CSV_AMENDER) {
        for (size_t i = 0; i <= csvr->_n_fields_alloc; i++) {
            csvr->field_pos[i] = 0;
        }
    }

    if (mode == CSV_WRITER || mode == CSV_AMENDER) {
        for (size_t i = 0; i < csvr->_n_fields_alloc; i++) {
            csvr->fields[i] = NULL;
        }
    }

    csvr->n_fields = 0;
}

void CSVRecord_del(CSVRecord * csvr) {
    if (csvr->field_pos) {
        CSV_FREE(csvr->field_pos);
    }
    if (csvr->fields) {
        CSV_FREE(csvr->fields);
    }
    CSV_FREE(csvr);
}

// only use in CSV_READER mode or when adding a new record, otherwise do not use in CSV_AMENDER mode
int CSVRecord_append_field_pos(CSVRecord * csvr, size_t pos) {
    if (csvr->n_fields == csvr->_n_fields_alloc + 1) {
        bool result;
        RESIZE_REALLOC(result, csvr->field_pos, size_t, csvr->_n_fields_alloc*RESIZE_SCALE + 1)
        if (result) {
            /*
            if (csvr->fields) {
                RESIZE_REALLOC(result, csvr->fields, char *, csvr->_n_fields_alloc*RESIZE_SCALE)
                if (result) {
                    csvr->_n_fields_alloc = csvr->_n_fields_alloc*RESIZE_SCALE;
                    csvr->field_pos[csvr->n_fields] = pos;
                    for (size_t i = csvr->n_fields; i < csvr->_n_fields_alloc; i++) {
                        csvr->fields[i] = NULL;
                        csvr->field_pos[i+1] = 0;
                    }
                } else {
                    RESIZE_REALLOC(result, csvr->field_pos, size_t, csvr->_n_fields_alloc + 1)
                    return CSV_FAILURE;
                }
            } else {
                csvr->_n_fields_alloc = csvr->_n_fields_alloc*RESIZE_SCALE;
                csvr->field_pos[csvr->n_fields] = pos;
                for (size_t i = csvr->n_fields; i < csvr->_n_fields_alloc; i++) {
                    csvr->field_pos[i+1] = 0;
                }
            }
            */
            csvr->_n_fields_alloc = csvr->_n_fields_alloc*RESIZE_SCALE;
            csvr->field_pos[csvr->n_fields] = pos;
            for (size_t i = csvr->n_fields; i < csvr->_n_fields_alloc; i++) {
                csvr->field_pos[i+1] = 0;
            }
            csvr->n_fields++;
        } else {
            return CSV_MEMORY_ERROR;
        }
    }
    return CSV_SUCCESS;
}

CSVFile * CSVFile_new(char * filename, char mode, char * line_ending, char * file_out) {
    if (!(mode == CSV_READER || mode == CSV_WRITER || mode == CSV_AMENDER)) {
        goto failed_mode;
    }
    CSVFile * new_csv = (CSVFile *) CSV_MALLOC(sizeof(CSVFile));
    if (!new_csv) {
        goto failed_csvfile_alloc;
    }

    new_csv->_n_records_alloc = DEFAULT_N_RECORDS;

    // available for READER/WRITER/AMENDER
    new_csv->records = (CSVRecord **) CSV_MALLOC(sizeof(CSVRecord *) * new_csv->_n_records_alloc);
    if (!new_csv->records) {
        goto failed_records_alloc;
    }

    if (!line_ending) {
        line_ending = DEFAULT_LINE_ENDING;
    }

    CSVFile_init(new_csv, filename, mode, line_ending, file_out);
    if (!new_csv->_handle) {
        goto failed_init;
    }

    return new_csv;

failed_init:
    CSV_FREE(new_csv->records);

failed_records_alloc:
    CSV_FREE(new_csv);

failed_csvfile_alloc:
failed_mode:
    return NULL;
}

void CSVFile_init(CSVFile * csv, char * filename, char mode, char * line_ending, char * file_out) {
    if (mode == CSV_READER) {
        csv->_handle = fopen(filename, "rb");
        if (!csv->_handle) {
            return;
        }
        file_out = NULL;
        csv->_handle_file_out = NULL;
    } else if (mode == CSV_WRITER) {
        csv->_handle = fopen(filename, "wb");
        if (!csv->_handle) {
            return;
        }
        file_out = NULL;
        csv->_handle_file_out = NULL;
    } else if (mode == CSV_AMENDER) {
        if (file_out) {
            csv->_handle = fopen(filename, "rb");
            if (!csv->_handle) {
                return;
            }
            csv->_handle_file_out = fopen(file_out, "wb");
            if (!csv->_handle_file_out) {
                fclose(csv->_handle);
                csv->_handle = NULL;
                return;
            }
        } else {
            csv->_handle = fopen(filename, "rb+");
            csv->_handle_file_out = NULL;
        }
    }

    csv->filename = filename;
    csv->line_ending = line_ending;
    csv->line_ending_size = strlen(line_ending);
    csv->mode = mode;
    csv->n_records = 0;

    if (mode == CSV_READER || mode == CSV_AMENDER) {
        CSVFile_read(csv);
    }
}

int CSVFile_append_record(CSVFile * csv, size_t pos) {
    csv->records[csv->n_records] = CSVRecord_new(csv->mode, 0);
    if (!csv->records[csv->n_records]) {
        return CSV_MEMORY_ERROR;
    }
    
    // note that despite this next function being only appropriate for CSV_READER, when adding a new record, it is OK
    if (CSVRecord_append_field_pos(csv->records[csv->n_records], pos) == CSV_FAILURE) {
        CSVRecord_del(csv->records[csv->n_records]);
        return CSV_MEMORY_ERROR;
    }
    csv->n_records++;
    return CSV_SUCCESS;
}

CSVRecord * CSVFile_pop_record(CSVFile * csv) {
    return csv->records[--csv->n_records];
}

static int sm_get_next_state(CSVFile * csv, int state) {
    int ch = fgetc(csv->_handle);
    switch (state) {
        case IN_FIELD: {
            if (ch == ',') {
                return END_FIELD;
            } else if (ch == csv->line_ending[0]) {
                size_t ct = 1;
                // need to test that the fgetc is not executed when ct == csv->line_ending_size
                while (ct < csv->line_ending_size && (ch = fgetc(csv->_handle)) == csv->line_ending[ct]) {
                    ct++;
                }
                if (ct != csv->line_ending_size) { // failed to find line_ending
                    fseek(csv->_handle, -1, SEEK_CUR); // move back one since we fgetc'd a character that was not a line_ending
                    return IN_FIELD;
                }
                return END_RECORD;
            } else if (ch == EOF) {
                return END_CSV;
            } else if (ch == '"') { // malformed csv
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
                while (ct < csv->line_ending_size && (ch = fgetc(csv->_handle)) == csv->line_ending[ct]) {
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
                while (ct < csv->line_ending_size && (ch = fgetc(csv->_handle)) == csv->line_ending[ct]) {
                    ct++;
                }
                if (ct != csv->line_ending_size) { // failed to find line_ending, this cannot actually happend in a well-formed csv file
                    return FAILURE;
                }
                return END_RECORD;
            } else if (ch == EOF) {
                return END_CSV;
            }
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
        case FAILURE: {
            // fall through
        }
        default: {
            // do nothing, will return failure
        }
    }

    return FAILURE;
}

int CSVFile_read(CSVFile * csv) {
    int state = IN_FIELD;
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
                if (res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], ftell(csv->_handle))) {
                    return res;
                }
                break;
            }
            case END_RECORD: {
                // end the last field and record a new record
                size_t field_end = ftell(csv->_handle) - csv->line_ending_size + 1;
                // use for CSV_READER only. TODO: make case for CSV_APPENDER
                if (res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], field_end)) {
                    return res;
                }
                if (res = CSVFile_append_record(csv, field_end + csv->line_ending_size)) {
                    return res;
                }
                break;
            }
            case END_CSV: {
                // cleanup records
                
                if (csv->records[csv->n_records-1]->n_fields) {
                    // final record ended without a line ending. TODO: need to check that this actually includes the last character or if it cuts off the last one
                    if (res = CSVRecord_append_field_pos(csv->records[csv->n_records-1], ftell(csv->_handle))) {
                        return res;
                    }
                } else {
                    // if last record has zero fields, pop it and destroy. This will happend if final real record ending with a line ending
                    CSVRecord_del(CSVFile_pop_record(csv));

                    // if mode is reader, try to free extraneous memory
                    if (csv->mode == CSV_READER) {
                        RESIZE_REALLOC(res, csv->records, CSVRecord*, csv->n_records)
                        if (res) {
                            csv->_n_records_alloc = csv->n_records;
                        }
                        for (size_t i = 0; i < csv->n_records; i++) {
                            // probably should have a function to hide the ->field_pos member
                            RESIZE_REALLOC(res, csv->records[i]->field_pos, size_t, csv->records[i]->n_fields+1)
                            if (res) {
                                csv->records[i]->_n_fields_alloc = csv->records[i]->n_fields;
                            }
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
    return CSV_SUCCESS;
}

int CSVFile_write(CSVFile * csv) {
    // TODO;
    return CSV_SUCCESS;
}

// for builders
int CSVFile_append_record(CSVFile * csv, char ** record, size_t n_fields) {
    // TODO;
    return CSV_SUCCESS;
}

// for builders
int CSVFile_append_field(CSVFile * csv, size_t record, char * format, ...) {
    // TODO;
    return CSV_SUCCESS;
}

int CSVFile_set_cell(CSVFile * csv, size_t record, size_t field, char * format, ...) {
    // TODO;
    // sprintf into the cell, realloc'ing memory as needed to ensure (record, field) exist or reallocing the existing field
    return CSV_SUCCESS;
}

// use sscanf after some minor pre-formatting
int CSVFile_get_cell(CSVFile * csv, size_t record, size_t field, char * format, ...) {
    // TODO;
    // get field at (record, field) by fseek and reading in fgetc until next delimiter in to cell_buffer
    // process by removing extraneous quotes
    // pass to sscanf with format and output values
    if (record >= csv->n_records || field >= csv->records[record]->n_fields) {
        return CSV_INDEX_ERROR;
    }
    size_t start = csv->records[record]->field_pos[field] + ((field) ? 1 : 0);
    size_t size = csv->records[record]->field_pos[field+1] - start;
    fseek(csv->_handle, start, SEEK_SET);
    for (size_t i = 0; i < size; i++) {
        cell_buffer[i] = fgetc(csv->_handle);
    }
    cell_buffer[size] = '\0';
    va_list arg;
    va_start(arg, format);
    int res = vsscanf(cell_buffer, format, arg);
    va_end(arg);
    if (res == EOF) {
        return CSV_READ_ERROR;
    }
    return CSV_SUCCESS;
}

int CSVFile_get_column(char ** column, CSVFile * csv, size_t icolumn) {
    return CSV_SUCCESS;
}

int CSVFile_get_row(char ** row, CSVFile * csv, size_t irow) {
    return CSV_SUCCESS;
}

// should be internal/static in csv.c
// applies only to reader
static char * CSVFile_remove_quotes(CSVFile * csv, char * cell_string) {
    // read value in cell (record, field) and remove any outer quotes if able (cannot for escaped sequences)
    return NULL;
}

void CSVFile_del(CSVFile * csv) {
    fclose(csv->_handle);
    if (csv->file_out) {
        fclose(csv->_handle_file_out);
    }
    for (size_t i = 0; i < csv->n_records; i++) {
        CSVRecord_del(csv->records[i]);
    }
    CSV_FREE(csv->records);
    CSV_FREE(csv);
}