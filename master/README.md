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

容灾：
主cache掉线，
1 主cache有备份cache
master通知备份cache变为主cache，并同步所有cache的ipport，通信格式：B#ip1#port1#ip2#port2#...
master通知所有cache，将原本存主cache的ip_port的数据更新为备份cache的ip_port,通信格式：C#origin_ip#origin_port#backup_ip#backup_port
2 主cache没有备份cache：master通知所有cache，将原本存ip_port的数据里,cache_1的数据删除，通信格式：D#delete_ip#delete_port

