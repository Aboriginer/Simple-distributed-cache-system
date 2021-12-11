# Cache Server communication

## 编译方法
g++ -pthread cache_main.cpp cache.cpp cache.h -o cache_main 

## 运行方法

testMaster无参数，testClient需指定1.主从地位，2.本地IP, 3.给client的端口, 4.给其他Cache Server的端口(eg. ./testClient Primary 127.0.0.1 8887 8888)

## 与client通信格式

### client读请求

接收：key#value

发送(TODO)：ip:port#state#key(port为监听端口，state = SUCCESS/FAILED)

### client写请求

接收：key

发送(TODO)：ip:port#state#key#value

## 与master通信格式

发送：心跳 + ip/port + time(TODO)

接收(TODO)：扩缩容信息

