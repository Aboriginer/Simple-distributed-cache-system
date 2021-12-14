#include "master.h" 
using namespace std;

// unordered_map<string , uint32_t> keyCacheMap;
// unordered_map<string, time_t> cacheAddrMap;
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
    cout<<"the next is while true"<<endl;
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
                string cacheServerAddr = handleClientMessage(recv_buff_str);
                string send_to_client = "MASTER#" + recv_buff_str + "#" + cacheServerAddr;
                // char cacheServerAddrChar[BUF_SIZE];
                // sprintf(cacheServerAddrChar, "%d", cacheServerAddr);
                // // cout<<"hikh"<<endl;
                //===========================================================
                strcpy(send_buff_client, send_to_client.c_str());  //CacheSever IP
                send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);

                memset(recv_buff_client, 0, sizeof(recv_buff_client));
                memset(send_buff_client, 0, sizeof(send_buff_client));
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

//    // 客户端列表
//    std::unordered_map<int, struct fdmap *> clients_list;
//    std::vector<int> fd_node;

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
        epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }
        // cout<<"epoll_events_count"<<epoll_events_count<<endl;
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
                // tmp->ip_port = 
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
                    if(clients_list[client_events[i].data.fd]==0){
                        clients_list.erase(client_events[i].data.fd);
                        epoll_events_count--;
                        continue;
                        // 这里还是会接收到关掉的那个cache，我不知道要怎么把他关掉，就直接continue了
                    }
                    struct fdmap *it = clients_list[client_events[i].data.fd];
                    //————————————————————————————————-————--————————————————————————————————————————————
                    std::cout << "get heartbeat from cache server:" << recv_buff_client << std::endl; 
                    //————————————————————————————————-————--————————————————————————————————————————————
                    //################################################# 2/3
                    string recv_buff_str = recv_buff_client;
                    // cout<<"recv_buff_str"<<recv_buff_str<<endl;
                    vector<string> vtmsg = split(recv_buff_str, "#");
                    // 陌生人自报家门
                    // cout<<"client_events[i].data.fd"<<client_events[i].data.fd<<endl;
                    // cout<<"client_list.size"<<clients_list.size()<<endl;
                    if (clients_list[client_events[i].data.fd]->ip_port == "0") {
                        cout<<"clients_list[client_events[i].data.fd]->ip_port" << clients_list[client_events[i].data.fd]->ip_port << endl;
                        string socli = vtmsg[1]+"#"+vtmsg[2];
                        //cout<<socli<<endl;
                        it->ip_port = socli; //写ip_port
                        cout<<it->ip_port<<endl;
                        clients_list[client_events[i].data.fd]->status = vtmsg[3][0]; //写主备
                        // cout<<socli<<endl;
                        //分配备份server
                        if (vtmsg[3] == "P") {
                            if (!rcache.empty()) {  //有多余的备份
                                clients_list[client_events[i].data.fd]->pair_fd = rcache.top();
                                clients_list[rcache.top()]->pair_fd = client_events[i].data.fd;
                                rcache.pop();
                            }
                            else {  //没有多余的备份
                                pcache.push(client_events[i].data.fd);
                            }
                        }
                        else if (vtmsg[3] == "R") {
                            if (!pcache.empty()) {
                                clients_list[client_events[i].data.fd]->pair_fd = pcache.top();
                                clients_list[pcache.top()]->pair_fd = client_events[i].data.fd;
                                pcache.pop();
                            }
                            else {
                                rcache.push(client_events[i].data.fd);
                            }
                        }                       
                        
                        // 扩容===============================================
                        
                        // 假设检测到一个新上线的cache
                        int index = fd_node.size()-1;// 获取它的实际节点索引
                        cacheAddrHash.addNode(index);// 在哈希部分增加节点
                        int fd = fd_node[index];// 获取fd的值
                        string cacheServerAddr = clients_list[fd]->ip_port;// 找到fd对应的ip和port
                        // N#new_ip#new_port
                        string extendmsg = "N#"+cacheServerAddr;
                        // 然后把这个extendmsg发给所有的cache====广播
                        // 广播新上线的cache
                        cout<<"send msg \'"<< extendmsg <<"\' to all cache"<<endl; 
                        for(auto fdi : fd_node){//将新加入的cache的ip和port发给所有的cache
                            cout<<"the fd is "<<fdi<<endl;
                            strcpy(send_buff_client, extendmsg.c_str());
                            send(fdi, send_buff_client, BUF_SIZE, 0);
                        }
                        // 向新上线的cache单独发送所有cache的IP和port
                        // ip1#port1#ip2#port2...
                        string allcache;
                        for(auto fdi : fd_node){
                            allcache = allcache + "#" + clients_list[fdi]->ip_port;
                        }
                        allcache = allcache.substr(1, allcache.length());
                        cout<<"send msg \'"<< allcache <<"\' to cache" <<cacheServerAddr <<endl;
                        strcpy(send_buff_client, allcache.c_str());
                        send(fd, send_buff_client, BUF_SIZE, 0);
                        //========================================================
                    }
                    //发送应答 p/r#ip#port
                    char cacheServerAddrChar[BUF_SIZE];
                    stringstream ss;
                    ss << it->status;
                    ss << "#";
                    if (it->pair_fd > -1) {
                        ss << (clients_list[it->pair_fd]->ip_port).c_str();
                    }
                    else {
                        ss << "none";
                    }
                    string str2 = ss.str();
                    strcpy(send_buff_client, str2.data());
                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    memset(send_buff_client, 0, sizeof(send_buff_client));
                    handleHeartBeatResponse(recv_buff_client);
                    //#################################################
                }
                // else if (pch == 'g') {  // 更新请求："g#ip#port#key"
                //     std::cout << "update KeyCacheMap By CacheReq:" << recv_buff_client << std::endl;
                //     updateKeyCacheMapByCacheReq(recv_buff_client);
                // }
            }
        }
    }
}

void Master::Start() {
    cacheAddrHash.initialize(0,100);
    auto f_client = std::bind(&Master::start_client, this);
    auto f_cache = std::bind(&Master::start_cache, this);
    auto f_periodheart = std::bind(&Master::periodicDetectCache,this);
    auto f_shrink = std::bind(&Master::shrinkageCapacity, this);
    std::thread for_client(f_client);
    std::thread for_cache(f_cache);
    std::thread for_heart(f_periodheart);
    std::thread for_shrink(f_shrink);


    for_client.join();
    for_cache.join();
    for_heart.join();
    for_shrink.join();
    // periodicDetectCache();
}


void Master::shrinkageCapacity(){
    // 假设检测到一个cache要下线====每次缩容都缩最后一个cache吧 = =|||
    char c;
    char send_buff_shink[BUF_SIZE];
    while(true){
        c = getchar();// 如果键盘输入's'——>则认为要缩容
        if(c=='s'){
            cout<< " shrink cache "<< endl;
            int index = fd_node.size()-1;//缩容减少的是最后一个cache
            cacheAddrHash.deleteNode(index);
            int fd = fd_node[index];// 获取fd的值
            // 根据fd找到对应的ip：port
            string cacheServerAddr = clients_list[fd]->ip_port; 
            // 格式：K#killed_ip#killed_port
            string shrinkmsg = "K#"+cacheServerAddr;
            cout<<"send shrinkmsg:"<<shrinkmsg << "to all cache"<<endl;
            // 然后把这个shrinkmsg发给所有的cache
            for(auto fdi : fd_node){
                strcpy(send_buff_shink, shrinkmsg.c_str());
                send(fdi, send_buff_shink, BUF_SIZE, 0);
                // cout<<fdi<<endl;
            }
            fd_node.pop_back();
            auto addr = clients_list.erase(fd);
            // 这里有两个问题：
            // 1 删除的信息可能没有同步到其他线程——>我不会 T T [所以我让收心跳包那里就直接continue了]
            // 2 缩容里主备分部分需要做的内容
        }
    }
}

string Master::handleClientMessage(string msg){
    // 获得虚拟节点对应的值
    cout<<"handle client message: the key is "<< msg <<endl;
    size_t vir_node = cacheAddrHash.GetServer(msg.c_str());
    cout<<"vir_node:" <<vir_node<<"\t";
    // 虚拟节点对应的真实节点的索引
    int index = cacheAddrHash.key2Index[vir_node];
    // 通过fd_node根据返回的index索引到cache对应的fd
    int fd = fd_node[index];
    // 根据fd找到对应的ip：port
    string cacheServerAddr1 = clients_list[fd]->ip_port;
    // 返回格式为ip:port?
    vector<string> vtstr = split(cacheServerAddr1, "#");
    string cacheServerAddr = vtstr[0]+":"+vtstr[1];
    cout<<"the cacheAddr is: "<<cacheServerAddr<<endl;
    return cacheServerAddr;
}


//################################################################################3/3
void Master::handleHeartBeatResponse(string msg) {
    // 心跳检测包处理
    //————————————————————————————————-————--——————————————————————————————
    //cout << "handle heartbeat message" << endl;
    //————————————————————————————————-————--——————————————————————————————
    time_t timeNow;
    time(&timeNow);
    //————————————————————————————————-————--——————————————————————————————
    //cout << "the time is: "<< timeNow<<endl;
    //————————————————————————————————-————--——————————————————————————————
    // "x#ip#port"，其中ip:port是cache_server的套接字地址    
    vector<string> vtmsg = split(msg, "#");
    if(vtmsg.size()<3){
        // error==========
    }
    string cacheAddr = vtmsg[1]+"#"+vtmsg[2];
    cacheAddrMap[cacheAddr] = timeNow;

    //TODO：更新时间戳
/* 
    vector<string> vtmsg = split(msg, "#");
    string cacheServerAddrStr = vtmsg[1];
    string cacheServerPortStr = vtmsg[2];
    string cacheServerTimeFlgStr = vtmsg[3];
    cout << "insert cache-time map with addr is " << cacheServerAddrStr << " and time is " << cacheServerTimeFlgStr << endl; */
}
//################################################################################

bool Master::heartBeatDetect(int fd) {
    string cacheServerAddr = clients_list[fd]->ip_port;
    // 对cache server判断是否存活
    // cout << "judge the cache " << cacheServerAddr<<" is or not activate" << endl;
    time_t timeNow;
    time(&timeNow);
    //————————————————————————————————-————--——————————————————————————————
    //cout<<"the diff: "<<timeNow - cacheAddrMap[cacheServerAddr]<<endl;  ｜
    //————————————————————————————————-————--——————————————————————————————
    if (timeNow - cacheAddrMap[cacheServerAddr] < heartBeatInterval) {
    //————————————————————————————————-————--——————————————————————————————
    //   cout << "the cache " << cacheServerAddr << " is activate" << endl;
    //————————————————————————————————-————--——————————————————————————————
        return true;
    }
    // return furtherHeartBeatDetect(cacheServerAddr);
    //————————————————————————————————-————--——————————————————————————————
    //cout << "the cache " << cacheServerAddr << " is not activate" << endl;
    //————————————————————————————————-————--——————————————————————————————
    return false;
}

//========================================================================================
// void Master::periodicDetectCache(){
//     while(true){
//         //————————————————————————————————-————--
//         //cout<< "periodicDetectCache"<<endl;   ｜
//         //————————————————————————————————-————--
//         // 周期性更新本地的cache时间戳的表
//         for(auto cacheAddr : cacheAddrMap){
//             time_t timeNow;
//             time(&timeNow);
//             if(heartBeatDetect(cacheAddr.first)){

//                 continue;
//             } else{
//                 // updateKeyCacheMapByHeartBeat(cacheAddr.first);
//                 // // 这里应该做类似缩容的事情——>删除哈希节点——>但是如何获取index？
//                 // index = ？
//                 // cacheAddrHash.deleteNode(index);
//             }
//         }
//         sleep(3);
//     }
// }
void Master::periodicDetectCache(){
    while(true){
        //————————————————————————————————-————--
        //cout<< "periodicDetectCache"<<endl;   ｜
        //————————————————————————————————-————--
        // 周期性更新本地的cache时间戳的表
        for(auto fd : fd_node){
            time_t timeNow;
            time(&timeNow);
            if(heartBeatDetect(fd)){   //存活

                continue;
            } 
            else{     //不存活
                if (clients_list[fd]->status == 'R') { //掉线的是备份cache
                    clients_list[clients_list[fd]->pair_fd]->pair_fd = -1;  // 配偶清空
                    pcache.push(clients_list[fd]->pair_fd);  //待配对状态
                    clients_list.erase(fd);//清除clients_list
                    //TODO  清除fd_node.
                }
                else if (clients_list[fd]->status == 'P')  { //掉线的是主cache
                    clients_list[clients_list[fd]->pair_fd]->pair_fd = -1;  // 配偶清空
                    clients_list[clients_list[fd]->pair_fd]->status = 'P';  //备份变主
                    pcache.push(clients_list[fd]->pair_fd);  //待配对状态
                    clients_list.erase(fd);//清除clients_list
                    //TODO  更改fd_node.
                    //TODO  通知备份变主
                }
                }
        }
        sleep(3);
    }
}
// void extendCapability(){
//     int index = fd_node.size();
//     cacheAddrHash.addNode(index);
// }

// void shrinkageCapacity(){
//     int index = fd_node.size()-1;
//     // fd_node.pop();
//     cacheAddrHash.deleteNode(index);
// }

// void Master::updateKeyCacheMapByCacheReq(string msg){
//     vector<string> vtmsg = split(msg, "#");
//     string cacheServerAddrStr = vtmsg[1];
//     string cacheServerPortStr = vtmsg[2];
//     string key = vtmsg[3];
//     cout<<"the key is update: key = " << key << "cache: "<<cacheServerAddrStr<<":"<<cacheServerPortStr<<endl;
// }
