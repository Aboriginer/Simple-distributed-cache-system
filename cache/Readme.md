# Cache Server communication

## 编译方法
g++ -pthread cache_main.cpp cache.cpp cache.h -o cache_main 

## 运行方法

testMaster无参数，testClient需指定1.主从地位，2.本地IP, 3.给client的端口, 4.给其他Cache Server的端口(eg. ./testClient Primary 127.0.0.1 8887 8888)

## 与client通信格式

### client读请求

接收：key#value

发送：state#key#ip:port(port为监听端口，state = SUCCESS/FAILED)

### client写请求

接收：key

发送：key#value

## 与master通信格式

发送：x#ip:port#status

接收：P/R#ip1#port1#Y#ip2#port2#key#key#key···

