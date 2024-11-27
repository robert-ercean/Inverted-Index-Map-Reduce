#include <iostream>
#include <fstream>
#include "structs.h"
#include <unistd.h>
#include "Mapper.h"
#include "Reducer.h"
#include <chrono>
#include <filesystem>

using namespace std;

void parse_filenames(string in_filename, filesControlBlock *fcb) {
    ifstream in("../checker/" + in_filename);
    if (!in.is_open()) {
        in.close();
        throw runtime_error("Err: Input File failed to open: " + in_filename);
    }

    string line;
    getline(in, line);
    int count = 1;
    while (getline(in, line)) {
        file f;
        f.filename = "../checker/" + line;
        filesystem::path p(f.filename);
        f.size = filesystem::file_size(p);
        f.id = count++;
        fcb->filesPq.push(f);
    }
    in.close();
}

void writeFiles(filesControlBlock &fcb) {
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        auto &heap = fcb.heapsf[i];
        /* Write the heap to its file */
        string filename = string(1, 'a' + i) + ".txt";
        ofstream out(filename);
        if (!out.is_open()) {
            cerr << "Error in opening file " << filename << " during writing phase in heapify()" << endl;
            return;
        }
        while(!heap.empty()) {
            entry e = heap.top();
            int arrSize = (int) e.ids.size();
            string line = e.word + ":[";
            for (int i = 0; i < arrSize; i++) {
                line += to_string(e.ids[i]);
                string delim = (i == arrSize - 1) ? "" : " ";
                line += delim;
            }
            line += "]\n";
            out << line;
            heap.pop();
        }
        out.close();
    }
}

int main(int argc, char **argv)
{
    int mappers_count = atoi(argv[1]);
    int reducers_count = atoi(argv[2]);

    vector<Mapper> mappers;
    vector<Reducer> reducers;
    chrono::duration<double>time{0};
    auto start = chrono::high_resolution_clock::now();
    filesControlBlock fcb;
    fcb.heapsf.resize(ALPHABET_SIZE);
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        fcb.heapsf[i] = priority_queue<entry, vector<entry>, decltype(&pqComparator)>(pqComparator);
    }
    fcb.heaps.resize(ALPHABET_SIZE);
    for (int i = 0; i < ALPHABET_SIZE; ++i) {
        fcb.heaps[i].resize(mappers_count);
        for (int j = 0; j < mappers_count; ++j) {
            fcb.heaps[i][j] = priority_queue<entry, vector<entry>, decltype(&pqComparator)>(pqComparator);
        }
    }
    try {
        parse_filenames(argv[3], &fcb);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return -1;
    }


    // Extract files from filesPq (assuming it's populated earlier)
    while (!fcb.filesPq.empty()) {
        fcb.allFiles.push_back(fcb.filesPq.top());
        fcb.filesPq.pop();
    }

    // Sort files in descending order of size
    std::sort(fcb.allFiles.begin(), fcb.allFiles.end(), [](const file& a, const file& b) {
        return a.size > b.size;
    });

    // Initialize Mapper workloads
    std::vector<std::vector<file>> mapperFiles(mappers_count);
    std::vector<intmax_t> mapperWorkloads(mappers_count, 0);

    // Assign files to Mappers to balance workloads
    for (const file& f : fcb.allFiles) {
        // Find the Mapper with the minimum current workload
        auto minIt = std::min_element(mapperWorkloads.begin(), mapperWorkloads.end());
        int mapperIndex = std::distance(mapperWorkloads.begin(), minIt);

        // Assign file to the Mapper
        mapperFiles[mapperIndex].push_back(f);
        mapperWorkloads[mapperIndex] += f.size;
    }

    pthread_barrier_init(&fcb.reduceBarrier, NULL, mappers_count + reducers_count);
    pthread_barrier_init(&fcb.writeBarrier, NULL, reducers_count);
    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers.emplace_back(Reducer(&fcb, i - mappers_count, reducers_count));
            continue;
        }
        mappers.emplace_back(Mapper(&fcb, i, reducers_count, move(mapperFiles[i])));
    }

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            if (!reducers[i - mappers_count].startInternalThread()) {
                cerr << "Starting Reducer Threads failed!" << endl;
                return -1;
            }
            continue;    
        }
        if (!mappers[i].startInternalThread()) {
            cerr << "Starting Mapper Threads failed!" << endl;
            return -1;
        }
    }

    for (int i = 0; i < mappers_count + reducers_count; ++i) {
        if (i >= mappers_count) {
            reducers[i - mappers_count].WaitForInternalThreadToExit();
            continue;
        }
        mappers[i].WaitForInternalThreadToExit();
    }

    pthread_barrier_destroy(&fcb.reduceBarrier);
    pthread_barrier_destroy(&fcb.writeBarrier);

    writeFiles(fcb);

    auto end = chrono::high_resolution_clock::now();
    time += end - start;
    cout << "Time: " << time.count() << endl;
    return 0;
}