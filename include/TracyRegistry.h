#ifndef TRACYREGISTRY_H_
#define TRACYREGISTRY_H_

#include <iostream>
#include <fstream>
#include <map>
#include <list>
#include <thread>
#include "Tracy.h"

using namespace std;
namespace TRACY
{
	typedef list<TracyThread*> TracyThreadCtr;
	typedef TracyThreadCtr::iterator TracyThreadCtrIt;

	//const unsigned int BUFSIZE = 1000;

	class TracyRegistry
	{
	public:
        static TracyRegistry* instance()
        {
            if (instance_ == NULL)
            {
                std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
                lck.lock();

                if (instance_ == NULL)
                    instance_ = new TracyRegistry();
                lck.unlock();
            }
            return instance_;
        }
	private:
		static TracyRegistry* instance_;
		static std::mutex mtx;
		std::ofstream stream_;
		static TracyThreadCtr* pThreadCtr_;
		thread* theThread_;

        TracyRegistry()
        {
            char fileName[BUFSIZE];

            sprintf_s(fileName, BUFSIZE, "tracy_registry");
            stream_.open(fileName, std::ios::out | std::ios::trunc);

            pThreadCtr_ = new TracyThreadCtr();
            pThreadCtr_->clear();

            //std::thread t1(doCycle);
            theThread_ = new std::thread(doCycle);
            //theThread_.resume();
        }
        ~TracyRegistry()
        {
            if (stream_.is_open())
            {
                stream_.close();
            }
        }
	public:
		       
        void add(TracyThread* thread)
        {
            pThreadCtr_->push_back(thread);
        }

        inline static void flush()
        {
            TracyThreadCtrIt i, end = pThreadCtr_->end();
            for (i = pThreadCtr_->begin(); i != end; ++i)
            {
                (*i)->flush(false, true);
            }
        }
       inline static void doCycle()
        {
            printf("### STARTING doCycle() \n");
            unsigned int timeout = 100 + (rand() % 1000);

            while (true)
            {
                flush();
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
            }
        }
       
	};
}
#endif
