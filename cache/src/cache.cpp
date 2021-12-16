//
// Created by Aboriginer on 12/1/2021.
//

#include "cache.h"

// bool operator == (const std::pair<std::string, std::string> &a, 
//                         std::pair<std::string, std::string> &b){
//                             return a.first == b.first;
//                         }

Cache::Cache(int cache_size_local, std::string status, std::string local_cache_IP, std::string port_for_client,
             std::string port_for_cache) {
    cache_size_local_ = cache_size_local;
    status_ = status;
    pr_status_ = status;
    local_cache_IP_ = local_cache_IP;
    port_for_client_ = port_for_client;
    port_for_cache_ = port_for_cache;

    // 以下成员变量用于容灾
    target_IP_ = "None";
    target_port_ = "None";
    cache_list_update_flag = false;
    kv_update_flag = false;
    is_initialed = 0;
    initial_flag = false;
    if (pr_status_ == "R") initial_flag = true;
}

void Cache::Start() {
    auto Heartbeat_bind = std::bind(&Cache::Heartbeat,  this);
    auto Client_chat_bind = std::bind(&Cache::Client_chat, this);
    auto Cache_pass_bind = std::bind(&Cache::cache_pass, this);
    auto Cache_replica_bind = std::bind(&Cache::replica_chat, this);

    std::thread for_master_Heartbeat(Heartbeat_bind);
    std::thread for_client(Client_chat_bind);
    std::thread for_cache(Cache_pass_bind);
    std::thread for_replica(Cache_replica_bind);

    for_replica.join();
    for_cache.join();
    for_client.join();
    for_master_Heartbeat.join();
}

//从master接受初始化信息
void Cache::initial(int cache_master_sock, char *recv_buff_initial) {
    //cache将一直监听，直到master向cache传递初始化数据位置。

    auto is_not_init = [](char* message)->bool{
        if(strlen(message)) return false;
        else if(!(message[0] >= '0' && message[0] <= '0')){
            return false;
        }else{
            return true;
        }
    };

    while(is_not_init(recv_buff_initial)){
        bzero(recv_buff_initial, BUF_SIZE);
        recv(cache_master_sock, recv_buff_initial, BUF_SIZE, 0);
    }

    if(strlen(recv_buff_initial) == 0){
        std::cout<< "ERROR : didn't received any message from master."<<std::endl;
        is_initialed = ERROR_INIT;   //没有收到任何消息,初始化错误
        return;
    }

    //解包
    int count = 0;
    vector<std::string> ip, port;
    std::string tmp;
    std::cout<<"receving initial message from master."<<std::endl;
    // std::cout << "========================receving ip#port:" << recv_buff_initial << std::endl;
    for(int i = 0; i < strlen(recv_buff_initial); i++){
        char single = recv_buff_initial[i];
        if(single != '#'){
            tmp += single;
        }else{
            if(count % 2 == 0) ip.push_back(tmp);
            else port.push_back(tmp);
            tmp.clear();
            count++;
        }
    }
    port.push_back(tmp);

    if(ip.size() != port.size()){
        std::cout<<"ERROR : IPs and ports cannot correspond one by one. wrong message."<<std::endl;
        is_initialed = ERROR_INIT;   //初始化错误
        return;
    }


    // std::cout<<"writing ip & port to hashmap ."<<std::endl;
    // std::cout<<"ip           port             "<<std::endl;
    for(int i = 0; i < ip.size(); i++){
        // std::cout<<ip[i]<<" "<<port[i]<<std::endl;
        pair <std::string , std::string> pair (ip[i], port[i]);
        cache_list.emplace_back(pair);
    }
    std::cout<<"write successful."<<std::endl;
    std::cout << "Number of online cache:" << cache_list.size() << std::endl;
    std::cout << "Online cache list:" << std::endl;
    for (int i = 0; i < cache_list.size(); i ++ ) {
        cout << "cache" << i + 1 << " ip:" << cache_list[i].first << ", " << "cache" << i + 1 << " port:" << cache_list[i].second << std::endl;
    }
    std::cout << "==================" << std::endl;
    is_initialed = SUCCESS_INIT;
}


void Cache::Heartbeat() {
    static auto timer = std::make_shared<Timer> (10, true, nullptr, nullptr); //10ms上传一次心跳包
    timer->setCallback([this](void * pdata){

        char send_buff_master[BUF_SIZE], recv_buff_master[BUF_SIZE];
        std::string master_port = std::to_string(MASTER_PORT);

        static int cache_master_sock = client_socket(MASTER_IP, master_port);

        std::string heart_message = "x#" + local_cache_IP_ + "#" + port_for_client_ + "#" + port_for_cache_ + "#" + pr_status_;
        strcpy(send_buff_master, heart_message.data());
        // std::cout << "Send message:" << send_buff_master << std::endl;
        send(cache_master_sock, send_buff_master, BUF_SIZE, 0);
        // std::cout << "Heartbeat successfully!" << std::endl;
        bzero(send_buff_master, BUF_SIZE);
        bzero(recv_buff_master, BUF_SIZE);

        // i ++;
        bzero(recv_buff_master, BUF_SIZE);
        // 设置为非阻塞模式接收信息
        int len = recv(cache_master_sock, recv_buff_master, BUF_SIZE, MSG_DONTWAIT);
        // TODO:测试用，待删除
        // std::cout << "================len:" << len << std::endl;
        // std::cout << "recv:" << recv_buff_master << std::endl;
        // std::cout << "=====================i:" << i << std::endl;
        if (len != 0 && initial_flag) {
            if (recv_buff_master[0] != 'P') std::cout << "Receive from master:" <<  recv_buff_master << std::endl;
            ReadFromMaster(recv_buff_master);
        }
        // std::cout << "===================== second i:" << i << std::endl;
        while (!initial_flag && len > 0) {
            initial(cache_master_sock, recv_buff_master);
            if(is_initialed == NO_INIT){
                std::cout<<"ERROR: the cache is not initiated. "<<std::endl;
                exit(EXIT_FAILURE);
            }else if(is_initialed == ERROR_INIT){
                std::cout<<"ERROR: something wrong when initiating the cache."<<std::endl;
                exit(EXIT_FAILURE);
            }
            initial_flag = true;
        }
        // std::cout << "===================== last i:" << i << std::endl;
        // std::cout << ">>>>>>>>>>>>>>>>>>>>>>>cache_list size:" << cache_list.size() << std::endl; 
    });
    timer->start();
}

void Cache::Client_chat() {
    //线程池
    ThreadPool TP(MAX_THREADS_NUMBER);
    std::string buffer;
    std::queue<std::future<int>> future_queue;
    //LRU缓存
//    LRU_Cache<std::string, std::string> MainCache(LRU_CAPACITY);

    int serv_client_sock = server_socket(local_cache_IP_, port_for_client_);
    // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];

    //处理client的需求的任务函数。
    auto LRU_handle_task = [&](std::string message, int targetPort) -> int {
        //分割协议报文
        int spear = 0;
        for (int i = 0; i < message.size(); i++) {
            if (message[i] != '#') {
                ++spear;
            } else {
                break;
            }
        }
        // std::cout << "message = " + message << std::endl;
        if (spear == message.size()) {
            //client要求读出值
            std::string key = message;
            std::string res;
            if (MainCache.check(key) > 0) {
                res = MainCache.get(key);
                buffer = "SUCCESS#" + key + "#" + local_cache_IP_ + ":" + port_for_client_ + "#" + res;

            } else {
                buffer = "FAILED#" + key + "#" + local_cache_IP_ + ":" + port_for_client_;
            }

            return targetPort;
        } else {
            //client要求写入值
            std::string key = message.substr(0, spear);
            std::string val = message.substr(spear + 1, message.size());
            MainCache.put(key, val);
            buffer = "SUCCESS#" + key + "#" + local_cache_IP_ + ":" + port_for_client_;

            return targetPort;
        }
    };

    auto get_port = [](std::future<int> &future) -> int {
        return future.get();
    };

    epoll_event client_events[MAX_EVENT_NUMBER];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, serv_client_sock, true);
    // 开辟共享内存

    while (true) {
        // epoll_events_count表示就绪事件数目
        int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if (epoll_events_count < 0) {
//            perror("epoll failure");
//            break;
            continue;
        }

        for (int i = 0; i < epoll_events_count; i++) {
            // 新用户连接
            if (client_events[i].data.fd == serv_client_sock) {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int client_fd = accept(serv_client_sock, (struct sockaddr *) &client_address, &client_addrLength);
                std::cout << "client connection from:" << inet_ntoa(client_address.sin_addr) << ":"
                          << ntohs(client_address.sin_port) << ", client_fd = " << client_fd << std::endl;

                addfd(epfd, client_fd, true);
            } else {
                bzero(recv_buff_client, BUF_SIZE);
                int len = recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE - 1, 0);
                
                // 打印输出显示用
                std::string client_request_status;
                std::string return_key;
                char* Delimiter = strchr(recv_buff_client, '#');
                std::string temp_recv = recv_buff_client;
                if (Delimiter == NULL) {
                    client_request_status = "Read key:";
                    return_key = recv_buff_client;
                }else {
                    client_request_status = "Write key:";
                    return_key = temp_recv.substr(0, Delimiter - recv_buff_client);
                }


                if (len == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_events[i].data.fd, NULL);
                    close(client_events[i].data.fd);
                    std::cout << "close client:" << client_events[i].data.fd << std::endl;
                }
                else {
//                    LRU_handle_task(recv_buff_client, i);
//                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    auto future = TP.enqueue(LRU_handle_task, recv_buff_client, i);
                    future_queue.emplace(std::move(future));
                    int targetPort = get_port(future_queue.front());
                    memcpy(send_buff_client, buffer.c_str(), buffer.size());
                    //向端口传出数据：SUCCESS/FAILED#key#ip:port(cache server)
                    send(client_events[targetPort].data.fd, send_buff_client, BUF_SIZE, 0);

                    // 打印输出显示用
                    std::string return_status;
                    if (buffer[0] == 'S') return_status = ",SUCCESS";
                    else return_status = ",FAILED";

                    // std::cout << "========================================return value:" << send_buff_client << std::endl;
                    std::cout << client_request_status + return_key + return_status << std::endl;

                    //这里用一把锁来管理Replica的缓冲区。
                    // TODO : 记得在Replica那边也要试图访问这个锁
                    kv_update_flag = true;
                    kv_mutex.lock();
                    kv_to_replica.clear();
                    kv_to_replica += recv_buff_client;
                    kv_mutex.unlock();
                    //移除事件
                    future_queue.pop();
                }
                bzero(send_buff_client, BUF_SIZE);
            }
        }
    }
    close(serv_client_sock);
    close(epfd);
}


//向目标IP传递信息
void Cache::to_single_cache(std::string &ip, std::string &port, std::string &key){
    int cache_cache_sock = client_socket(ip, port);

    char send_buff_master[BUF_SIZE];
    // Master返回的扩缩容信息
    bzero(send_buff_master, BUF_SIZE);
    std::string val = MainCache.get(key);
    std::string message = key + "#" + val;
    strcpy(send_buff_master, message.data());
    std::cout<<"sending message = "<<send_buff_master<<std::endl;
    send(cache_cache_sock, send_buff_master, BUF_SIZE, 0);
    std::cout<<"message send."<<std::endl;
    bzero(send_buff_master, BUF_SIZE);
}


void slice(std::string message, std::string &key, std::string &val){
    int spear = 0;
    while(spear != message.size() && message[spear] != '#'){
        spear ++;
    }
    key = message.substr(0, spear);
    val = message.substr(spear + 1, message.size());
}


//从目标IP地址接受信息
//这里似乎没有将备份也考虑进去。
void Cache::from_single_cache(std::string &ip, std::string &port){
    char recv_buff_master[BUF_SIZE];
    int serv_sock = server_socket(ip, port);
    int clnt_sock;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz;
    clnt_adr_sz = sizeof(clnt_adr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
    std::cout << "client connection from:" << inet_ntoa(clnt_adr.sin_addr) << ":"
                << ntohs(clnt_adr.sin_port) << ", client_fd = " << clnt_sock << std::endl;

    recv(serv_sock, recv_buff_master, BUF_SIZE, 0);
    std::cout<<"receving message = "<<recv_buff_master<<std::endl;
    std::string key ,val;
    slice(recv_buff_master, key ,val);
    std::cout<<"putting key & val :"<<std::endl;
    std::cout<<key<<" "<<val<<std::endl;
    MainCache.put(key, val);
    std::cout<<"receved."<<std::endl;
}


void Cache::ReadFromMaster(std::string message) {
    int spear = 2;
    while (message.size() && spear != message.size() && message[spear] != '#') {
        spear++;
    }

    auto part = [](std::string str,int spear, int part_num)->std::string{
        if(part_num == 1) return str.substr(2, spear - 2);
        else if(part_num == 2)return str.substr(spear + 1, str.size() - spear -1);
    };
    std::string head = message.substr(0,1);   //P或者R
    // std::cout << "====================ReadFromMaster i:" << i << std::endl;
    std::lock_guard<std::mutex> guard(status_mutex);
    if(head == "K"){
        dying_cache_IP_ = part(message, spear, 1);
        dying_cache_Port = part(message, spear, 2);
        
        // TODO：这里感觉应该更新完cache_list再把状态改为K
        update_cache(dying_cache_IP_, dying_cache_Port, "K"); // 更新cache_list
        if(dying_cache_IP_ == local_cache_IP_ && dying_cache_Port == port_for_cache_){
            status_ = "K";
        }
    }else if(head == "N"){
        std::string neo_cache_IP = part(message, spear, 1);
        std::string neo_cache_Port = part(message, spear, 2);
        update_cache(neo_cache_IP, neo_cache_Port, "N");
    }else if(head =="P"){
        status_ = head;
        pr_status_ = "P";   // 用于备份转正
        if(message.substr(2, message.size() - 2) == "None"){
            target_IP_ = "None";
            target_port_ = "None";
        }else {
            target_IP_  = part(message, spear, 1);
            target_port_ = part(message, spear, 2);
        }
    }else if(head == "R"){
        status_ = head;
        if(message.substr(2, message.size() - 2) == "None"){
            target_IP_ = "None";
            target_port_ = "None";
        }else {
            target_IP_  = part(message, spear, 1);
            target_port_ = part(message, spear, 2);
        }
    }else if(head == "C"){
        std::string origin_ = part(message, spear, 1);
        std::string backup_ = part(message, spear, 2);
        // int spear1 = 0, spear2 = 0;
        auto slice = [](std::string origin_)->int{
            int spear1 = 0;
            while(spear1 != origin_.size() && origin_[spear1] != ':'){
                spear1++;
            }
        };
        int ptr1 = slice(origin_);
        int ptr2 = slice(backup_);
        std::string origin_ip = part(origin_, ptr1, 1);
        std::string origin_port = part(origin_, ptr1, 2);
        std::string backup_ip = backup_.substr(0, ptr2);
        std::string backup_port = backup_.substr(ptr2 + 1, backup_.size() - ptr2 -1);
        //现在对表进行替换。
        std::pair<std::string , std::string> origin = {origin_ip, origin_port};
        std::pair<std::string , std::string> backup = {backup_ip, backup_port};
        auto replace = [&origin, &backup](std::pair<std::string, std::string> a){
            if(a.first == origin.first && a.second == origin.second){
                a.first = backup.first;
                a.second = backup.second;
            }
            return a.first == origin.first && a.second == origin.second;
        };
        auto it  = find_if(cache_list.begin(), cache_list.end(), replace);
    }else if(head == "D"){
        std::string dead_ip = part(message, spear, 1);
        std::string dead_port = part(message, spear, 2);
        std::pair<std::string , std::string> dead_ = {dead_ip, dead_port};
        auto equal = [&dead_](std::pair<std::string, std::string> &a){
            return a.first == dead_.first &&a.second == dead_.second;
        };
        auto it  = find_if(cache_list.begin(), cache_list.end(), equal);
        cache_list.erase(it);
    }
    // std::cout << "====================ReadFromMaster after i:" << i << std::endl;
}

//扩缩容函数
void Cache::cache_pass(){
    // dying_cache_IP_ = "127.0.0.1";
    // dying_cache_Port = "8889";
    // std::cout<<"u r going to ..."<<std::endl;
    // std::cin>> status_;
    // std::cout<<(status_ == "K" ? "die" : "primary")<<std::endl;
    if(status_ == "K"){
        // TODO：otherIP是啥含义，要缩容的所有IP？
        // otherIP.push_back("127.0.0.1");
        // otherPort.push_back("8888");
        // std::cout<<"set"<<std::endl;
        if(otherIP.size() == 0){
            std::cout<<"IP address is not sent by the master."<<std::endl;
            return;
        }
        int size = otherIP.size();
        std::cout << "Start cal_hash_key, cache_list size:" << cache_list.size() << std::endl;
        //计算目标地址
        cal_hash_key();
        // out_key.push_back("123");
        //传递数据
        for(int i = 0; i < otherIP.size(); i++){
            std::cout<<"handling cache = "<<i<<std::endl;
            std::cout<<"cache = "<<otherIP[i]<<":"<<otherPort[i]<<std::endl;
            to_single_cache(otherIP[i], otherPort[i], out_key[i]);
        }
        //这把锁在to_replica线程里也会被管理，只有那里被解锁之后才能进行exit操作。
        end_mutex.lock();
        exit(0);
        end_mutex.unlock();
    }else if(status_ == "P"){
        std::cout<<"cache = "<<dying_cache_IP_<<" "<<dying_cache_Port<<std::endl;
        from_single_cache(local_cache_IP_, port_for_cache_);
        cache_list_update_flag = true;
        std::cout<<"cache_list_update_flag == "<< (cache_list_update_flag ? "true" : "false") <<std::endl;
        // TODO：为啥这里要sleep
        // sleep(4);
    }else if(status_ == "R"){
        from_single_cache(local_cache_IP_, port_for_cache_);
    }
}

//更新其他IP地址
void Cache::update_cache(std::string &IP, std::string &port,std::string status){
    if(status == "N"){
        // TODO:初始化后不加入cache本身
        if (IP != local_cache_IP_ || port != port_for_cache_) {
            pair <std::string , std::string> pair (IP, port);
            cache_list.emplace_back(pair);
            std::cout << "Add new cache, cache ip:" << IP << " cache port:" << port << std::endl;
            std::cout << "Number of online cache:" << cache_list.size() << std::endl;
            std::cout << "Online cache list:" << std::endl;
            for (int i = 0; i < cache_list.size(); i ++ ) {
                cout << "cache" << i + 1 << " ip:" << cache_list[i].first << ", " << "cache" << i + 1 << " port:" << cache_list[i].second << std::endl;
            }
            std::cout << "==================" << std::endl;
        }    

    }else if(status == "K"){
        dying_cache_IP_ = IP;
        dying_cache_Port = port;
        std::pair<std::string, std::string> pair = {IP, port};
        auto equal = [&pair](std::pair<std::string, std::string> &a){
            return a.first == pair.first &&a.second == pair.second;
        };
        auto it  = find_if(cache_list.begin(), cache_list.end(), equal);
        cache_list.erase(it);
        std::cout << "Shrink a cache, cache ip:" << IP << " cache port:" << port << std::endl;
        std::cout << "Number of online cache:" << cache_list.size() << std::endl;
        std::cout << "Online cache list:" << std::endl;
        for (int i = 0; i < cache_list.size(); i ++ ) {
            cout << "cache" << i + 1 << " ip:" << cache_list[i].first << ", " << "cache" << i + 1 << " port:" << cache_list[i].second << std::endl;
        }
        std::cout << "==================" << std::endl;
    }
    cache_list_update_flag = true;
}


// 容灾
void Cache::replica_chat() {
    while (true) {
        // 接收Master心跳包时要更新pr_status_
        if (pr_status_ == "P") {   // 本地cache是主cache，作为服务器端
            if (target_port_ != "None") {    // 确定备份cache已上线

                int clnt_sock;
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int serv_sock = server_socket(local_cache_IP_, port_for_cache_);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                std::cout << "client connection from:" << inet_ntoa(clnt_adr.sin_addr) << ":"
                          << ntohs(clnt_adr.sin_port) << ", client_fd = " << clnt_sock << std::endl;

                std::string send_message;
                char send_buff_replica[BUF_SIZE];
                while (true) {
                    // cache_list_update_flag = true;  // 用于测试
                    // kv_update_flag = true;  // 用于测试
                    if (cache_list_update_flag) {   // cache_list有更新
                        std::cout << "Need to write cache_list" << std::endl;
                        send_message.clear();
                        bzero(send_buff_replica, BUF_SIZE);
                        // 写入更新的cache_list
                        send_message = "#"; // 第一个字符是'#', 则表示收到的是cache_list
                        //现在开始传输哈希表
                        // TODO:cache_list是不是需要加锁
                        for(auto it : cache_list){
                            send_message += it.first + "#" +it.second;
                        }

                        std::cout << "Send cache_list to replica cache, fd = " << clnt_sock << std::endl;
                        strcpy(send_buff_replica, send_message.data());
                        send(clnt_sock, send_buff_replica, BUF_SIZE, 0);
                        std::cout << "Send message:" << send_buff_replica << std::endl;
                        cache_list_update_flag = false; // 成功写入cache_list
                    }
                    if (kv_update_flag) {
                        std::cout << "Need to write key/key#value" << std::endl;
                        send_message.clear();
                        bzero(send_buff_replica, BUF_SIZE);
                        kv_mutex.lock();
                        send_message = kv_to_replica;
                        kv_mutex.unlock();
                        std::cout << "Send key/key#value to replica cache, fd = " << clnt_sock << std::endl;
                        strcpy(send_buff_replica, send_message.data());
                        send(clnt_sock, send_buff_replica, BUF_SIZE, 0);
                        std::cout << "Send message:" << send_buff_replica << std::endl;
                        kv_update_flag = false; // 成功写入cache_list
                    }
                    // sleep(3);   // TODO:测试用，需删除
                }
                close(clnt_sock);
            }
        }   else {  // 本地cache是备份cache，作为客户端

            std::string recv_message;
            char recv_buff_primary[BUF_SIZE];
            std::cout << "Server connection from:";
            int clnt_sock = client_socket(target_IP_, target_port_);
            while (true) {
                if (pr_status_ != "R") {
                    std::cout << "Disaster recovery starts, Replica cache =====> Primary cache successfully" << std::endl;
                    break;  // 备份cache转正
                }
                recv_message.clear();
                bzero(recv_buff_primary, BUF_SIZE);
                int len = recv(clnt_sock, recv_buff_primary, BUF_SIZE, 0);
                recv_message = std::string(recv_buff_primary);
                std::cout << "Receive from primary cache:" << recv_message << std::endl;
                // 解析收到的recv_message，写入备份的LRU中
                if (recv_message[0] == '#') {   // 收到更新的cache_list
                    //上锁，防止主cache提前推出。
                    end_mutex.lock();
                    std::cout << "Receive cache_list" << std::endl;
                    std::vector<std::string> ip, port;
                    std::string tmp;
                    int count  = 0;
                    for(int i = 1; i < recv_message.size(); i++){
                        char single = recv_message[i];
                        if(single != '#'){
                            tmp += single;
                        }else{
                            if(count % 2 == 0) ip.push_back(tmp);
                            else port.push_back(tmp);
                            tmp.clear();
                            count++;
                        }
                    }
                    port.push_back(tmp);
                    if(ip.size() != port.size()){
                        std::cout<<"wrong message."<<std::endl;
                    }else{
                        for(int i = 0; i < ip.size(); i++){
//                            auto pair = {ip[i], port[i]};
                            pair <std::string , std::string> pair (ip[i], port[i]);
                            cache_list.emplace_back(pair);
                        }
                    }
                    //解锁。
                    end_mutex.unlock();
                    // TODO：这里要是有问题可以直接到ReadFromMaster中下线备份cache
                    // 主cache缩容后，备份cache下线
                    std::pair<std::string, std::string> local_pair = {target_IP_, target_port_};
                    auto equal = [&local_pair](std::pair<std::string, std::string> &a){
                        return a.first == local_pair.first &&a.second == local_pair.second;
                    };
                    if (find_if(cache_list.begin(), cache_list.end(), equal) == cache_list.end()) {
                        exit(0);
                    }

                }
                else{   // 收到更新的key/key#value
                    std::cout << "Receive key/key#value" << std::endl;
                    int spear = 0;
                    for (int i = 0; i < recv_message.size(); i++) {
                        if (recv_message[i] != '#') {
                            ++spear;
                        } else {
                            break;
                        }
                    }
                    if(spear == recv_message.size()){
                        std::string key = recv_message;
                        if(MainCache.check(key) > 0){
                            MainCache.get(key);
                        }
                    }else{
                        std::string key = recv_message.substr(0, spear);
                        std::string val = recv_message.substr(spear + 1, recv_message.size());
                        MainCache.put(key, val);
                    }
                }
            }
            close(clnt_sock);
        }
    }
}

void Cache::cal_hash_key() {
    std::vector<std::string> caches;
    ConsistentHash cache_hash;
    for(auto it : cache_list){
        std::string single = it.first + it.second;
        caches.push_back(single);
    }

    cache_hash.initialize(caches.size(), 100);
    std::vector<std::string> all_keys = MainCache.all_key();

    std::vector<size_t> all_index;
    if(otherIP.size() > 0) otherIP.clear();
    if(otherPort.size() > 0) otherPort.clear();
    for(std::string key : all_keys){
        size_t idx = cache_hash.key2Index[cache_hash.GetServer(key.c_str())];
        string target_cache = caches[idx];
        std::string ip;
        std::string port;
        slice(target_cache, ip, port);
        otherIP.push_back(ip);
        otherPort.push_back(port);
    }

    out_key = all_keys;
}

/*在调用server_socket函数后写入以下内容即可：
   int clnt_sock;
   struct sockaddr_in clnt_adr;
   socklen_t clnt_adr_sz;
   clnt_adr_sz = sizeof(clnt_adr);
   clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
   std::cout << "client connection from:" << inet_ntoa(clnt_adr.sin_addr) << ":"
             << ntohs(clnt_adr.sin_port) << ", client_fd = " << clnt_sock << std::endl;
*/
int server_socket(std::string server_IP, std::string server_port) {
    int serv_sock;
    struct sockaddr_in serv_adr;

    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {  // SOCK_STREAM表示这个套接字是一个连接的端点
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    char server_IP_[BUF_SIZE];
    memset(server_IP_, 0, sizeof(server_IP_));
    strcpy(server_IP_, server_IP.c_str());
    bzero(&serv_adr, sizeof(serv_adr));
    serv_adr.sin_family = PF_INET;
    serv_adr.sin_port = htons(stoi(server_port));
    serv_adr.sin_addr.s_addr = inet_addr(server_IP_);

    int setsockopt_val = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &setsockopt_val, sizeof(int)) < 0) {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }

    if (bind(serv_sock, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) == -1) {
        fprintf(stderr, "bind error\n");
        exit(EXIT_FAILURE);
    }

    if (listen(serv_sock, 5) == -1) {
        fprintf(stderr, "listen error\n");
        exit(EXIT_FAILURE);
    }

    std::cout << "Start to listen client" << std::endl;

    return serv_sock;
}

int client_socket(std::string server_IP, std::string server_port) {
    int sock;
    int str_len;

    struct sockaddr_in serv_adr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char server_IP_[BUF_SIZE];
    memset(server_IP_, 0, sizeof(server_IP_));
    strcpy(server_IP_, server_IP.c_str());
    bzero(&serv_adr, sizeof(serv_adr));
    serv_adr.sin_family = PF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(server_IP_);
    serv_adr.sin_port = htons(stoi(server_port));

    // 因为调用了connect，所以sock是客户端套接字
    //connect函数返回后，并不立刻进行数据交换（只是服务器端把来连接请求信息记录到等待队列）
    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        fprintf(stderr, "Connect failed\n");
        exit(EXIT_FAILURE);
    }
    else
        puts("Connected........");

    return sock;
}

// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
void addfd(int epollfd, int fd, bool enable_et) {
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}