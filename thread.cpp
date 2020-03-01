#include "thread.h"

void Thread::start()
{
	if (pthread_create(&m_thread, NULL, &Thread::run_, this) < 0) {
		std::cerr << "ERROR: failed to create thread" << std::endl;
		return;
	}
}

void Thread::join()
{
	pthread_join(m_thread, NULL);
}
