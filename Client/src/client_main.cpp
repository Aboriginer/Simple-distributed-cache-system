#include <getopt.h>

#include "client.hpp"


int main(int argc, char *argv[]) {
    int local_cache_capacity = 0;  //本地cache容量
    char mode;

    int c;

    static struct option long_options[] = {
        {"capacity", required_argument,NULL,'c'},
        {"write",    no_argument,      NULL,'w'},
        {"read",     no_argument,      NULL,'r'}
    };

    while(1) {
        int opt_index = 0;
        c = getopt_long(argc, argv,"c:wr", long_options,&opt_index);

        if(-1 == c) {
            break;
        }
        switch(c) {
            case 'c':
                local_cache_capacity = atoi(optarg);
                break;
            case 'w':
                std::cout << "Write mode" << std::endl;
                mode = 'w';
                break;
            case 'r':
                std::cout << "Read mode" << std::endl;
                mode = 'r';
                break;
            default:
                std::cout << "???" << std::endl;
                break;
        }
    }

    Client client(local_cache_capacity);
    client.Start();
    if (mode == 'w') {
        client.Write();
    } else if (mode == 'r'){
        client.Read();
    }
}
