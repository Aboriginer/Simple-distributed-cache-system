#include "LRU_cache.hpp"
#include "threadPool.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>

static const int MaxCapcacity = 10;
static const int ThreadNum = 5;
    

void future_check(std::future<int>& future) {
    try {
        //future.
        future.get();
    }
    catch (const std::future_error& e) {
        std::cout << "Caught a future_error with code \"" << e.code()
            << "\"\nMessage: \"" << e.what() << "\"\n";
            //<< "i = " << i << std::endl;
    }
}

int main() {
    //主内存
    LRU_Cache<std::string, std::string> MainCache(MaxCapcacity);
    //线程池
    ThreadPool MainPool(ThreadNum);
    std::vector<std::string> key = {"1","2","3","4","5"};
    std::vector<std::string> val = {"a","b","c","d","e"};
    std::vector<std::future<int>>futures;
    //一个简单的lambda函数
    auto hello = []() -> int{
        std::cout << "shihfguiodhuighufi" << std::endl;
        return 0;
    };

    //对内存写入数据
    auto CachePut = [&](int pos) -> int {
        std::string k = key[pos];
        std::string v = val[pos];



        MainCache.put(k, v); 
        std::cout<<"putting key & value :" << k<<" "<<v<<std::endl; 
        //std::cout << "found value = " << MainCache.get(key[pos]) << std::endl;
        return 1;
    };

    //对内存读出数据
    auto CacheGet = [&](int pos) -> int {
       
        /*try {*/
            std::cout << "found value = " << MainCache.get(key[pos]) << std::endl;
        //}
        /*catch(std::exception){
            std::cout << "nothing found" << std::endl;
        }
       */
            
        return 0;
    };

    CachePut(1);
    CacheGet(1);

    //for(int i = 0; i < key.size(); i++){
        auto future0 = MainPool.enqueue(hello);
        //future_check(future0);
        auto future1 = MainPool.enqueue(CachePut, 2);
        auto future2 = MainPool.enqueue(CacheGet, 2);
        
        futures.emplace_back(std::move(future0));
        futures.emplace_back(std::move(future1));
        futures.emplace_back(std::move(future2));
    //}
    
    int i = 0;
    for (auto &future : futures) {
        i++;
        future_check(future);
    }

    return 0;

}