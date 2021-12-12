# from client:
## 读/写请求：(string) 
    格式："key"
    返回："MASTER#key#ip:port"

# from cache:
## 心跳包：(string) 
    格式："x#ip#port"，其中ip:port是cache_server的套接字地址
    返回：None

## 更新包：(string) 
    格式："g#ip#port#key"，其中ip:port是cache_server的套接字地址
    返回：None