#ifndef SHELTERCAPACITYTRACKER_H
#define SHELTERCAPACITYTRACKER_H

#include <string>
#include <vector>

struct ShelterAssignment
{
    std::string shelterName;
    int evacuees;
};

struct ShelterCapacity
{
    std::string name;
    std::string region;
    int capacity;
    int occupied;
};

class ShelterCapacityTracker
{
private:
    struct Allocation
    {
        int requestId;
        int evacuees;
    };

    struct TrackedShelter
    {
        ShelterCapacity details;
        std::vector<Allocation> allocations;
    };

    std::vector<TrackedShelter> shelters;

public:
    bool addShelter(const std::string &name, int capacity);
    bool addShelter(const std::string &name, const std::string &region,
                    int capacity);
    bool allocate(int requestId, const std::string &preferredShelter,
                  int evacuees, std::vector<ShelterAssignment> &assignments);
    bool allocate(int requestId, const std::string &preferredRegion,
                  int evacuees, const std::vector<std::string> &reachableRegions,
                  std::vector<ShelterAssignment> &assignments);
    bool release(int requestId, int evacuees);
    bool release(int requestId);
    const ShelterCapacity *findShelter(const std::string &name) const;
    std::vector<ShelterCapacity> getShelters() const;
    int totalAvailable() const;
};

#endif
