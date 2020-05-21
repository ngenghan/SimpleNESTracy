#include "Log.h"

namespace sn
{
    Log::~Log()
    {TRACY_VFUNC(L5,46);
    }

    Log& Log::get()
    {TRACY_VFUNC(L5,47);
        static Log instance;
        return instance;
    }

    std::ostream& Log::getCpuTraceStream()
    {TRACY_VFUNC(L5,48);
        return *m_cpuTrace;
    }
    std::ostream& Log::getStream()
    {TRACY_VFUNC(L5,49);
        return *m_logStream;
    }

    void Log::setLogStream(std::ostream& stream)
    {TRACY_VFUNC(L5,50);
        m_logStream = &stream;
    }

    void Log::setCpuTraceStream(std::ostream& stream)
    {TRACY_VFUNC(L5,51);
        m_cpuTrace = &stream;
    }

    Log& Log::setLevel(Level level)
    {TRACY_VFUNC(L5,52);
        m_logLevel = level;
        return *this;
    }

    Level Log::getLevel()
    {TRACY_VFUNC(L5,53);
        return m_logLevel;
    }



    TeeBuf::TeeBuf(std::streambuf * sb1, std::streambuf * sb2) :
        m_sb1(sb1),
        m_sb2(sb2)
    {TRACY_VFUNC(L5,54);}
    int TeeBuf::overflow(int c)
    {TRACY_VFUNC(L5,55);
        if (c == EOF)
        {
            return !EOF;
        }
        else
        {
            int const r1 = m_sb1->sputc(c);
            int const r2 = m_sb2->sputc(c);
            return r1 == EOF || r2 == EOF ? EOF : c;
        }
    }

    int TeeBuf::sync()
    {TRACY_VFUNC(L5,56);
        int const r1 = m_sb1->pubsync();
        int const r2 = m_sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }

    TeeStream::TeeStream(std::ostream& o1, std::ostream& o2) :
        std::ostream(&m_tbuf),
        m_tbuf(o1.rdbuf(), o2.rdbuf())
    {TRACY_VFUNC(L5,57);}

}
