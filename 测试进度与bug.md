# Cache端所有需测试的功能

需改进：

- 修改打印信息，使在演示时展示更友好

cached端待测试：

- cache心跳包：
  - ~~init后N#ip#port加入unique功能~~
  - ~~recv ReadFromMaster~~
    - ~~P#None~~
    - ~~init~~
    - ~~R#target_ip#target_port~~
    - ~~K#ip#port~~
    - ~~C#origin_ip:origin_port#backup_ip:backup_cache~~
    - ~~D#delete_ip#delete_port~~
- ~~与master通信的接收信息部分的测试（1.init 2.接收扩缩容信息 3.接收容灾信息）~~
  - ~~init读取（接收N和init信息的顺序）~~
  - ~~测试收到的扩容信息是否正确~~
  - ~~测试收到的缩容信息是否正确~~
  - ~~测试收到的容灾信息是否正确~~
- 扩缩容
  - ~~扩容时所有cache中的cache_list是否更新~~
  - 缩容传递的key#value是否正确，以及是否写到备份中
  - 缩容是否kill掉~~主cache~~和备份cache
- 容灾
  - ~~主cache宕机后，备份cache转正~~
  - 容灾情况发送后client写入读取的key#value更新是否正确
  - 容灾情况发送后传递的cache_list是否正确(扩容N和缩容K）
- ~~考察点1的单机LRU淘汰功能如何演示？~~

已完成：

- client写入和读取key的功能



# 目前测试遇到的bug

