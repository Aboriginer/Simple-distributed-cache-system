#include "Newcli.h"

// 服务端主函数
// 创建服务端对象后启动服务端
int main(int argc, char *argv[]) {
    nClient server2(CLIENT_PORT);
    server2.Start();
    return 0;
}