#include "structs.h"

using namespace std;

class Mapper : public AbstractThread {
public:
    Mapper(int id, filesControlBlock *fcb) : id(id), fcb(fcb) {}

protected:
    void strip_word(string &word) {
        for (int i = 0; i < (int) word.size(); i++) {
            char c = word[i];
            if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z')) {
                word.erase(i, 1);
                i--;
            }
            word[i] = tolower(word[i]);
        }
    }

    int processFile(int idx) {
        unordered_map<string, int> partialList;
        cout << "Mapper " << id << " is processing file: " << fcb->files[idx] << endl;

        ifstream input("../checker/" + fcb->files[idx]);
        if (!input.is_open()) {
            return 0;
        }
   
        string word;
        while (input >> word) {
            strip_word(word);
            int fileId = idx + 1;
            partialList[word] = fileId;
        }
        /* Now push the newly created partial list to the shared
         * partial list vector in the file control block */
        pthread_mutex_lock(&fcb->partialListsMutex);
        fcb->partialLists.push_back(partialList);
        pthread_mutex_unlock(&fcb->partialListsMutex);

        input.close();
        return 1;
    }

    virtual void InternalThreadFunc() override {
        int static_idx = id;

        /* Process the statically assigned file id first */
        if (static_idx >= (int) fcb->files.size()) {
            cerr << "Static IDX exceeds maximum file number, exiting Mapper with ID: " << id << endl;
            return;
        }
        if (!processFile(static_idx)) {
            cerr << "[STATIC]Mapper failed to open the file with id: " << static_idx << endl;
            return;
        }
        /* If there are anymore files left to 
         * process, fetch their atomic id and start parsing */ 
        while (true) {
            int dynamic_idx = fcb->fileIdx.fetch_add(1);
            
            if (dynamic_idx >= (int) fcb->files.size())
                break;
            
            if (!processFile(dynamic_idx)) {
                cerr << "[DYNAMIC]Mapper failed to open the file with id: " << dynamic_idx << endl;
                return;
            }
        }
        pthread_barrier_wait(&fcb->barrier);
    }
private:
    int id;
    filesControlBlock *fcb;
};