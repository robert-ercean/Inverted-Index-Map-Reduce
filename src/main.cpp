#include <iostream>
#include <pthread.h>
#include "structs.hpp"

int main(int argc, char **argv)
{
    int mappers_count = atoi(argv[1]);
    int reducers_count = atoi(argv[2]);

    InFile in_file(argv[3]);
    try {
        in_file.parse_filenames();
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }

    pthread_t mappers[mappers_count];
    pthread_t reducers[reducers_count];

    

    return 0;
}