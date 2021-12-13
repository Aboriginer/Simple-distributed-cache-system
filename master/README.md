# from client:
## 读/写请求：(string) 
    格式："key"
    返回："MASTER#key#ip:port"

# from cache:
## 心跳包：(string) 
    格式："x#127.0.0.1#8867#R(/P)"，其中ip:port是cache_server的套接字地址
    返回：   P/R#none                   配对不成功
            P/R#127.0.0.1#8887         配对成功
    
