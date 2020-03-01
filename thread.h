#ifndef _THREAD_H
#define _THREAD_H

#include <iostream>
#include <pthread.h>

class Thread {
public:
	Thread() : m_thread(0) {}

	void start();
	void join();
private:
	virtual void run() {}
	static void* run_(void* p) {
		((Thread*)p)->run();
   	}

	pthread_t m_thread;
};

#endif
