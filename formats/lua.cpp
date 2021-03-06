#include "lua.h"

#include <cstdint>
#include <cstring>


const unsigned char Lua::header_magic[] = { 0x1B, 0x4C, 0x75, 0x61 };


// copied from an old code, need to refactor someday
void Lua::FunctionParser()
{

    uint32_t i;
    char *oldp;

    // Function header
    i = *(uint32_t *)p_read;
    p_read += i + 4;

    // remove Source name, Line defined and last line defined
    memset(p_write, 0, 12);
    *(uint32_t *)(p_write + 12) = *(uint32_t *)(p_read + 8);
    p_write += 0x10;
    p_read += 0xC;


    // Instruction list
    i = *(uint32_t *)p_read * 4 + 4;
    memmove(p_write, p_read, i);
    p_write += i;
    p_read += i;


    oldp = p_read;
    // Constant list
    i = *(uint32_t *)p_read;
    p_read += 4;
    while (i--)
    {
        switch (*p_read++)
        {
        // 1=LUA_TBOOLEAN
        case 1:
            p_read++;
            break;
        // 3=LUA_TNUMBER
        case 3:
            p_read += 8;
            break;
        // 4=LUA_TSTRING
        case 4:
            p_read += *(uint32_t *)p_read + 4;
        }
    }


    // Function prototype list
    i = *(uint32_t *)p_read;
    p_read += 4;

    memmove(p_write, oldp, p_read - oldp);
    p_write += p_read - oldp;
    while (i--)
    {
        FunctionParser();
    }



    // Source line position list (optional debug data)
    p_read += *((uint32_t *)p_read) * 4 + 4;



    // Local list (optional debug data)
    i = *(uint32_t *)p_read;
    p_read += 4;
    while (i--)
    {
        p_read += *(uint32_t *)p_read + 12;
    }

    // Upvalue list (optional debug data)
    i = *(uint32_t *)p_read;
    p_read += 4;
    while (i--)
    {
        p_read += *(uint32_t *)p_read + 4;
    }

    // strip above optional debug data
    memset(p_write, 0, 12);
    p_write += 12;


}


size_t Lua::Leanify(size_t size_leanified /*= 0*/)
{
    // skip header
    p_read += 0xC;
    p_write = p_read - size_leanified;
    fp -= size_leanified;
    FunctionParser();

    return p_write - fp;
}