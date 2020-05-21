#include "MapperCNROM.h"
#include "Log.h"

namespace sn
{
    MapperCNROM::MapperCNROM(Cartridge &cart) :
        Mapper(cart, Mapper::CNROM),
        m_selectCHR(0)
    {TRACY_VFUNC(L5,67);
        if (cart.getROM().size() == 0x4000) //1 bank
        {
            m_oneBank = true;
        }
        else //2 banks
        {
            m_oneBank = false;
        }
    }

    Byte MapperCNROM::readPRG(Address addr)
    {TRACY_VFUNC(L5,68);
        if (!m_oneBank)
            return m_cartridge.getROM()[addr - 0x8000];
        else //mirrored
            return m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
    }

    void MapperCNROM::writePRG(Address addr, Byte value)
    {TRACY_VFUNC(L5,69);
        m_selectCHR = value & 0x3;
    }

    const Byte* MapperCNROM::getPagePtr(Address addr)
    {TRACY_VFUNC(L5,70);
        if (!m_oneBank)
            return &m_cartridge.getROM()[addr - 0x8000];
        else //mirrored
            return &m_cartridge.getROM()[(addr - 0x8000) & 0x3fff];
    }

    Byte MapperCNROM::readCHR(Address addr)
    {TRACY_VFUNC(L5,71);
        return m_cartridge.getVROM()[addr | (m_selectCHR << 13)];
    }

    void MapperCNROM::writeCHR(Address addr, Byte value)
    {TRACY_VFUNC(L5,72);
        LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
    }
}
