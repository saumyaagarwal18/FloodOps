#include "RequestHashTable.h"

RequestHashTable::RequestHashTable(std::size_t bucketCount)
    : buckets(bucketCount == 0 ? 1 : bucketCount, NULL), entryCount(0)
{
}

RequestHashTable::~RequestHashTable()
{
    for (std::size_t i = 0; i < buckets.size(); ++i)
    {
        Node *node = buckets[i];
        while (node != NULL)
        {
            Node *next = node->next;
            delete node;
            node = next;
        }
    }
}

std::size_t RequestHashTable::bucketIndex(int id) const
{
    const unsigned int key = static_cast<unsigned int>(id);
    return static_cast<std::size_t>(key) % buckets.size();
}

bool RequestHashTable::insert(const ReliefRequest &request)
{
    if (request.id < 0 || request.people <= 0 ||
        (request.priority != 0 && request.priority != 1) ||
        find(request.id) != NULL)
    {
        return false;
    }

    const std::size_t index = bucketIndex(request.id);
    Node *node = new Node(request);
    node->next = buckets[index];
    buckets[index] = node;
    ++entryCount;
    return true;
}

ReliefRequest *RequestHashTable::find(int id)
{
    if (id < 0)
    {
        return NULL;
    }

    Node *node = buckets[bucketIndex(id)];
    while (node != NULL)
    {
        if (node->request.id == id)
        {
            return &node->request;
        }
        node = node->next;
    }
    return NULL;
}

const ReliefRequest *RequestHashTable::find(int id) const
{
    if (id < 0)
    {
        return NULL;
    }

    const Node *node = buckets[bucketIndex(id)];
    while (node != NULL)
    {
        if (node->request.id == id)
        {
            return &node->request;
        }
        node = node->next;
    }
    return NULL;
}

bool RequestHashTable::erase(int id)
{
    if (id < 0)
    {
        return false;
    }

    const std::size_t index = bucketIndex(id);
    Node *node = buckets[index];
    Node *previous = NULL;
    while (node != NULL)
    {
        if (node->request.id == id)
        {
            if (previous == NULL)
            {
                buckets[index] = node->next;
            }
            else
            {
                previous->next = node->next;
            }
            delete node;
            --entryCount;
            return true;
        }
        previous = node;
        node = node->next;
    }
    return false;
}

bool RequestHashTable::isEmpty() const
{
    return entryCount == 0;
}

std::size_t RequestHashTable::size() const
{
    return entryCount;
}

std::vector<ReliefRequest> RequestHashTable::getAll() const
{
    std::vector<ReliefRequest> requests;
    requests.reserve(entryCount);
    for (std::size_t i = 0; i < buckets.size(); ++i)
    {
        const Node *node = buckets[i];
        while (node != NULL)
        {
            requests.push_back(node->request);
            node = node->next;
        }
    }
    return requests;
}
