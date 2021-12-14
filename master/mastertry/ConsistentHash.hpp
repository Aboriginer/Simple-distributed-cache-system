#ifndef CONSISTENTHASH_H
#define CONSISTENTHASH_H
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
using namespace std;
class ConsistentHash
{
public:
    ConsistentHash();
    ~ConsistentHash();

    void initialize(const int nodes, const int v_nodes);
    size_t GetServer(const char* key);
    uint32_t murmur3_32(string s, int len);

    void deleteNode(const int index);
    void addNode(const int index);
    map<uint32_t, size_t> key2Index;
private:
    int nodes;
    int v_nodes;
};

#endif
