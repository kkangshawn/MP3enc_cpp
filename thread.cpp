/**
 * @file        thread.cpp
 * @version     1.0
 * @brief       MP3enc_cpp thread module source
 * @date        Mar 4, 2020
 * @author      Siwon Kang (kkangshawn@gmail.com)
 */

#include <iostream>
#include "thread.h"

void
Thread::start()
{
    if (!m_is_running) {
        if (pthread_create(&m_thread, NULL, &Thread::run_, this) < 0) {
            std::cerr << "ERROR: failed to create thread" << std::endl;
            return;
        }
        m_is_running = true;
    }
}

void
Thread::join()
{
    if (m_is_running) {
        pthread_join(m_thread, NULL);
        m_is_running = false;
    }
}
