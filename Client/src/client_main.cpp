#include <getopt.h>
#include "client.hpp"

// #define ELPP_UNICODE  						//使用Unicode字符集
// #define ELPP_THREAD_SAFE          //多线程支持
#include "easylogging++.h"


INITIALIZE_EASYLOGGINGPP  //log初始化

int main(int argc, char *argv[]) {
	el::Configurations conf("log.conf");
	el::Loggers::reconfigureLogger("default", conf);
	el::Loggers::reconfigureAllLoggers(conf);

	LOG(INFO) << "Client start";

	int local_cache_capacity = 0; //本地cache容量
	char mode;

	int c;

	static struct option long_options[] = {
			{"capacity", required_argument, NULL, 'c'},
			{"write", no_argument, NULL, 'w'},
			{"read", no_argument, NULL, 'r'}};

	while (1) {
		int opt_index = 0;
		c = getopt_long(argc, argv, "c:wr", long_options, &opt_index);

		if (-1 == c) {
			break;
		}
		switch (c) {
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
	//TODO：读取config文件
	Client client(local_cache_capacity, mode);
	client.init();
	client.start();
}
