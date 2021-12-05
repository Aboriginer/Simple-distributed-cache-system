//
// Created by songyh on 2021/11/29.
//

#ifndef MASTER_H
#define MASTER_H
#include <iostream>
#include <stdint.h>
#include <unordered_map>
#include "cMessage.hpp"
#include <string>
using namespace std;
class Master{
public:
   unordered_map<string , uint32_t> keyCacheMap;
   unordered_map<uint32_t, uint32_t> cacheAddrMap;
   class heartBeatProbe{

   };
   class heartBeatResponse{
       uint32_t cacheServerAddr;
       uint32_t timeFlag;
   };
    uint32_t heartBeatInterval = 30 ;//???
    uint32_t a = 0;
    // cacheServerAddr - timeFlag
public:
    Master(){};

    //virtual void handleMessage();
    //virtual void sendMessage() {};

    uint32_t handleClientReq(cMessage* msg);

    //void handleHeartBeatResponse(cMessage* msg);
    //bool heartBeatDetect(uint32_t cacheServerAddr);
    //void periodicDetectCache();
protected:
    uint32_t getCacheServerAddr(string key);
    uint32_t getCacheServerAddr();

    //bool furtherHeartBeatDetect(uint32_t cacheServerAddr);
    //void updateKeyCacheMapByHeartBeat(uint32_t cacheAddr);


    // heartBeatResponse* msg2heart(cMessage*);

    uint32_t loadBalance();
};


#endif 
