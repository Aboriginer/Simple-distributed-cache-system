//
// Created by songyh on 2021/11/29.
//

#include "Master.hpp"
#include <iostream>
using namespace std;
//void Master::handleMessage(cMessage* msg) {
//    if(msg->num==1){
//        // 收到 client 请求的的数据包（可能读可能写）
//        handleClientReq(msg);
//    }
//    else if(msg->num==2){
//        // heartbeat detect----heartBeatResponse 收到心跳检测的数据包
//
//        handleHeartBeatResponse(msg);
//    } else if(msg->num==3){
//        //update key-cache server Num 收到cache中的key发生变化的数据包（可能是因为扩缩容等问题引起）
//    }
//
//
//}

uint32_t Master::handleClientReq(cMessage* msg){
    // 收到client的请求包，请求包中需要包含1）执行read/write；2）read/write需要的key
    // read--返回key对应的cache地址；
    // write--通过负载均衡获取key需要写入的cache地址
    cout << "handleClientReq" << endl;
    string clientkey = msg->key;
    bool write = msg->write;
    uint32_t cacheServerAddr = 0;
    if(write){
        //write
        cacheServerAddr = getCacheServerAddr();
        keyCacheMap.insert(pair<string,uint32_t>(clientkey, cacheServerAddr));
        cout << "keyCacheMap[" << clientkey << "]=" << cacheServerAddr << endl;
    } else{
        // read
        cacheServerAddr = getCacheServerAddr(clientkey);
    }
    return cacheServerAddr;
}


uint32_t Master::getCacheServerAddr(string key){
    // get cacheServerNum by key (client read)
   if(keyCacheMap.find(key)==keyCacheMap.end()){
       throw string("cache server has not contain about key");
   }
   return keyCacheMap[key];
}


uint32_t Master::getCacheServerAddr(){
    // get cacheServerNum by load balance (client write)
    uint32_t cacheServerAddr = loadBalance();
    return cacheServerAddr;
}


////==================================================
//void Master::handleHeartBeatResponse(cMessage* msg) {
//    // 心跳检测包处理 
//    heartBeatResponse* heartResp = msg2heart(msg);
//    cacheAddrMap[heartResp->cacheServerAddr] = heartResp->timeFlag;
//}
//
//bool Master::heartBeatDetect(uint32_t cacheServerAddr) {
//    // 对cache server判断是否存活
//    uint32_t timeFlag = 0; // get time=??????????????????????????????????????????????????
//        if (cacheAddrMap[cacheServerAddr] - timeFlag < heartBeatInterval) {
//            return true;
//        }
//    return furtherHeartBeatDetect(uint32_t cacheServerAddr);
//}
//
// void Master::periodicDetectCache(){
//     // 周期性更新本地的cache时间戳的表
// //    cacheAddrMap: cacheServerAddr - timeFlag
//
//    for(auto cacheAddr : cacheAddrMap){
//        uint32_t timeFlag = 0;//??????????????????????????????????????????????????
//        if(heartBeatDetect(cacheAddr.first)){
//            continue;
//        } else{
//            updateKeyCacheMapByHeartBeat(cacheAddr.first);
//        }
//    }
// }
//
// void Master::updateKeyCacheMapByHeartBeat(uint32_t cacheAddr){
// //    keyCacheMap: key - cacheServerAddr
//    for(auto keyCache : keyCacheMap){
//        if(keyCache.second == cacheAddr){
//            keyCacheMap.erase(keyCache.first);//erase problem cannot considered??????????????????????????????????????????????????
//        }
//    }
// }
//
//
//
//
// bool Master::furtherHeartBeatDetect(uint32_t cacheServerAddr){
//    return false;
// }


uint32_t Master::loadBalance(){
//    uint32_t cacheServerNum;
//    if(heartBeatDetect(cacheServerNum)){
//        return cacheServerNum;
//    }
    return 100;
}


