#ifndef REQUESTHASHTABLE_H
#define REQUESTHASHTABLE_H

#include <cstddef>
#include <vector>

#include "ReliefRequest.h"

class RequestHashTable
{
private:
    struct Node
    {
        ReliefRequest request;
        Node *next;

        explicit Node(const ReliefRequest &value) : request(value), next(NULL) {}
    };

    std::vector<Node *> buckets;
    std::size_t entryCount;

    std::size_t bucketIndex(int id) const;

public:
    explicit RequestHashTable(std::size_t bucketCount = 101);
    ~RequestHashTable();

    RequestHashTable(const RequestHashTable &) = delete;
    RequestHashTable &operator=(const RequestHashTable &) = delete;

    bool insert(const ReliefRequest &request);
    ReliefRequest *find(int id);
    const ReliefRequest *find(int id) const;
    bool erase(int id);
    bool isEmpty() const;
    std::size_t size() const;
    std::vector<ReliefRequest> getAll() const;
};

#endif
