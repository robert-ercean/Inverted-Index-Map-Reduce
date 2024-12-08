#pragma once

#include <pthread.h>
#include <atomic>
/**
 * Base class that encapsulates the creation and management of a single thread
 * using the pthread library.
 * 
 * Provides a modular way to start and join a thread.
 *
 * Derived classes must implement the InternalThreadFunc() method, which defines the
 * thread's main execution logic.
 */
class AbstractThread {
public:
    AbstractThread() {}
    virtual ~AbstractThread() {}
     /**
     * Starts the internal thread execution.
     * 
     * This function uses pthread_create to start a new thread that runs the InternalThreadFuncWrapper()
     * which takes a pointer to the current thread instance, casted to a void pointer.
     * 
     * @return true if the thread was successfully created, false if any error occured.
     */
    bool startInternalThread() {
        return (pthread_create(&internalThread, NULL, InternalThreadFuncWrapper, this) == 0);
    }
    /**
     * Waits for the internal thread to stop running its internal function.
     */
    void WaitForInternalThreadToExit() {
        pthread_join(internalThread, NULL);
    }

private:
    pthread_t internalThread;

    /**
     * A static wrapper function that calls the virtual InternalThreadFunc().
     * 
     * It casts the void pointer to an AbstractThread and invokes InternalThreadFunc().
     * 
     * @param ThreadObj A pointer to the AbstractThread instance.
     * @return Always returns NULL after InternalThreadFunc() completes.
     */
    static void *InternalThreadFuncWrapper(void * ThreadObj) {
        ((AbstractThread *)ThreadObj)->InternalThreadFunc();
        return NULL;
    }

protected:
    /**
     * The function that will run in the new thread.
     * 
     * Derived classes must implement this pure virtual function. This is where
     * the thread's main logic resides.
     */
    virtual void InternalThreadFunc() = 0;
};
