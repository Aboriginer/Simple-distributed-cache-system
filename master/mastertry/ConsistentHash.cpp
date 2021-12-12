#include "ConsistentHash.hpp"
#include <sstream>
#include <cstring>
using namespace std;

ConsistentHash::ConsistentHash()
{
    // nodes = _nodes;
    // v_nodes = _v_nodes;
}
ConsistentHash::~ConsistentHash()
{
    key2Index.clear();
}
uint32_t ConsistentHash::murmur3_32(string s, int len){
    const int p = 16777619;
    uint32_t hash = 2166136261LL;
    for (int idx = 0; idx < s.size(); ++idx) {
        hash = (hash ^ s[idx]) * p;
    }
    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    if (hash < 0) {
        hash = -hash;
    }
    return hash;
}
void ConsistentHash::initialize(int _nodes, int _v_nodes)
{
    for(int i = 0; i < nodes; ++i) //i:ip
    {
        for(int j = 0; j < v_nodes; ++j) 
        {
            stringstream s;
            s << "SHARD-" << i << "NODE-" << j;//shard
            uint32_t key1 = murmur3_32(s.str().c_str(), strlen(s.str().c_str()));
            key2Index.insert(pair<uint32_t, size_t>(key1, i));
        }
    }
}
size_t ConsistentHash::GetServer(const char* key)
{
    uint32_t key1 = murmur3_32(key, strlen(key));
    map<uint32_t, size_t>::iterator it = key2Index.lower_bound(key1);
    if(it == key2Index.end())
    {
        return key2Index.begin()->second;
    }
    return it->second;
}
void ConsistentHash::deleteNode(const int index)
{
    for(int i = 0; i < v_nodes; ++i){
        stringstream s;
        s << "SHARD-" << index << "NODE-" << i;
        uint32_t key = murmur3_32(s.str().c_str(), strlen(s.str().c_str()));
        map<uint32_t, size_t>::iterator it = key2Index.find(key);
        if(it != key2Index.end())
        {
            key2Index.erase(it);
        }
    }
}
void ConsistentHash::addNode(const int index)
{
    for(int i = 0; i < v_nodes; ++i){
        stringstream s;
        s << "SHARD-" << index << "NODE-" << i;
        uint32_t key = murmur3_32(s.str().c_str(), strlen(s.str().c_str()));
        key2Index.insert(pair<uint32_t, size_t>(key, index));
    }
}