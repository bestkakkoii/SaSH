#ifndef GAMEGLOBAL_H_
#define GAMEGLOBAL_H_

namespace game
{
    enum AssmblyCode
        : unsigned char
    {
        Nop = 0x90,
        Jmp = 0xE9,
        JmpShort = 0xEB,
        Call = 0xE8,
        Int3 = 0xCC,
    };

    constexpr unsigned int kMaxMACLength = 18 + 1;
}

#endif // GAMEGLOBAL_H_