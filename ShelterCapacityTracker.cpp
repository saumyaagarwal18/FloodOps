#include "ShelterCapacityTracker.h"

bool ShelterCapacityTracker::addShelter(const std::string &name, int capacity)
{
    return addShelter(name, name, capacity);
}

bool ShelterCapacityTracker::addShelter(
    const std::string &name,
    const std::string &region,
    int capacity)
{
    if (name.empty() || region.empty() || capacity <= 0 ||
        findShelter(name) != NULL)
    {
        return false;
    }

    TrackedShelter shelter;
    shelter.details.name = name;
    shelter.details.region = region;
    shelter.details.capacity = capacity;
    shelter.details.occupied = 0;
    shelters.push_back(shelter);
    return true;
}

bool ShelterCapacityTracker::allocate(
    int requestId,
    const std::string &preferredShelter,
    int evacuees,
    std::vector<ShelterAssignment> &assignments)
{
    std::vector<std::string> allRegions;
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        allRegions.push_back(shelters[i].details.region);
    }
    return allocate(requestId, preferredShelter, evacuees, allRegions,
                    assignments);
}

bool ShelterCapacityTracker::allocate(
    int requestId,
    const std::string &preferredRegion,
    int evacuees,
    const std::vector<std::string> &reachableRegions,
    std::vector<ShelterAssignment> &assignments)
{
    assignments.clear();
    if (requestId < 0 || evacuees <= 0)
    {
        return false;
    }

    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        for (std::size_t j = 0; j < shelters[i].allocations.size(); ++j)
        {
            if (shelters[i].allocations[j].requestId == requestId)
            {
                return false;
            }
        }
    }

    int remaining = evacuees;
    for (std::size_t pass = 0; pass < 2 && remaining > 0; ++pass)
    {
        for (std::size_t i = 0; i < shelters.size() && remaining > 0; ++i)
        {
            bool reachable = false;
            for (std::size_t j = 0; j < reachableRegions.size(); ++j)
            {
                if (reachableRegions[j] == shelters[i].details.region)
                {
                    reachable = true;
                    break;
                }
            }
            if (!reachable)
            {
                continue;
            }

            const bool preferred =
                shelters[i].details.region == preferredRegion;
            if ((pass == 0 && !preferred) || (pass == 1 && preferred))
            {
                continue;
            }

            const int available =
                shelters[i].details.capacity - shelters[i].details.occupied;
            if (available <= 0)
            {
                continue;
            }

            const int assigned = available < remaining ? available : remaining;
            ShelterAssignment assignment;
            assignment.shelterName = shelters[i].details.name;
            assignment.evacuees = assigned;
            assignments.push_back(assignment);
            remaining -= assigned;
        }
    }

    if (remaining > 0)
    {
        assignments.clear();
        return false;
    }

    for (std::size_t i = 0; i < assignments.size(); ++i)
    {
        for (std::size_t j = 0; j < shelters.size(); ++j)
        {
            if (shelters[j].details.name == assignments[i].shelterName)
            {
                shelters[j].details.occupied += assignments[i].evacuees;
                Allocation allocation;
                allocation.requestId = requestId;
                allocation.evacuees = assignments[i].evacuees;
                shelters[j].allocations.push_back(allocation);
                break;
            }
        }
    }
    return true;
}

bool ShelterCapacityTracker::release(int requestId, int evacuees)
{
    if (requestId < 0 || evacuees <= 0)
    {
        return false;
    }

    int allocated = 0;
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        for (std::size_t j = 0; j < shelters[i].allocations.size(); ++j)
        {
            if (shelters[i].allocations[j].requestId == requestId)
            {
                allocated += shelters[i].allocations[j].evacuees;
            }
        }
    }
    if (allocated < evacuees)
    {
        return false;
    }

    int remaining = evacuees;
    for (std::size_t i = 0; i < shelters.size() && remaining > 0; ++i)
    {
        std::vector<Allocation> &allocations = shelters[i].allocations;
        for (std::size_t j = 0; j < allocations.size() && remaining > 0;)
        {
            if (allocations[j].requestId != requestId)
            {
                ++j;
                continue;
            }

            const int released = allocations[j].evacuees < remaining
                ? allocations[j].evacuees : remaining;
            allocations[j].evacuees -= released;
            shelters[i].details.occupied -= released;
            remaining -= released;
            if (allocations[j].evacuees == 0)
            {
                allocations.erase(allocations.begin() + j);
            }
            else
            {
                ++j;
            }
        }
    }
    return true;
}

bool ShelterCapacityTracker::release(int requestId)
{
    int allocated = 0;
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        for (std::size_t j = 0; j < shelters[i].allocations.size(); ++j)
        {
            if (shelters[i].allocations[j].requestId == requestId)
            {
                allocated += shelters[i].allocations[j].evacuees;
            }
        }
    }
    return allocated > 0 && release(requestId, allocated);
}

const ShelterCapacity *ShelterCapacityTracker::findShelter(
    const std::string &name) const
{
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        if (shelters[i].details.name == name)
        {
            return &shelters[i].details;
        }
    }
    return NULL;
}

std::vector<ShelterCapacity> ShelterCapacityTracker::getShelters() const
{
    std::vector<ShelterCapacity> result;
    result.reserve(shelters.size());
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        result.push_back(shelters[i].details);
    }
    return result;
}

int ShelterCapacityTracker::totalAvailable() const
{
    int available = 0;
    for (std::size_t i = 0; i < shelters.size(); ++i)
    {
        available += shelters[i].details.capacity - shelters[i].details.occupied;
    }
    return available;
}
