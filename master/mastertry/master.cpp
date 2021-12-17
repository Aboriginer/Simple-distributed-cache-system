#include "master.h"
using namespace std;

Master::Master() {}

void Master::start_client()
{
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

    std::cout << "I am trying start a socket with client" << std::endl;
    client_listener = socket(PF_INET, SOCK_STREAM, 0);
    if (client_listener < 0)
    {
        perror("listener");
        exit(-1);
    }
    int val = 1;
    if (setsockopt(client_listener, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
    {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }
    if (bind(client_listener, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }

    int ret = listen(client_listener, 5);
    if (ret < 0)
    {
        perror("listen error");
        exit(-1);
    }

    std::cout << "Start to listen: " << MASTER_IP << ":" << CLIENT_PORT << std::endl;
    static struct epoll_event client_events[EPOLL_SIZE];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, client_listener, true);
    // 开辟共享内存
    cout << "the next is while true" << endl;
    while (true)
    {
        // epoll_events_count表示就绪事件数目
        int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if (epoll_events_count < 0)
        {
            perror("epoll failure");
            break;
        }

        for (int i = 0; i < epoll_events_count; i++)
        {
            // 新用户连接
            if (client_events[i].data.fd == client_listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int clientfd = accept(client_listener, (struct sockaddr *)&client_address, &client_addrLength);
                std::cout << "client connection from: " << inet_ntoa(client_address.sin_addr) << ":"
                          << ntohs(client_address.sin_port) << ", client_fd = " << clientfd << std::endl;

                addfd(epfd, clientfd, true);

                // 服务端用map保存用户连接，fd对应客户端套接字地址
                string clients_list_key = inet_ntoa(client_address.sin_addr) + ':' + ntohs(client_address.sin_port);
                clients_list[clientfd] = clients_list_key;
                std::cout << "Add new clientfd = " << clientfd << " to epoll" << std::endl;
                std::cout << "Now there are " << clients_list.size() << " clients in the chat room" << std::endl;
            }
            else
            {
                bzero(recv_buff_client, BUF_SIZE);
                recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE, 0);
                string recv_buff_str = recv_buff_client;
                string cacheServerAddr = handleClientMessage(recv_buff_str);
                string send_to_client = "MASTER#" + recv_buff_str + "#" + cacheServerAddr;
                strcpy(send_buff_client, send_to_client.c_str()); //CacheSever IP
                send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);

                memset(recv_buff_client, 0, sizeof(recv_buff_client));
                memset(send_buff_client, 0, sizeof(send_buff_client));
            }
        }
    }
}


void Master::start_cache()
{
    // 向client发送的数据
    char send_buff_client[BUF_SIZE];
    // client返回的数据
    char recv_buff_client[BUF_SIZE];

    int cache_sock;
    int cache_listener = 0;

    struct sockaddr_in cache_addr;
    bzero(&cache_addr, sizeof(cache_addr));

    cache_addr.sin_family = PF_INET;
    cache_addr.sin_port = htons(CACHE_PORT);
    cache_addr.sin_addr.s_addr = inet_addr(MASTER_IP);

    cache_listener = 0;

    //处理client的需求的任务函数。

    std::cout << "I am trying start a socket with cache" << std::endl;
    cache_listener = socket(PF_INET, SOCK_STREAM, 0);
    if (cache_listener < 0)
    {
        perror("listener");
        exit(-1);
    }
    int val = 1;
    if (setsockopt(cache_listener, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) < 0)
    {
        std::cout << "setsockopt error" << std::endl;
        exit(1);
    }
    if (bind(cache_listener, (struct sockaddr *)&cache_addr, sizeof(cache_addr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }

    int ret = listen(cache_listener, 5);
    if (ret < 0)
    {
        perror("listen error");
        exit(-1);
    }

    std::cout << "Start to listen: " << MASTER_IP << ":" << CACHE_PORT << std::endl;
    static struct epoll_event client_events[EPOLL_SIZE];

    // 在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }
    // 往事件表中添加监听事件
    addfd(epfd, cache_listener, true);
    // 开辟共享内存

    while (true)
    {
        // epoll_events_count表示就绪事件数目
        // cout << epfd << endl;
        epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);
        // cout << "epoll_events_count" << epoll_events_count << endl;
        if (epoll_events_count < 0)
        {
            perror("epoll failure");
            break;
        }
        // cout<<"epoll_events_count"<<epoll_events_count<<endl;
        for (int i = 0; i < epoll_events_count; i++)
        {
            // cout<<"for 1 "<<fd_node.size()<<endl;
            // 新用户连接
            // cout<<"client_events[i].data.fd"<<client_events[i].data.fd<<"cache_listener"<<cache_listener<<endl;
            if (client_events[i].data.fd == cache_listener)
            {
                // cout<<"client"<<endl;
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(client_address);
                int clientfd = accept(cache_listener, (struct sockaddr *)&client_address, &client_addrLength);
                std::cout << "cache server connection from: " << inet_ntoa(client_address.sin_addr) << ":"
                          << ntohs(client_address.sin_port) << ", client_fd = " << clientfd << std::endl;

                addfd(epfd, clientfd, true);
                //#################################################1/3
                // 服务端用map保存用户连接，fd对应客户端套接字地址
                struct fdmap *tmp = new fdmap(clientfd);
                caches_list[clientfd] = tmp;
                //#################################################
                std::cout << "Add new clientfd = " << clientfd << " to epoll" << std::endl;
                std::cout << "Now there are " << caches_list.size() << " cache servers int the chat room" << std::endl;
            }
            else
            {
                // cout<<"else"<<endl;
                bzero(recv_buff_client, BUF_SIZE);
                recv(client_events[i].data.fd, recv_buff_client, BUF_SIZE, 0);
                // 判断心跳包还是更新包
                char pch = recv_buff_client[0];
                if (pch == 'x')
                { // 心跳包请求："x#local_cache_IP#port_for_client#port_for_cache#P/R"
                    // cout<<"caches_list[client_events[i].data.fd"<<i<<endl;
                    // if(caches_list[client_events[i].data.fd]==0){
                    //     caches_list.erase(client_events[i].data.fd);
                    //     epoll_events_count--;
                    //     // close(client_events[i].data.fd);    //不知道能不能关闭
                    //     continue;
                    //     // 这里还是会接收到关掉的那个cache，我不知道要怎么把他关掉，就直接continue了
                    // }
                    // cout<<"msg:"<<recv_buff_client<<endl;
                    // if(caches_list.size()==0){
                    //     continue;
                    // }
                    struct fdmap *it = caches_list[client_events[i].data.fd];
                    //————————————————————————————————-————--————————————————————————————————————————————
                    // std::cout << "get heartbeat from cache server:" << recv_buff_client << std::endl;
                    string recv_buff_str = recv_buff_client;
                    vector<string> vtmsg = split(recv_buff_str, "#");
                    if (caches_list[client_events[i].data.fd]->ip_port == "0")
                    { //第一次接受心跳

                        cout << "===================the new cache is access==============================" << endl;
                        // cout<<"caches_list[client_events[i].data.fd]->ip_port" << caches_list[client_events[i].data.fd]->ip_port << endl;
                        string socli = vtmsg[1] + "#" + vtmsg[2];
                        string socac = vtmsg[1] + "#" + vtmsg[3];
                        //cout<<socli<<endl;
                        it->ip_port = socli; //写ip_port
                        it->ip_cache = socac;
                        cout << it->ip_port << endl;
                        caches_list[client_events[i].data.fd]->status = vtmsg[4][0]; //写主备
                        // cout<<socli<<endl;
                        //分配备份server
                        if (vtmsg[4] == "P")
                        {                                                //主cache
                            fd_node.push_back(client_events[i].data.fd); //放入主cache向量fd_node
                            if (!rcache.empty())
                            { //有多余的备份
                                caches_list[client_events[i].data.fd]->pair_fd = rcache.top();
                                caches_list[rcache.top()]->pair_fd = client_events[i].data.fd;
                                rcache.pop();
                            }
                            else
                            { //没有多余的备份
                                pcache.push(client_events[i].data.fd);
                            }
                            
                            // 扩容===============================================

                            // 假设检测到一个新上线的cache
                            int index = fd_node.size() - 1;                     // 获取它的实际节点索引
                            cacheAddrHash.addNode(index);                       // 在哈希部分增加节点
                            int fd = fd_node[index];                            // 获取fd的值
                            string cacheServerAddr = caches_list[fd]->ip_cache; // 找到fd对应的ip和port(for cache)
                            // 向新上线的cache单独发送所有cache的IP和port
                            // ip1#port1#ip2#port2...
                            string allcache;
                            for (auto fdi : fd_node)
                            {
                                allcache = allcache + "#" + caches_list[fdi]->ip_cache;
                            }
                            allcache = allcache.substr(1, allcache.length());
                            cout << "send msg \'" << allcache << "\' to cache" << cacheServerAddr << endl;
                            strcpy(send_buff_client, allcache.c_str());
                            send(fd, send_buff_client, BUF_SIZE, 0);

                            // N#new_ip#new_port
                            string extendmsg = "N#" + cacheServerAddr;
                            // 然后把这个extendmsg发给所有的cache====广播
                            // 广播新上线的cache
                            cout << "send msg \'" << extendmsg << "\' to all cache" << endl;
                            for (auto fdi : fd_node)
                            { //将新加入的cache的ip和port发给所有的cache
                                // cout<<"the fd is "<<fdi<<endl;
                                cout << "send msg \'" << extendmsg << "\' to " << caches_list[fdi]->ip_cache << endl;
                                strcpy(send_buff_client, extendmsg.c_str());
                                // cout<<"send_buff_client"<<send_buff_client<<endl;
                                cout << "-------------------------------------------" << endl;
                                send(fdi, send_buff_client, BUF_SIZE, 0);
                                // sleep(1);
                            }
                            //========================================================
                        }
                        else if (vtmsg[4] == "R")
                        {
                            if (!pcache.empty())
                            {
                                caches_list[client_events[i].data.fd]->pair_fd = pcache.top();
                                caches_list[pcache.top()]->pair_fd = client_events[i].data.fd;
                                pcache.pop();
                            }
                            else
                            {
                                rcache.push(client_events[i].data.fd);
                            }
                        }

                    }
                    //发送应答 p/r#ip#port
                    char cacheServerAddrChar[BUF_SIZE];
                    stringstream ss;
                    ss << it->status;
                    ss << "#";
                    if (it->pair_fd > -1)
                    {
                        ss << (caches_list[it->pair_fd]->ip_cache).c_str();
                    }
                    else
                    {
                        ss << "None";
                    }
                    string str2 = ss.str();
                    strcpy(send_buff_client, str2.data());
                    send(client_events[i].data.fd, send_buff_client, BUF_SIZE, 0);
                    memset(send_buff_client, 0, sizeof(send_buff_client));
                    handleHeartBeatResponse(recv_buff_client);
                    //#################################################
                }
            }
        }
    }
}

void Master::Start()
{
    cacheAddrHash.initialize(0, 100);
    auto f_client = std::bind(&Master::start_client, this);
    auto f_cache = std::bind(&Master::start_cache, this);
    auto f_periodheart = std::bind(&Master::periodicDetectCache, this);
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

void Master::shrinkageCapacity()
{
    // 假设检测到一个cache要下线====每次缩容都缩最后一个cache吧 = =|||
    char c;
    char send_buff_shink[BUF_SIZE];
    while (true)
    {
        c = getchar(); // 如果键盘输入's'——>则认为要缩容
        if (c == 's' && (fd_node.size() > 0))
        {

            cout << " shrink cache " << endl;
            int index = fd_node.size() - 1; //缩容减少的是最后一个cache
            cacheAddrHash.deleteNode(index);
            int fd = fd_node[index]; // 获取fd的值
            // 根据fd找到对应的ip：port
            string cacheServerAddr = caches_list[fd]->ip_cache;
            // 格式：K#killed_ip#killed_port
            string shrinkmsg = "K#" + cacheServerAddr;
            cout << "send shrinkmsg:" << shrinkmsg << "to all cache" << endl;
            // 然后把这个shrinkmsg发给所有的cache
            for (auto fdi : fd_node)
            {
                strcpy(send_buff_shink, shrinkmsg.c_str());
                send(fdi, send_buff_shink, BUF_SIZE, 0);
            }
            if (caches_list[fd]->pair_fd > 0)
            {                                                             //如果有备份cache
                auto addr1 = caches_list.erase(caches_list[fd]->pair_fd); // 删除缩容的配偶cache
                // close(caches_list[fd]->pair_fd);                          //关闭配偶的cache通信
            }
            fd_node.pop_back();
            auto addr = caches_list.erase(fd); //删除主cache
            // close(fd);                         //关闭主socket
            // 这里有两个问题：
            // 1 删除的信息可能没有同步到其他线程——>需要测试
            // 2 缩容里主备分部分需要做的内容
            // auto addr = caches_list.erase(caches_list[fd]->pair_fd);  //删除备份cache
            // close(caches_list[fd]->pair_fd);         //关闭备份socket
            // cout<<
        }
        else if (c == 'c')
        {
            for (auto cache : caches_list)
            {
                cout << cache.second->ip_cache << " is active." << endl;
            }
        }
    }
}

string Master::handleClientMessage(string msg)
{
    // 获得虚拟节点对应的值
    cout << "handle client message: the key is " << msg << endl;
    size_t vir_node = cacheAddrHash.GetServer(msg.c_str());
    cout << "vir_node:" << vir_node << "\t";
    // 虚拟节点对应的真实节点的索引
    int index = cacheAddrHash.key2Index[vir_node];
    // 通过fd_node根据返回的index索引到cache对应的fd
    int fd = fd_node[index];
    // 根据fd找到对应的ip：port
    string cacheServerAddr1 = caches_list[fd]->ip_port;
    // 返回格式为ip:port?
    vector<string> vtstr = split(cacheServerAddr1, "#");
    string cacheServerAddr = vtstr[0] + ":" + vtstr[1];
    cout << "the cacheAddr is: " << cacheServerAddr << endl;
    return cacheServerAddr;
}

//################################################################################3/3
void Master::handleHeartBeatResponse(string msg)
{
    // 心跳检测包处理
    time_t timeNow;
    time(&timeNow);
    // "x#ip#port"，其中ip:port是cache_server的套接字地址
    vector<string> vtmsg = split(msg, "#");
    if (vtmsg.size() < 4)
    {
        // error==========
    }
    string cacheAddr = vtmsg[1] + "#" + vtmsg[2];
    cacheAddrMap[cacheAddr] = timeNow;
}
//################################################################################

bool Master::heartBeatDetect(int fd)
{
    string cacheServerAddr = caches_list[fd]->ip_port;
    // 对cache server判断是否存活
    time_t timeNow;
    time(&timeNow);
    if (timeNow - cacheAddrMap[cacheServerAddr] < heartBeatInterval)
    {
        return true;
    }
    return false;
}



void Master::periodicDetectCache()
{
    char send_buff_disaster[BUF_SIZE];
    while (true)
    {
        // 周期性更新本地的cache时间戳的表
        // cout << "caches_list.size(): " << caches_list.size() << ", fd_node.size()" << fd_node.size() << endl;
        // std::unordered_map<int, struct fdmap *>::iterator itcache;
        int deletefd = -1;
        for (auto itcache : caches_list)
        {
            int fd = itcache.second->fd;
            time_t timeNow;
            time(&timeNow);
            if (heartBeatDetect(fd))
            { //存活
                deletefd = -1;
                continue;
            }
            else
            {   //不存活
                deletefd = fd;
                //容灾
                // 2）如果是备份cache_2掉线后，
                // 则master通知主cache，她没有备份cache了
                if (caches_list[fd]->status == 'R')
                { //掉线的是备份cache
                    cout << "the backup cache is disaster " << endl;
                    caches_list[caches_list[fd]->pair_fd]->pair_fd = -1; // 配偶清空
                    pcache.push(caches_list[fd]->pair_fd);               //待配对状态
                    close(fd);
                }
                // 1）如果是主cache_1掉线后，
                // 如果cache_1有备份cache_2，则master设置将备份cache_2变为主cache，并且master通知备份cache_2现在是主cache，通知所有cache，将原本存ip_port的数据里，cache_1的位置更新为cache_2的位置
                // 如果cache_1没有备份cache，则master通知所有cache，将原本存ip_port的数据里,cache_1的数据删除，并且找到cache_1对应的cache索引index，删除哈希里的对应节点
                else if (caches_list[fd]->status == 'P')
                { //掉线的是主cache
                    cout << "the main cache is disaster and the fd is " << fd << ", addr is " << caches_list[fd]->ip_cache << endl;
                    if (caches_list[fd]->pair_fd > 0)
                    { //如果有备份cache
                        cout << "the main cache has backup cache, and the backup cache fd is " << caches_list[fd]->pair_fd << ", addr is " << caches_list[caches_list[fd]->pair_fd]->ip_cache << endl;
                        // 更改本地fd_node
                        int index = 0;
                        for (vector<int>::iterator it = fd_node.begin(); it != fd_node.end();)
                        {
                            if (*it == fd)
                            {
                                // it = fd_node.erase(it);
                                fd_node[index] = caches_list[fd]->pair_fd; // 把fd更改了
                                cout << "change fd_node: " << fd << "->" << caches_list[fd]->pair_fd << endl;
                                break;
                            }
                            else
                            {
                                ++it;
                            }
                            ++index;
                        }
                        // master通知所有cache，将原本存ip_port的数据里，cache_1的位置更新为cache_2的位置
                        //------------------------------------------------------
                        // C#origin_ip#origin_port#backup_ip#backup_port
                        //------------------------------------------------------
                        //C#origin_ip:origin_port#backup_ip:backup_cache
                        string tmpstr = caches_list[fd]->ip_cache;
                        string str1 = tmpstr.replace(caches_list[fd]->ip_cache.find("#"), 1, ":");
                        tmpstr = caches_list[caches_list[fd]->pair_fd]->ip_cache;
                        string str2 = tmpstr.replace(caches_list[caches_list[fd]->pair_fd]->ip_cache.find("#"), 1, ":");
                        // string toAllCacheMsg = "C#"+caches_list[fd]->ip_cache+caches_list[caches_list[fd]->pair_fd]->ip_cache;
                        string toAllCacheMsg = "C#" + str1 + "#" + str2;
                        // 然后把这个msg发给所有的cache====广播
                        cout << "send msg \'" << toAllCacheMsg << "\' to all cache" << endl;
                        for (auto fdi : fd_node)
                        { //将新加入的cache的ip和port发给所有的cache
                            cout << "the fd is " << fdi << endl;
                            strcpy(send_buff_disaster, toAllCacheMsg.c_str());
                            send(fdi, send_buff_disaster, BUF_SIZE, 0);
                        }
                        // 本地master的其他配置
                        caches_list[caches_list[fd]->pair_fd]->pair_fd = -1; // 配偶清空
                        caches_list[caches_list[fd]->pair_fd]->status = 'P'; //备份变主
                        pcache.push(caches_list[fd]->pair_fd);               //待配对状态
                        close(fd);
                    }
                    else
                    {
                        // 如果cache_1没有备份cache，则
                        // 更改本地fd_node
                        cout << "the main cache has not backup cache" << endl;
                        int index = 0;
                        vector<int>::iterator it = fd_node.begin();
                        for (it = fd_node.begin(); it != fd_node.end();)
                        {
                            if (*it == fd)
                            {
                                it = fd_node.erase(it); //删除
                                cacheAddrHash.deleteNode(index);
                                cout << "delete fd " << fd << ", and fd_node size is " << fd_node.size() << endl;
                                break;
                            }
                            else
                            {
                                ++it;
                            }
                            ++index;
                        }

                        //master通知所有cache，将原本存ip_port的数据里,cache_1的数据删除，
                        //------------------------------------------------------
                        // D#delete_ip#delete_port
                        //------------------------------------------------------
                        string toAllCacheDeleteMsg = "D#" + caches_list[fd]->ip_cache;
                        cout << "send msg \'" << toAllCacheDeleteMsg << "\' to all cache" << endl;
                        for (auto fdi : fd_node)
                        {
                            // cout << "the fd is " << fdi << endl;
                            strcpy(send_buff_disaster, toAllCacheDeleteMsg.c_str());
                            send(fdi, send_buff_disaster, BUF_SIZE, 0);
                        }
                        // 本地操作
                        close(fd);
                    }
                }
                
                break;
            }
        }
        if(deletefd>0){
            caches_list.erase(deletefd);
        }
        memset(send_buff_disaster, 0, sizeof(send_buff_disaster));
        sleep(1);
    }
}