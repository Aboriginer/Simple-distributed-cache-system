#include <future>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <iostream>

#pragma once
//using namespace std;


class ThreadPool {
private:
    std::vector<std::thread> threads;             //开辟的线程
    std::queue<std::function<void(void)>> tasks;  //将要处理的任务

    //互斥锁和条件变量
    std::mutex mutex;
    std:: condition_variable __cv;

    //线程池的参数
    bool isPoolStoped; //指明线程池是否停止
    uint32_t numActiveThreads;  //活跃线程的个数
    const uint32_t capacity;    //最大线程数

    //线程的工厂模板
    template< typename Func,            //函数名
              typename ...Args,         //函数的参数
              typename Rtrn = typename std::result_of<Func(Args...)>::type>      //函数的返回值
    auto taskBuilder(
        Func && func,
        Args &&... args) -> std::packaged_task<Rtrn(void)> {
            auto aux = std::bind(std::forward<Func>(func),
                                 std::forward<Args>(args)...);
            return std::packaged_task<Rtrn(void)> (aux);
        }


    void before_task_hook(){
        numActiveThreads ++;
    }


    void after_task_hook() {
        numActiveThreads --;
    }
public:
    ThreadPool(uint64_t capacity_) : 
    isPoolStoped(false), 
    numActiveThreads(0), 
    capacity(capacity_){
        

        //主循环
        auto wait_loop = [this]()->void {
            
            while(true) {  
                std::function<void(void)> task;

                {
                    std::unique_lock<std::mutex> ulock(mutex);
                    
                    //if (tasks.empty()) std::cout << "empty" << std::endl;
                    auto predicate = [this]() -> bool {
                        return (isPoolStoped) || !(tasks.empty());
                    };

                    //printf("hello from thread\n");
                    __cv.wait(ulock,predicate);
                    //printf("hello thread\n");

                    if (isPoolStoped && tasks.empty()) {
                        return;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();
                    before_task_hook();
                }
                
                std::cout << "running" << std::endl;
                task();

                {
                    std::lock_guard<std::mutex> lock_guard(mutex);
                    after_task_hook();
                }


            }



        };
        for( uint64_t id = 0; id < capacity; id++){
            threads.emplace_back(wait_loop);
            
        }

    }


    ~ThreadPool() {
        //初始化互斥锁
        {
            std::lock_guard<std::mutex> lock_guard(mutex);
            isPoolStoped = true;
        }
        //通知所有锁
        __cv.notify_all();
        //合并所有线程
        for(auto &thread : threads) {
            thread.join();
        }
    }

    template<typename    Func,
         typename ...Args,
         typename Rtrn = typename std::result_of<Func(Args...)>::type>
    auto enqueue(Func && func, Args &&...args) -> std::future<Rtrn> {
            auto task = taskBuilder(func, args...);
            auto future = task.get_future());
            auto task_ptr = std::make_shared<decltype(task)> (move(task));

            
            std::lock_guard<std::mutex> lock_guard(mutex);
            if (isPoolStoped) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
                }
            auto payload = [task_ptr] () -> void {
                 task_ptr ->operator()();
            } ;

            tasks.emplace(payload);
            __cv.notify_one();
            return future;
            }

  

    
};


