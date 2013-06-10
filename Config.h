#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <vector>
using namespace std;
class Interface;
class Config
{
    public:
        static uint32_t routerId;
        static vector<Interface*> inters;
};

#endif // CONFIG_H
