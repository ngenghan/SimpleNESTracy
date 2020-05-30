#ifndef __TRACY_H
#define __TRACY_H

#if defined(__cplusplus)

#include <windows.h>
#include <fstream>
#include <mutex>  
#include "Log.h"
#include <vector>
#include <stdio.h>

using namespace std;

typedef std::vector<string> FileCtr;


struct TRACY_VOID	{bool val;};
enum TRACY_LEVEL	{L0=0,L1,L2,L3,L4,L5};
enum TRACY_INDENT_BRANCHTYPE	{INDENT_NA=32, INDENT_STARTBRANCH='{', INDENT_MIDBRANCH='|', INDENT_PARAMBRANCH='@',INDENT_ENDBRANCH='}'};

const unsigned int BUFSIZE = 1024;
const unsigned int TRACY_FUNCLEVEL_DEF = L5;
const unsigned int TRACY_PARAMLEVEL_DEF = L5;
const unsigned int BUFSET = 10 * 1024 * BUFSIZE;
const unsigned int MAXFILENAME = 250;
//const unsigned int SIZEHEADER = 3 + sizeof(unsigned long) + (2 * sizeof(unsigned short)) + sizeof(char);
const unsigned int SIZEHEADER = 1 + sizeof(unsigned long) + sizeof(byte) + sizeof(unsigned short);
const unsigned int FilerMgr_BUFCOUNT = 100;
const unsigned int WRITETIMEOUT = 5000;
const unsigned int FILER_MAXFILENUM = 10;
const unsigned int FILER_MAXFILESIZ = 512 * 1024 * 1024;

#define TRACY_PROLOG ::GetTickCount()
//#define TRACY_GETTHREAD Tracy::TrachThread* = Tracy::TracyRegistry::instance()->getTracyThread();

#define TRACY_SET_INSTRUMENT_LEVEL(F,P) \
	TRACY_GETTHREAD \
	tracyThread->setFuncLevel(F); \
	tracyThread->setParamLevel(P);

#define TRACY_VFUNC(level, uId) \
	TRACY::TracyTracer __fnTraceObj(tracyThread, level, uId);


namespace TRACY
{
	class Filer
	{
	private:
		unsigned int numFile_, sizeFile_;
		unsigned int currFileIndex_, currFileSize_;
		FileCtr fileCtr_;
		char fileName_[BUFSIZE];
		char fileNameWorking_[BUFSIZE];

		std::ofstream stream_;

	public:
		Filer(const char* name)
		{
			//Set the init values
			numFile_ = FILER_MAXFILENUM;
			sizeFile_ = FILER_MAXFILESIZ;
			currFileIndex_ = numFile_ - 1;
			currFileSize_ = 0;

			//Set up the fileName to be used
			sprintf_s(fileName_, BUFSIZE, "tracy_%s", name);
			fileName_[MAXFILENAME - 1] = '\0';

			//Init all the fileNum in ctr
			for (unsigned int i = 0; i < numFile_; ++i)
			{
				fileCtr_.push_back("");
			}

			//Setup a new file for recording
			setupNewFile();
		};
		~Filer()
		{
			fileCtr_.clear();
			if (stream_.is_open())
			{
				stream_.close();
			}
		};
		void flush(char* dat, unsigned int count)
		{
			//Writing this data will overflow the file limit
			if ((currFileSize_ + count) > sizeFile_)
			{
				closeCurrFile();
				if (setupNewFile())
				{
					return;
				}
			}
			
			stream_.write(dat, count);
			currFileSize_ += count;
		}
	private:
		void closeCurrFile()
		{
			if (stream_.is_open())
			{
				stream_.close();
			}
		}
		bool setupNewFile()
		{
			bool status = false;
			unsigned int newFileIndex = (currFileIndex_ + 1) % numFile_;

			//Delete the oldFile if to be overriden
			string oldFileName = fileCtr_[newFileIndex];
			//If theres a valid filename 
			if (oldFileName.compare("") != 0)
			{
				//delete the oldFile
				if (remove(oldFileName.c_str()) != 0)
				{
					printf("Error in deleting file");
					return status;
				}
			}

			//generate the newFileName and save it
			sprintf_s(fileNameWorking_, BUFSIZE, "%s_%lu.bin\0", fileName_, TRACY_PROLOG);
			fileCtr_[newFileIndex] = string(fileNameWorking_);

			//update the curr Tracking
			currFileSize_ = 0;
			currFileIndex_ = newFileIndex;

			stream_.open(fileNameWorking_, std::ios::out | std::ios::trunc | std::ios::binary);
			if (!stream_.is_open())
			{
				printf("Error in opening file");
				return status;
			}

			status = true;
			return status;
		}
	};

class FilerMgr
{
private:
	std::mutex mtx;
	unsigned long uniqueTimer_;
	unsigned int randomTimeOut_;
	unsigned int currBuf_;
	unsigned int currBufFlush_;
	unsigned int currBufIndex_[FilerMgr_BUFCOUNT];
	Filer* filer_;
	char buf_[FilerMgr_BUFCOUNT][BUFSET];
	unsigned long lastWrote_;

public:
	FilerMgr()
	:currBuf_(0),lastWrote_(0),currBufFlush_(FilerMgr_BUFCOUNT-1), uniqueTimer_(0)
	{
		randomTimeOut_ = rand()%1000;
		
		memset(&(currBufIndex_[0]),0,FilerMgr_BUFCOUNT*sizeof(unsigned int));
		memset(&(buf_[0][0]),0,BUFSET*FilerMgr_BUFCOUNT);
	}
	~FilerMgr()
	{
		if (filer_)
			delete filer_;
	}
	void init(const char* qName)
	{
		filer_ = new Filer(qName);
	}

	inline void flush(unsigned long timeNow, bool isForced, bool flushOtherBuf)
	{
		static bool isStop = false;
		std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
		lck.lock();
		unsigned int LOCAL_currBuf_ = currBuf_;
		unsigned int LOCAL_currBufFlush = currBufFlush_;
		lck.unlock();

		unsigned int totalToFlush = 0;
		unsigned int bufToFlush = LOCAL_currBuf_;
		if (flushOtherBuf)
		{
			if (((LOCAL_currBufFlush + 1) % FilerMgr_BUFCOUNT) != LOCAL_currBuf_)//if (bufSwapCount_ > 0)
			{
				//bufToFlush = (bufToFlush == 0) ? 1 : 0;
				bufToFlush = (LOCAL_currBufFlush + 1) % FilerMgr_BUFCOUNT;

				if (LOCAL_currBuf_ > bufToFlush)
				{
					totalToFlush = LOCAL_currBuf_ - bufToFlush;
				}
				else
				{
					totalToFlush = FilerMgr_BUFCOUNT - bufToFlush + LOCAL_currBuf_;
				}

				// totalToFlush = (totalToFlush > 3) ? 3 : totalToFlush;
				printf("### Flushing Start: BUF%d total%d [currBuf_=%d currBufFlush=%d]\n", bufToFlush, totalToFlush, LOCAL_currBuf_, LOCAL_currBufFlush);
			}
			else
			{
				return;
			}
		}

		unsigned int lastWrite = 0;
		for (unsigned int i = 0; i < totalToFlush; ++i)
		{
			lastWrite = (bufToFlush + i) % FilerMgr_BUFCOUNT;

			if ((currBufIndex_[lastWrite] > 0))
			{
				printf("### Flushing BUF%d \n", lastWrite);
				filer_->flush(&(buf_[lastWrite][0]), currBufIndex_[lastWrite]);
				currBufIndex_[lastWrite] = 0;
				lastWrote_ = timeNow;
			}
		}

		lck.lock();
		currBufFlush_ = lastWrite;
		lck.unlock();
	}

	inline void print(TRACY_INDENT_BRANCHTYPE type, unsigned short indentLevel, unsigned short uId, char* dat, int datLen)
	{
		//static bool isStop = false;
		std::unique_lock<std::mutex> lck(mtx, std::defer_lock);
		//lck.lock();
		unsigned int LOCAL_currBuf_ = currBuf_;
		unsigned int LOCAL_currBufFlush = currBufFlush_;
		//lck.unlock();
		
		unsigned long timeNow = uniqueTimer_++;
		if ((BUFSET - currBufIndex_[LOCAL_currBuf_]) < (datLen + SIZEHEADER))
		{
			//Next BUF to use
			unsigned int prevbuf = LOCAL_currBuf_;
			LOCAL_currBuf_ = (LOCAL_currBuf_ + 1) % FilerMgr_BUFCOUNT;//currBuf_ = (currBuf_ == 0) ? 1 : 0;

			lck.lock();
			currBuf_ = LOCAL_currBuf_;
			lck.unlock();

			printf("### BUF%d full, using next BUF%d\n", prevbuf, LOCAL_currBuf_);

			if (LOCAL_currBuf_ == LOCAL_currBufFlush)
			{
				printf("### All BUF full. Waiting for flushing [FlushBuf=%d]\n", LOCAL_currBufFlush);

				while (LOCAL_currBuf_ == LOCAL_currBufFlush)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
				printf("### BUF cleared [FlushBuf=%d]\n", LOCAL_currBufFlush);
			}

			currBufIndex_[LOCAL_currBuf_] = 0;
			//bufSwapCount_++;
			// isStop = true;
		}
		
		//if (!isStop)
		{
			//byte buId = (byte)uId;
			byte bindentLevel = (byte)indentLevel;

			buf_[LOCAL_currBuf_][currBufIndex_[LOCAL_currBuf_]++] = '#';
			memcpy(&(buf_[LOCAL_currBuf_][currBufIndex_[LOCAL_currBuf_]]), &uId, sizeof(unsigned short));	currBufIndex_[LOCAL_currBuf_] += sizeof(unsigned short);
			memcpy(&(buf_[LOCAL_currBuf_][currBufIndex_[LOCAL_currBuf_]]), &uniqueTimer_, sizeof(unsigned long));  currBufIndex_[LOCAL_currBuf_] += sizeof(unsigned long);
			memcpy(&(buf_[LOCAL_currBuf_][currBufIndex_[LOCAL_currBuf_]]), &bindentLevel, sizeof(byte));  currBufIndex_[LOCAL_currBuf_] += sizeof(byte);
			//buf_[currBuf_][currBufIndex_[currBuf_]++] = (char)type;
			//buf_[currBuf_][currBufIndex_[currBuf_]++] = '[';
			//if (datLen > 0)
			//{
			//   memcpy(&(buf_[currBuf_][currBufIndex_[currBuf_]]), dat, datLen);	currBufIndex_[currBuf_] += datLen;
			//}
			//buf_[currBuf_][currBufIndex_[currBuf_]++] = ']';
		}
	}
	
};

class TracyThread
{
private:
	FilerMgr FilerMgr_;
	char workingBuf_[BUFSIZE];
	unsigned short funcLevel_;
	unsigned short paramLevel_;
	unsigned short indentLevel_;
public:
	TracyThread(const char* qName)
	:funcLevel_(TRACY_FUNCLEVEL_DEF),
	paramLevel_(TRACY_PARAMLEVEL_DEF),
	indentLevel_(0)
	{
		FilerMgr_.init(qName);

	}
	~TracyThread()
	{
	}
	unsigned short getFuncLevel()	{return funcLevel_;}
	unsigned short getParamLevel()	{return paramLevel_;}
	void setFuncLevel(unsigned short val)	{funcLevel_ = val;}
	void setParamLevel(unsigned short val)	{paramLevel_ = val;}
	void decrIndent()	{indentLevel_--;}
	void incrIndent()	{indentLevel_++;}
	void flush(bool isForced, bool flushOtherBuf)
	{
		unsigned long timeNow = TRACY_PROLOG;
		FilerMgr_.flush(timeNow, isForced, flushOtherBuf);
	}

	inline void printX(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr, ...)
	{
		memset(&(workingBuf_[0]), 0, BUFSIZE);
		va_list argList;
		va_start(argList, ctlStr);
		int increment = vsprintf_s(workingBuf_, BUFSIZE, ctlStr, argList);
		va_end(argList);

		if (increment < 0)
		{
			memset(&(workingBuf_[0]), 0, BUFSIZE);
			sprintf_s(&(workingBuf_[0]), BUFSIZE, "TracError: Err in arg(A)");
		}
		else if ((increment + SIZEHEADER) >= BUFSIZE)
		{
			workingBuf_[BUFSIZE - 1] = '\0';
		}
		FilerMgr_.print(type, indentLevel_, uId, workingBuf_, increment);
	}
	inline void print(TRACY_INDENT_BRANCHTYPE type, unsigned short uId, const char* ctlStr)
	{
		FilerMgr_.print(type, indentLevel_, uId, const_cast<char*>(ctlStr), (int)::strlen(ctlStr));
	}

};

//template <typename T>
class TracyTracer
{
private:
	//char workingBuf_[BUFSIZE];
	unsigned short uId_;
	unsigned short m_level;
	//T* m_pRetVal;
	TracyThread* tracyThread_;
public:
	TracyTracer(TracyThread* tracyThread, unsigned short level, unsigned short uId):
	//m_pRetVal(NULL),
	m_level(level),
	uId_(uId),
	tracyThread_(tracyThread)
	{
		//if(tracyThread_->getFuncLevel() >= m_level)
		{
			tracyThread_->print(INDENT_STARTBRANCH, uId_, "");
			tracyThread_->incrIndent();
		}
	}
	TracyTracer(TracyThread* tracyThread, unsigned short level, unsigned short uId, const char* ctlStr, ...):
	//m_pRetVal(NULL),
	m_level(level),
	uId_(uId),
	tracyThread_(tracyThread)
	{
		//if(tracyThread_->getFuncLevel() >= m_level)
		{
			/*memset(&(workingBuf_[0]),0,BUFSIZE);
			va_list argList;
			va_start(argList, ctlStr);
			int increment = vsprintf_s(workingBuf_, BUFSIZE, ctlStr, argList);
			va_end(argList);

			if(increment < 0)
			{
				memset(&(workingBuf_[0]),0,BUFSIZE);
				sprintf_s(&(workingBuf_[0]),BUFSIZE,"TracError: Err in arg(A)");
			}
			else if((increment + SIZEHEADER) >= BUFSIZE)
			{
				workingBuf_[BUFSIZE-1]='\0';
			}
		
			tracyThread_->print(INDENT_STARTBRANCH, uId_, workingBuf_);
			tracyThread_->incrIndent();*/
		}
	}	
	~TracyTracer()
	{
		//if(tracyThread_->getParamLevel() >= m_level || tracyThread_->getFuncLevel() >= m_level)
		{ 	
			tracyThread_->decrIndent();

			//if(tracyThread->getParamLevel() >= m_level)
			{
				/*if(m_pRetVal)
				{
					tracyThread_->printX(INDENT_ENDBRANCH, uId_, "[0x%x]", Return());
				}*/
			}
		}
	}
	//void SetRetVal(T *retVal)
	//{
	//	if(retVal)
	//	{
	//		//m_pRetVal = retVal;
	//	}
	//}
	/*T Return()
	{
		if(m_pRetVal)
		{
			return *m_pRetVal;
		}
		else
		{
			return T();
		}
	}*/
};
}
#else
#endif
#endif
