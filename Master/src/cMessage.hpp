//
// Created by songyh on 2021/11/29.
//

#ifndef CMESSAGE_H
#define CMESSAGE_H

#include <stdint.h>
#include <string>

using namespace std;
class cMessage{
public:
    cMessage(uint32_t num, string key, uint32_t write){
        this->num = num;
        this->key = key;
        this->write = write;
    }
    uint32_t num = 0;
    string key = "1";
    uint32_t write = 1;
    
};


#endif //CHATROOM_CMESSAGE_H
