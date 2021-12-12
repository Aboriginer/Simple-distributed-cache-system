#include <iostream>
#include <unordered_map>
#include <mutex>

template<typename T>
class null {
    null() = default;
};

template<typename T1, typename T2>
class DNode{
public:
    DNode *prev;
    DNode *next;
    T1 key;
    T2 val;
    //类的构造函数
    DNode():prev(nullptr),next(nullptr){};
    DNode(T1 key__,T2 val__): key(key__), val(val__),prev(nullptr),next(nullptr){};
};

template<typename T1, typename T2>
class LRU_Cache{
//LRU缓存的实现：依靠双向链表和哈希表
private:
    std::unordered_map<T1,DNode<T1, T2>* > map;
    //map分别存储节点的频率和它的内容。
    DNode<T1, T2> *head, *tail;       //头部节点和尾节点
    std::mutex mutex;               //LRU链表的锁
    int size;
    int capacity;
public:
    LRU_Cache(int capacity__);//构造函数
    const T2 get(T1 key);         //获取关键字的值
    bool check(T1 key);     //查找值是否存在
    T1 put(T1 key,T2 val);//变更数据值，或者插入关键字-值对。
    void del(T1 key);
//    vector<vector<T>> show_all();
private:
    void add2head(DNode<T1, T2> *node);
    void move2head(DNode<T1, T2> *node);
    void remove(DNode<T1, T2> *node);
    DNode<T1, T2>* remove_tail();
};

template<typename T1, typename T2>
LRU_Cache<T1, T2>::LRU_Cache(int capacity__){
    capacity = capacity__;
    size = 0;
    //使用伪头部和伪尾部节点
    head = new DNode<T1,T2>();
    tail = new DNode<T1,T2>();
    head->next = tail;
    tail->prev = head;
}

//^ 主要功能：获取关键字的值
template<typename T1, typename T2>
const T2 LRU_Cache<T1,T2>::get(T1 key){
    mutex.lock();
    if(map.count(key) > 0){
        auto node = map[key];
        move2head(node);
        mutex.unlock();
        return node->val;
    }else{
        mutex.unlock();
        const T2 temp;
        return temp ;
    }
}

//template<typename T1, typename T2>
//std::vector<std::vector<>> show_all() {
//    std::vector<std::vector<T>> res;
//    for(auto it : map){
//        res.push_back({it.first(), it.second()});
//    }
//}


//^ 主要功能：变更或插入关键字
template<typename T1, typename T2>
T1 LRU_Cache<T1,T2>::put(T1 key, T2 val){
    //加锁
    mutex.lock();
    T1 removed_key;
    if(map.count(key) > 0){
        //如果链表中存在key，那么则直接调出这个值，并将其移动到链表的头部
        //找出key
        auto node = map[key];
        //移动到头部
        move2head(node);
        //更改值为val
        node->val = val;
        //解锁
        mutex.unlock();
        return removed_key;
    }else{
        DNode<T1,T2>* node = new DNode<T1,T2> (key,val);
        map[key] = node;
        add2head(node);
        size ++;
        if(size > capacity){
            // 删除尾部节点
            DNode<T1,T2> *remove = remove_tail();
            map.erase(remove->key);
            removed_key = remove->key;
            delete remove;
            size --;
        }
        //解锁
        mutex.unlock();
        return removed_key;
    }
}

//^ 检查是否存在值
template<typename T1, typename T2>
bool LRU_Cache<T1, T2>::check(T1 key){
    return map.count(key) > 0;
}

//^ 删除相应节点
template<typename T1, typename T2>
void LRU_Cache<T1, T2>::del(T1 key){
    mutex.lock();
    DNode<T1, T2> *node = map[key];
    remove(node);
    auto it = map[key];
    map.erase(it);
    mutex.unlock();
}

//?--------------------------------------------------------------
//? 辅助性函数

// 将当前节点添加至头部
template<typename T1, typename T2>
void LRU_Cache<T1,T2>::add2head(DNode<T1,T2> *node){
    node->prev = head;
    node->next = head->next;
    head->next->prev = node;
    head->next = node;
}


// 将当前节点移动到头部
template<typename T1, typename T2>
void LRU_Cache<T1,T2>::move2head(DNode<T1,T2>* node){
    remove(node);
    add2head(node);
}

// 删除尾部节点
template<typename T1, typename T2>
DNode<T1,T2>* LRU_Cache<T1,T2>::remove_tail(){
    DNode<T1,T2>* node = tail->prev;
    remove(node);
    return node;
}


//删除当前节点
template<typename T1, typename T2>
void LRU_Cache<T1,T2>::remove(DNode<T1,T2> *node){
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

