1.Client_chat中的epoll在没有client接入时，有的时候会出现epoll failure：Interrupted system call，导致master那边报段错误

[epoll-wait-fails-due-to-eintr-how-to-remedy-this](https://stackoverflow.com/questions/6870158/epoll-wait-fails-due-to-eintr-how-to-remedy-this)

```c++
       // Client_chat代码 
	   int epoll_events_count = epoll_wait(epfd, client_events, EPOLL_SIZE, -1);

        if (epoll_events_count < 0) {
            perror("epoll failure");
//            break;
            continue;
        }
```

<img src="https://s2.loli.net/2021/12/15/EcqwrdlbOQBVLDu.png" alt="80" style="zoom:80%;" />

master显示的段错误如下，

<img src="https://s2.loli.net/2021/12/15/th3GAzwMgB8RTjJ.png" alt="image-20211215020225825" style="zoom:80%;" />



2.新上线cache接收master发送的所有IP和port未配对，尚未明确是Master发送的信息为配对还是本地解析错误

initial代码暂时先注释

```C++
//        while (!initial_flag) {
//            initial(cache_master_sock, recv_buff_master);
//            if(is_initialed == NO_INIT){
//                std::cout<<"ERROR: the cache is not initiated. "<<std::endl;
//                exit(EXIT_FAILURE);
//            }else if(is_initialed == ERROR_INIT){
//                std::cout<<"ERROR: something wrong when initiating the cache."<<std::endl;
//                exit(EXIT_FAILURE);
//            }
//            initial_flag = true;
//        }
```

<img src="C:/Users/Aboriginer/AppData/Roaming/Typora/typora-user-images/image-20211215021400334.png" alt="image-20211215021400334" style="zoom: 67%;" />



3.扩缩容cache_pass通信写的不对

4.容灾replica_chat的cache_list没输入





1.用time不行，while可以
好像只改了master的continue就行?

2.init的信息发送顺序

3.port_for_cache_好像没传给master