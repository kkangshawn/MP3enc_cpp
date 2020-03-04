/**
 * @file        thread.h
 * @version     0.9
 * @brief       MP3enc_cpp thread module header
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>

/**
 * @class   Thread thread.h "thread.h"
 *
 * @brief   Thread class using pthread library.
 *          This class is supposed to be used by other classes inherit this
 *          , override run() function then call start() and join() from instance.
 */
class Thread {
public:
    Thread() : m_thread(0), m_is_running(false) {}

    /**
     * @fn      void start()
     * @brief   start a thread binding a function specified by run() via pthread_create()
     */
    void start();
    /**
     * @fn      void join()
     * @brief   join a thread binding a function specified by run() via pthread_create()
     */
    void join();
private:
    /**
     * @fn      virtual void run()
     * @brief   A function to bind thread
     */
    virtual void run() {}
    static void* run_(void* p) {
        ((Thread*)p)->run();
        return NULL;
    }

    pthread_t m_thread;         /**< thread handle */
    bool      m_is_running;     /**< state variable to check thread is running */
};

#endif  /* _THREAD_H */
