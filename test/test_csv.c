#include <stdio.h>
#include "../src/csv.h"

int main() {
    CSVFile * csv = CSVFile_new("./data/csvs/basic_2x3_notermcrlf.csv", CSV_READER, NULL, NULL);
    int val = -1;
    CSVFile_get_cell(csv, 0, 0, "%d", &val);
    printf("0,0: %d", val);

    return 0;
}