# Cache Server communication

## 编译方法
 g++ -pthread cache_main.cpp cache.cpp cache.h threadPool.hpp LRU_cache.hpp Timer.hpp Timer.cpp ConsistentHash.cpp ConsistentHash.hpp -o cache_main

(有空写个makefile)
## TODO
cache从master接收的扩缩容容灾等所有的信息和发送给master的心跳包只能用同一个socket(这些函数只能开在一个线程中)

## 加入容灾后的测试方法
```shell
1.启动testMaster
.testMaster
2.启动主备cache
./cache_main P 127.0.0.1 8887 8888
./cache_main R 127.0.0.1 8897 8898
3.启动testClient
./testClient
```

## 运行方法

testMaster无参数，cache_main需指定1.主从地位，2.本地IP, 3.给client的端口, 4.给其他Cache Server的端口(eg. ./cache_main P 127.0.0.1 8887 8888)

## 与client通信格式

接收：key#value或key

发送：SUCCESS/FAILED#key#ip:port

## 与master通信格式

发送：x#local_IP#local_port_for_client#local_port#for_cache#P/R

从master接收所有的信息如下：
- 扩容：
    - 1.master向新上线的cache单独发送现有的所有cache的IP和port，通信格式：F#ip1#port1#ip2#port2#ip3#port3
    - 2.master向所有cache广播新上线cache的IP和port，通信格式：N#ip#port
- 缩容：
    - 1.master向要所有的cache发送K#killed_ip#killed_port
- 容灾：
    - 1.P/R#target_ip#target_port