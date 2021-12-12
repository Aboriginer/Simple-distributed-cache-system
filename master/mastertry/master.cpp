#include "master.h" 
using namespace std;

unordered_map<string , uint32_t> keyCacheMap;
unordered_map<uint32_t, time_t> cacheAddrMap;
Master::Master() {}

void Master::start_client() {
   // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];
    // 客户端列表
    std::unordered_map<int, string> clients_list;

    int client_sock;
    int client_listener = 0;
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));

	client_addr.sin_family = PF_INET;
	client_addr.sin_port = htons(CLIENT_PORT);
	client_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

	client_listener = 0;

    std::cout << "I am trying start a socket with client"<< std::endl;
    client_listener = socket(PF_INET, SOCK_STREAM, 0);
	if (client_listener < 0) {
		perror("listener");
        exit(-1);
	}
    int val = 1;
    if (setsockopt(client_listener, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }
    if (bind(client_listener, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("bind error");
        exit(-1);
    }

    int ret = listen(client_listener, 5);
    if (ret < 0) {
        perror("listen error");
        exit(-1);
    }

    std::cout << "Start to listen: " << MASTER_IP << ":" << CLIENT_PORT << std::endl;
    static struct epoll_event client_events[EPOLL_SIZE];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, client_listener, true);
    // 开辟共享内存 

    while (true) {
        // epoll_events_count表示就绪事件数目
        int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        for (int i = 0; i < epoll_events_count; i ++ ) {
            // 新用户连接
            if (client_events[i].data.fd == client_listener) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int clientfd = accept(client_listener, (struct sockaddr*)&client_address, &client_addrLength);
                std::cout << "client connection from: " << inet_ntoa(client_address.sin_addr) << ":"
                          <<  ntohs(client_address.sin_port) << ", client_fd = " << clientfd << std::endl;

                addfd(epfd, clientfd, true);
                
                // 服务端用map保存用户连接，fd对应客户端套接字地址
                string clients_list_key = inet_ntoa(client_address.sin_addr)+':'+ntohs(client_address.sin_port);
                clients_list[clientfd] = clients_list_key;
                std::cout << "Add new clientfd = " << clientfd << " to epoll" << std::endl;
                std::cout << "Now there are " << clients_list.size() << " clients in the chat room" << std::endl;
            }
            else {
                bzero(recv_buff_client, BUF_SIZE);
                recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE, 0);

                //===========================================================
                string recv_buff_str = recv_buff_client;
                uint32_t cacheServerAddr = handleClientMessage(recv_buff_str);
                char cacheServerAddrChar[BUF_SIZE];
                sprintf(cacheServerAddrChar, "%d", cacheServerAddr);
                // cout<<"hikh"<<endl;
                //===========================================================
                strcpy(send_buff_client, cacheServerAddrChar);  //CacheSever IP
                send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);

                memset(recv_buff_client, 0, sizeof(recv_buff_client));
                memset(send_buff_client, 0, sizeof(recv_buff_client));
            }
        }
    }
}

void Master::start_cache() {
    // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];

    int cache_sock;
    int cache_listener = 0;

    // 客户端列表
    std::unordered_map<int, struct fdmap *> clients_list;
    std::vector<int> fd_node;

    // 主cache堆栈
    stack<int> pcache;
    // 备用cache堆栈
    stack<int> rcache;

    struct sockaddr_in cache_addr;
    bzero(&cache_addr, sizeof(cache_addr));

    cache_addr.sin_family = PF_INET;
    cache_addr.sin_port = htons(CACHE_PORT);
    cache_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

    cache_listener = 0;

    //处理client的需求的任务函数。


    std::cout << "I am trying start a socket with cache"<< std::endl;
    cache_listener = socket(PF_INET, SOCK_STREAM, 0);
    if (cache_listener < 0) {
        perror("listener");
        exit(-1);
    }
    int val = 1;
    if (setsockopt(cache_listener, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0) {
        std::cout << "setsockopt error" << std::endl;
    exit(1);
    }
    if (bind(cache_listener, (struct sockaddr *)&cache_addr, sizeof(cache_addr)) < 0) {
        perror("bind error");
        exit(-1);
    }

    int ret = listen(cache_listener, 5);
    if (ret < 0) {
        perror("listen error");
        exit(-1);
    }

    std::cout << "Start to listen: " << MASTER_IP << ":" << CACHE_PORT << std::endl;
    static struct epoll_event client_events[EPOLL_SIZE];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, cache_listener, true);
    // 开辟共享内存

    while (true) {
        // epoll_events_count表示就绪事件数目
        int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        for (int i = 0; i < epoll_events_count; i ++ ) {
            // 新用户连接
            if (client_events[i].data.fd == cache_listener) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int clientfd = accept(cache_listener, (struct sockaddr*)&client_address, &client_addrLength);
                std::cout << "cache server connection from: " << inet_ntoa(client_address.sin_addr) << ":"
                          <<  ntohs(client_address.sin_port) << ", client_fd = " << clientfd << std::endl;

                addfd(epfd, clientfd, true);
                //#################################################1/3
                // 服务端用map保存用户连接，fd对应客户端套接字地址
                struct fdmap *tmp = new fdmap(clientfd);
                fd_node.push_back(clientfd);
                // 还有主备份问题要改
                clients_list[clientfd] = tmp;
                //#################################################
                std::cout << "Add new clientfd = " << clientfd << " to epoll" << std::endl;
                std::cout << "Now there are " << clients_list.size() << " cache servers int the chat room" << std::endl;
            }
            else {
                bzero(recv_buff_client, BUF_SIZE);
                recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE, 0);
                // 判断心跳包还是更新包
                char pch = recv_buff_client[0];
                if (pch == 'x') { // 心跳包请求："x#ip#port#status"
                    struct fdmap *it = clients_list[client_events[i].data.fd];
                    std::cout << "get heartbeat from cache server:" << recv_buff_client << std::endl;
                    //################################################# 2/3
                    string recv_buff_str = recv_buff_client;
                    vector<string> vtmsg = split(recv_buff_str, "#");
                    // 陌生人自报家门
                    if (clients_list[client_events[i].data.fd]->ip_port == "0") {
                        string socli = vtmsg[1]+"#"+vtmsg[2];
                        cout<<socli<<":"<<socli.size()<<endl;
                        it->ip_port = socli; //写ip_port
                        cout<<it->ip_port<<":"<<it->ip_port.size()<<endl;
                        clients_list[client_events[i].data.fd]->status = vtmsg[3][0]; //写主备
                        // cout<<socli<<endl;
                        //分配备份server
                        if (vtmsg[3] == "p") {
                            if (!rcache.empty()) {
                                clients_list[client_events[i].data.fd]->pair_fd = rcache.top();
                                rcache.pop();
                            }
                        }
                        else if (vtmsg[3] == "r") {
                            if (!pcache.empty()) {
                                clients_list[client_events[i].data.fd]->pair_fd = pcache.top();
                                pcache.pop();
                            }
                        }                       
                    }
                    //发送应答 p/r#ip#port
                    char cacheServerAddrChar[BUF_SIZE];
                    cout<<"endif"<<endl;
                    // char ctmp[20];
                    // string s="1234";
                    // strcpy(ctmp, clients_list[it->pair_fd]->ip_port.data());
                    // char * sendtmp = ;
                    // cout<<"string"<<endl;
                    // sprintf(send_buff_client, "%c#%s", , ctmp);
                    // cout<<"sprintf"<<endl;
                    // strcpy(send_buff_client,sendtmp.c_str());
                    // strcpy(send_buff_client, sendtmp);
                    stringstream ss;
                    ss << it->status;
                    // cout<<"1"<<endl;
                    ss << "#";
                    // cout<<"2"<<endl;
                    // string tmp =  clients_list[it->pair_fd]->ip_port;
                    // cout<<tmp<<endl;
                    // cout<<tmp.size()<<endl;
                    if (it->pair_fd > -1) {
                        ss << (clients_list[it->pair_fd]->ip_port).c_str();
                    }
                    else {
                        ss << "ip#port";
                    }
                    string str2 = ss.str();
                    // cout<<"string"<<endl;
                    // sprintf(tmp, "%c", );
                    // tmp += "#";
                    // tmp += clients_list[it->pair_fd]->ip_port;
                    strcpy(send_buff_client, str2.data());
                    // cout<<"strcpy"<<endl;
                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    // cout<<"send"<<endl;
                    memset(send_buff_client, 0, sizeof(send_buff_client));
                    handleHeartBeatResponse(client_events[i].data.fd);
                    //#################################################
                }
                else if (pch == 'g') {  // 更新请求："g#ip#port#key"
                    std::cout << "update KeyCacheMap By CacheReq:" << recv_buff_client << std::endl;
                    updateKeyCacheMapByCacheReq(recv_buff_client);
                }
            }
        }
    }
}

void Master::Start() {
    cacheAddrHash.initialize(3,100);
    auto f_client = std::bind(&Master::start_client, this);
    auto f_cache = std::bind(&Master::start_cache, this);
    std::thread for_client(f_client);
    std::thread for_cache(f_cache);

    for_client.join();
    for_cache.join();
}

uint32_t Master::handleClientMessage(string msg){
    // 收到client的请求包，请求包中需要包含1）执行read/write；2）read/write需要的key
    // read--返回key对应的cache地址；
    // write--通过负载均衡获取key需要写入的cache地址  read#key
    // vector<string> vtmsg = split(msg, "#");
    cout << "handle Client Request" << endl;
//    string clientkey = msg->key;
//    bool write = msg->write;
//     if(vtmsg.size()!=2){
// //        error==========================
//     }
//     string msgflg = vtmsg[0];
//     string clientkey = vtmsg[1];
    uint32_t cacheServerAddr = cacheAddrHash.GetServer(msg.c_str());
    // uint32_t cacheServerAddr = 0;
    // if(msgflg == "w"){
    //     //write
    //     cout << "the request is write" << endl;
    //     if (keyCacheMap.find(clientkey) == keyCacheMap.end()) {
    //         // the cache server dose not cache the contain of key --> For the first time to write
    //         cacheServerAddr = getCacheServerAddr();
    //         keyCacheMap.insert(pair<string, uint32_t>(clientkey, cacheServerAddr));
    //     }
    //     else {
    //         // the cache server cache the contain of key --> update the value of key
    //         cacheServerAddr = getCacheServerAddr(clientkey);
    //     }
    //     cout << "keyCacheMap[" << clientkey << "]=" << cacheServerAddr << endl;
    // } else if (msgflg == "r"){
    //     // read
    //     cout << "the request is read" << endl;
    //     cacheServerAddr = getCacheServerAddr(clientkey);
    //     cout << "the key is in cache server" << cacheServerAddr << endl;
    // }
    return cacheServerAddr;
}


// uint32_t Master::getCacheServerAddr(string key){
//     // get cacheServerNum by key (client read)
//     if(keyCacheMap.find(key)==keyCacheMap.end()){
// //        error========================================================
// //        throw string("cache server has not contain about key");
//     }
//     return keyCacheMap[key];
// }


// uint32_t Master::getCacheServerAddr(){
//     // get cacheServerNum by load balance (client write)
//     uint32_t cacheServerAddr = loadBalance();
//     return cacheServerAddr;
// }


//################################################################################3/3
void Master::handleHeartBeatResponse(int msg) {
    // 心跳检测包处理
    cout << "handle heartbeat message" << endl;
    //TODO：更新时间戳
/* 
    vector<string> vtmsg = split(msg, "#");
    string cacheServerAddrStr = vtmsg[1];
    string cacheServerPortStr = vtmsg[2];
    string cacheServerTimeFlgStr = vtmsg[3];
    cout << "insert cache-time map with addr is " << cacheServerAddrStr << " and time is " << cacheServerTimeFlgStr << endl; */
}
//################################################################################

bool Master::heartBeatDetect(uint32_t cacheServerAddr) {
    // 对cache server判断是否存活
    cout << "judge the cache " << cacheServerAddr<<" is or not activate" << endl;
    time_t timeNow;
    time(&timeNow);
    if (cacheAddrMap[cacheServerAddr] - timeNow < heartBeatInterval) {
        cout << "the cache " << cacheServerAddr << " is activate" << endl;
        return true;
    }
    return furtherHeartBeatDetect(cacheServerAddr);
}

bool Master::furtherHeartBeatDetect(uint32_t cacheServerAddr) {
    return false;
}
//========================================================================================
void Master::periodicDetectCache(){
    // 周期性更新本地的cache时间戳的表
    for(auto cacheAddr : cacheAddrMap){
//        time_t timeNow = time(&(this->timeFlag));
        time_t timeNow;
        time(&timeNow);
        if(heartBeatDetect(cacheAddr.first)){

            continue;
        } else{
            updateKeyCacheMapByHeartBeat(cacheAddr.first);
        }
    }
}

void Master::updateKeyCacheMapByHeartBeat(uint32_t cacheAddr){
    //    keyCacheMap: key - cacheServerAddr
    // for(auto keyCache : keyCacheMap){
    //     if(keyCache.second == cacheAddr){
    //         keyCacheMap.erase(keyCache.first);
    //     }
    // }
}

// uint32_t Master::loadBalance(){
// //    uint32_t cacheServerNum;
// //    if(heartBeatDetect(cacheServerNum)){
// //        return cacheServerNum;
// //    }
//     return 100;
// }

void Master::updateKeyCacheMapByCacheReq(string msg){
    vector<string> vtmsg = split(msg, "#");
    string cacheServerAddrStr = vtmsg[1];
    string cacheServerPortStr = vtmsg[2];
    string key = vtmsg[3];
    cout<<"the key is update: key = " << key << "cache: "<<cacheServerAddrStr<<":"<<cacheServerPortStr<<endl;
}
