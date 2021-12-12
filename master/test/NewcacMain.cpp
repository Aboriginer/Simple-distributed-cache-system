#include "Newcli.h"

// 服务端主函数
// 创建服务端对象后启动服务端
int main(int argc, char *argv[]) {
    nClient server1(CACHE_PORT);
    server1.Start();
    return 0;
}