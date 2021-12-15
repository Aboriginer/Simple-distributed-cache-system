# from client:
## 读/写请求：(string) 
    接收："key"
    返回："MASTER#key#ip:port"

# from cache:
## 心跳包：(string) 
    接收："x#local_cache_IP#port_for_client#port_for_cache#P/R"，
    返回：  P/R#None                             配对不成功
            P/R#r/pcache_IP#port_for_cache       配对成功
## 扩容
### 向新上线cache单发所有主cache:
    发送："ip1#cache_port1#ip2#cache_port2#..."
### 广播给所有主cache:
    广播发送："N#ipn#cache_portn"

## 缩容
    广播发送："K#killed_ip#killed_cache_port"


