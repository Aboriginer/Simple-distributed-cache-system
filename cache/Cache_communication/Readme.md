# Cache Server communication

## 运行方法

testMaster无参数，testClient需指定Cache Server端口(eg. ./testClient 8887)

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

