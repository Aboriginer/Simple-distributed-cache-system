#include <getopt.h>

#include "client.h"
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
	el::Configurations conf("./log.conf");
	el::Loggers::reconfigureLogger("default", conf);
	el::Loggers::reconfigureAllLoggers(conf);

	LOG(INFO) << "Client start";

	int local_cache_capacity = 0;  // 客户端本地cache容量
	char mode; 
	bool from_file = false;
	int c;
	int time_interval = 1000;  // 产生请求的时间间隔
	static struct option long_options[] = {
			{"capacity", required_argument, nullptr, 'c'},
			{"write", no_argument, nullptr, 'w'},
			{"read", no_argument, nullptr, 'r'},
			{"from_file", no_argument, nullptr, 'f'},
			{"time", no_argument, nullptr, 't'}};
	// 命令行解析
	while (1) {
		int opt_index = 0;
		c = getopt_long(argc, argv, "c:wrft:", long_options, &opt_index);

		if (-1 == c) {
			break;
		}
		switch (c) {
		case 'c':
			local_cache_capacity = atoi(optarg);
			break;
		case 'w':
			std::cout << "Write mode ";
			mode = 'w';
			break;
		case 'r':
			std::cout << "Read mode ";
			mode = 'r';
			break;
		case 'f':
			std::cout << "from local file  ";
			from_file = true;
			break;
		case 't':
			time_interval = atoi(optarg);
			std::cout << "time interval: " << optarg <<	" ms" << std::endl;
			break;
		default:
			std::cout << "???" << std::endl;
			break;
		}
	}

	Client client(local_cache_capacity, mode, from_file, time_interval);
	client.init();
	client.start();
}
