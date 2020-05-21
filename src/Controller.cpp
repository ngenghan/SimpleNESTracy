#include "Controller.h"
#include "Log.h"

namespace sn
{
    Controller::Controller() :
        m_keyStates(0),
        m_keyBindings(TotalButtons)
    {TRACY_VFUNC(L5,32);
//         m_keyBindings[A] = sf::Keyboard::J;
//         m_keyBindings[B] = sf::Keyboard::K;
//         m_keyBindings[Select] = sf::Keyboard::RShift;
//         m_keyBindings[Start] = sf::Keyboard::Return;
//         m_keyBindings[Up] = sf::Keyboard::W;
//         m_keyBindings[Down] = sf::Keyboard::S;
//         m_keyBindings[Left] = sf::Keyboard::A;
//         m_keyBindings[Right] = sf::Keyboard::D;
    }

    void Controller::setKeyBindings(const std::vector<sf::Keyboard::Key>& keys)
    {TRACY_VFUNC(L5,33);
        m_keyBindings = keys;
    }

    void Controller::strobe(Byte b)
    {TRACY_VFUNC(L5,34);
        m_strobe = (b & 1);
        if (!m_strobe)
        {
            m_keyStates = 0;
            int shift = 0;
            for (int button = A; button < TotalButtons; ++button)
            {
                m_keyStates |= (sf::Keyboard::isKeyPressed(m_keyBindings[static_cast<Buttons>(button)]) << shift);
                ++shift;
            }
        }
    }

    Byte Controller::read()
    {TRACY_VFUNC(L5,35);
        Byte ret;
        if (m_strobe)
            ret = sf::Keyboard::isKeyPressed(m_keyBindings[A]);
        else
        {
            ret = (m_keyStates & 1);
            m_keyStates >>= 1;
        }
        return ret | 0x40;
    }

}
