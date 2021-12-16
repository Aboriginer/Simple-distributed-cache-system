# Cache端所有需测试的功能

需改进：

- 打印的信息太频繁，需要改进(cache和master均需改进)

cached端待测试：

- cache心跳包：
  - ~~init后N#ip#port加入unique功能~~
  - recv ReadFromMaster
    - ~~P#None~~
    - ~~init~~
    - R#target_ip#target_port
    - K#ip#port
    - C#origin_ip:origin_port#backup_ip:backup_cache
    - D#delete_ip#delete_port
- 与master通信的接收信息部分的测试（1.init 2.接收扩缩容信息 3.接收容灾信息）
  - ~~init读取（接收N和init信息的顺序）~~
  - 测试收到的扩缩容信息是否正确
  - 测试收到的容灾信息是否正确
- 扩缩容
  - 扩容时所有cache中的cache_list是否更新
  - 缩容传递的key#value是否正确，以及是否写到备份中
  - 缩容是否kill掉主cache和备份cache
- 容灾
  - client写入读取的key#value更新是否正确
  - 传递的cache_list是否正确(扩容N和缩容K）
- 考察点1的单机LRU淘汰功能如何演示？

已完成：

- client写入和读取key的功能



# 目前测试遇到的bug

~~1.cache init后会把自己再加到cache_list中，需要unique~~

解决：目前已在update_cache中判断IP和port是否为本机，是的话不加入cache_list

~~2.cache启动多个主cache的时候有的时候会卡住（只开心跳和Cient_chat函数）~~

<img src="https://s2.loli.net/2021/12/16/NLgQ4XZqwnCF7dB.png" alt="image-20211216112825177" style="zoom:50%;" />

~~3.cache心跳包函数，将send和recv写在一起，因为是定时发送心跳包，发送心跳定时时间长(1000ms)的话导致会接收不到master发的信息，定时时间短(100ms)的话可以接收到master发送的信息，但是cache有很大机率会卡住不动~~

<img src="https://s2.loli.net/2021/12/16/EBPzXrfe5SgMF6J.png" alt="image-20211216121647894" style="zoom:50%;" />

解决：修改定时器