#include <stdio.h>
#include <string.h>
#include "../src/IOExt.h"

int main() {
    FileLineIterator * file = FileLineIterator_new("./data/pascals_triangle.csv", "rb", 0);    
    
    printf("iterating using while loop\n");
    char * line = FileLineIterator_start(file);
    while (!FileLineIterator_stop(file)) {
        printf("%llu, %s", strlen(line), line);
        line = FileLineIterator_next(file);
    }
    
    printf("\n\niterating using for loop\n");
        
    file = FileLineIterator_new("./data/pascals_triangle.csv", "rb", 0);    
    for (char * line = FileLineIterator_start(file); !FileLineIterator_stop(file); line = FileLineIterator_next(file)) {
        printf("%llu, %s", strlen(line), line);
    }

    printf("\n\niterating using for_each\n");
    for_each(char, line, FileLine, "./data/pascals_triangle.csv") {
        printf("%llu, %s", strlen(line), line);
    }

    printf("\n\niterating using for_each with additional constructor arguments\n");
    for_each(char, line2, FileLine, "./data/pascals_triangle.csv", "rb") {
        printf("%llu, %s", strlen(line2), line2);
    }
    

    //FileLineIterator_del(file);
}