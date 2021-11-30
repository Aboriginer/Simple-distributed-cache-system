#pragma once
#include <unordered_map>
#include <mutex>

template<typename T1, typename T2>
class DNode{ 
public:
    DNode *prev;
    DNode *next;
    T1 key;
    T2 val;
    //类的构造函数
    DNode(): key(0),val(0),prev(nullptr),next(nullptr){};
    DNode(T1 key__,T2 val__): key(key__), val(val__),prev(nullptr),next(nullptr){};
};

template<typename T1, typename T2>
class LRU_Cache{
//LRU缓存的实现：依靠双向链表和哈希表
    std::unordered_map<int,DNode<T1, T2>* > map;
    //map分别存储节点的频率和它的内容。
    DNode<T1, T2> *head, *tail;       //头部节点和尾节点
    int size;
    int capacity;
public:
    LRU_Cache(int capacity__);//构造函数
    const T2 get(T1 key);         //获取关键字的值
    void put(T1 key,T2 val);//变更数据值，或者插入关键字-值对。
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
    head = new DNode();
    tail = new DNode();
    head->next = tail;
    tail->prev = head;
}

//^ 主要功能：获取关键字的值
template<typename T1, typename T2>
const T2 LRU_Cache<T1,T2>::get(T1 key){
    if(map.count(key) > 0){
        DNode *node = map[key];
        move2head(node);
        return node->val;
    }else{
        return -1;
    }
}


//^ 主要功能：变更或插入关键字
template<typename T1, typename T2>
void LRU_Cache<T1,T2>::put(T1 key, T2 val){
    if(map.count(key) > 0){
        //如果链表中存在key，那么则直接调出这个值，并将其移动到链表的头部
        //找出key
        DNode *node = map[key];
        //移动到头部
        move2head(node);
        //更改值为val
        node->val = val;
        return;
    }else{
        DNode<T1,T2>* node = new DNode(key,val);
        map[key] = node;
        add2head(node);
        size ++;
        if(size > capacity){
            // 删除尾部节点
            DNode *remove = remove_tail();
            map.erase(key);
            delete remove;
            size --;
        }
    }
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

