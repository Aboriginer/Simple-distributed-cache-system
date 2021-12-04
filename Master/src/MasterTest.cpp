//
// Created by songyh on 2021/12/1.
//

#include <iostream>
#include <stdlib.h>
#include <unordered_map>
#include "cMessage.hpp"
#include "Master.hpp"
int main(){
    cMessage* msg = new cMessage(1,"cache1",1);
//    msg->write = 1;
//    msg->key = 1;
//    msg->num = 1;
    Master* masterTest = new Master();
    //cout<<masterTest->a<<endl;
    uint32_t num =  masterTest->handleClientReq(msg);
    //cout<<num<<endl;

    return 0;

}
