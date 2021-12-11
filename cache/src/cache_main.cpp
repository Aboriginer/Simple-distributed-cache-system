#include "cache.h"

/* argv[1]: SERV_CLIENT_CACHE_SEVER_PORT 与client通信的端口
 * argv[2]: SERV_CACHE_SEVER_PORT 与其他cache server通信的端口
 * eg. ./cache_main 8887 8888
 */

std::string port_for_client, port_for_cache;

int main(int argc, char *argv[]) {

    if (argc != 5) {
        std::cout << "Usage :" << argv[0] << " <port>" << std::endl;
        exit(-1);
    }

    std::string status =  argv[1];
    std::string local_cache_IP =  argv[2];
    std::string port_for_client = argv[3];
    std::string port_for_cache = argv[4];

    Cache cache(10, status, local_cache_IP, port_for_client, port_for_cache);
    cache.Start();

    return 0;
}
