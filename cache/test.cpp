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
    std::string buffer;
    std::mutex mutex;
    //主内存
    LRU_Cache<std::string, std::string> MainCache(MaxCapcacity);
    //线程池
    ThreadPool MainPool(ThreadNum);
    std::vector<std::string> key = {"1","2","3","4","5"};
    std::vector<std::string> val = {"a","b","c","d","e"};
    std::queue<std::future<int>>futures;
 
    //一个简单的lambda函数
    auto hello = [&]() -> int{
        //std::cout << "shihfguiodhuighufi" << std::endl;
        //mutex.lock();
        buffer.clear();
        buffer = "hello";
        //mutex.unlock();
        return 0;
    };

    //对内存写入数据
    auto CachePut = [&](int pos) -> int {
        std::lock_guard<std::mutex> lg(mutex);
        std::string k = key[pos];
        std::string v = val[pos];



        MainCache.put(k, v); 
        //std::cout<<"putting key & value :" << k<<" "<<v<<std::endl; 
        //std::cout << "found value = " << MainCache.get(key[pos]) << std::endl;
        
        buffer.clear();
        buffer = k +"#" + v;
        return 1;
    };

    //对内存读出数据
    auto CacheGet = [&](int pos) -> int {
       
        /*try {*/
        std::lock_guard<std::mutex> lg(mutex);
        std::string str = MainCache.get(key[pos]);
        //}
        /*catch(std::exception){
            std::cout << "nothing found" << std::endl;
        }
       */
        
        buffer.clear();
        buffer = "success = "+str;
        //mutex[1].unlock();
            
        return 0;
    };

 /*   CachePut(1);
    CacheGet(1);*/
    for (int i = 0; i < 10; i++) {
        auto future1 = MainPool.enqueue(CachePut, 2);
        auto future2 = MainPool.enqueue(CacheGet, 2);
        futures.emplace(std::move(future1));
        futures.emplace(std::move(future2));
        future_check(futures.front());
        std::cout << buffer << std::endl;
        futures.pop();
    }

    //for(int i = 0; i < key.size(); i++){
    //    auto future0 = MainPool.enqueue(hello);
    //    //future_check(future0);
    //    auto future1 = MainPool.enqueue(CachePut, 2);
    //   
    //    auto future2 = MainPool.enqueue(CacheGet, 2);
    //    
    //    futures.emplace_back(std::move(future0));
    //    std::cout << "buffer = " << buffer << std::endl;
    //    futures.emplace_back(std::move(future1));
    //    std::cout << "buffer = " << buffer << std::endl;
    //    futures.emplace_back(std::move(future2));
    //    std::cout << "buffer = " << buffer << std::endl;
    ////}
    //
    //int i = 0;
    //for (auto &future : futures) {
    //    i++;
    //    future_check(future);
    //    std::cout << "buffer = " << buffer << std::endl;
    //}

    return 0;

}