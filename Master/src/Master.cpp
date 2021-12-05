//
// Created by songyh on 2021/11/29.
//

#include "Master.hpp"
#include <iostream>
using namespace std;
//void Master::handleMessage(cMessage* msg) {
//    if(msg->num==1){
//        // �յ� client ����ĵ����ݰ������ܶ�����д��
//        handleClientReq(msg);
//    }
//    else if(msg->num==2){
//        // heartbeat detect----heartBeatResponse �յ������������ݰ�
//
//        handleHeartBeatResponse(msg);
//    } else if(msg->num==3){
//        //update key-cache server Num �յ�cache�е�key�����仯�����ݰ�����������Ϊ�����ݵ���������
//    }
//
//
//}

uint32_t Master::handleClientReq(cMessage* msg){
    // �յ�client������������������Ҫ����1��ִ��read/write��2��read/write��Ҫ��key
    // read--����key��Ӧ��cache��ַ��
    // write--ͨ�����ؾ����ȡkey��Ҫд���cache��ַ
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
//    // ������������ 
//    heartBeatResponse* heartResp = msg2heart(msg);
//    cacheAddrMap[heartResp->cacheServerAddr] = heartResp->timeFlag;
//}
//
//bool Master::heartBeatDetect(uint32_t cacheServerAddr) {
//    // ��cache server�ж��Ƿ���
//    uint32_t timeFlag = 0; // get time=??????????????????????????????????????????????????
//        if (cacheAddrMap[cacheServerAddr] - timeFlag < heartBeatInterval) {
//            return true;
//        }
//    return furtherHeartBeatDetect(uint32_t cacheServerAddr);
//}
//
// void Master::periodicDetectCache(){
//     // �����Ը��±��ص�cacheʱ����ı�
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


