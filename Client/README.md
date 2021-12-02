##	Client

### 运行方法

testMaster和testCacheServer无参数；Client -c 指定本地cache容量，-w/-r指定写/读模式（eg: **./Client -c 100 -w**）// 目前仅写模式能用

### 与cache server通信格式

#### 写请求

发送：key#value

接收：ip:port#state#key(port为监听端口，state = SUCCESS/FAILED)

#### 读请求

发送：key

接收：ip:port#状态#key#value

### 与master通信格式

发送：key#REQUEST

接收：key#ip:port

