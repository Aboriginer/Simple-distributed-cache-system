# Client

## 运行方法

1. 运行build.sh进行编译
2. 当前目录会生成可执行文件client
3. client -c 指定本地cache容量，-w/-r指定写/读模式（eg: **./client -c 100 -w**）

读模式逐行读取key_list文件作为key，写模式随机生成key和value
