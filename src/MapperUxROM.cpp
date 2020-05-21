#include "MapperUxROM.h"
#include "Log.h"

namespace sn
{
    MapperUxROM::MapperUxROM(Cartridge &cart) :
        Mapper(cart, Mapper::UxROM),
        m_selectPRG(0)
    {TRACY_VFUNC(L5,87);
        if (cart.getVROM().size() == 0)
        {
            m_usesCharacterRAM = true;
            m_characterRAM.resize(0x2000);
            LOG(Info) << "Uses character RAM" << std::endl;
        }
        else
            m_usesCharacterRAM = false;

        m_lastBankPtr = &cart.getROM()[cart.getROM().size() - 0x4000]; //last - 16KB
    }

    Byte MapperUxROM::readPRG(Address addr)
    {TRACY_VFUNC(L5,88);
        if (addr < 0xc000)
            return m_cartridge.getROM()[((addr - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
        else
            return *(m_lastBankPtr + (addr & 0x3fff));
    }

    void MapperUxROM::writePRG(Address addr, Byte value)
    {TRACY_VFUNC(L5,89);
        m_selectPRG = value;
    }

    const Byte* MapperUxROM::getPagePtr(Address addr)
    {TRACY_VFUNC(L5,90);
        if (addr < 0xc000)
            return &m_cartridge.getROM()[((addr - 0x8000) & 0x3fff) | (m_selectPRG << 14)];
        else
            return m_lastBankPtr + (addr & 0x3fff);
    }

    Byte MapperUxROM::readCHR(Address addr)
    {TRACY_VFUNC(L5,91);
        if (m_usesCharacterRAM)
            return m_characterRAM[addr];
        else
            return m_cartridge.getVROM()[addr];
    }

    void MapperUxROM::writeCHR(Address addr, Byte value)
    {TRACY_VFUNC(L5,92);
        if (m_usesCharacterRAM)
            m_characterRAM[addr] = value;
        else
            LOG(Info) << "Read-only CHR memory write attempt at " << std::hex << addr << std::endl;
    }
}
