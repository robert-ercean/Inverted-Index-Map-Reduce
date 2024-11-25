#pragma once

#include <pthread.h>
#include <atomic>

class AbstractThread {
public:
    AbstractThread() {}
    virtual ~AbstractThread() {}
    bool startInternalThread() {
        return (pthread_create(&internalThread, NULL, InternalThreadFuncWrapper, this) == 0);
    }
    void WaitForInternalThreadToExit() {
        pthread_join(internalThread, NULL);
    }

private:
    pthread_t internalThread;
    
    static void *InternalThreadFuncWrapper(void * ThreadObj) {
        ((AbstractThread *)ThreadObj)->InternalThreadFunc();
        return NULL;
    }

protected:
    virtual void InternalThreadFunc() = 0;
};
