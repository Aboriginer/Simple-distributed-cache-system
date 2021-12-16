# Client

## 运行方法

1. 运行build.sh进行编译
2. 当前目录会生成可执行文件client
3. client -c 指定本地cache容量，-w/-r指定写/读模式 -f指定是否从本地文件读取key -t指定产生随机请求的时间间隔，单位ms（eg: **./client -c 100 -w -t 1000**写模式，随机生成key，时间间隔1s）（eg: **./client -c 100 -w -f -t 50**写模式，从本地文件中读取key, 时间间隔50ms）
