#ifndef LOCATIONMANAGEMENT_H
#define LOCATIONMANAGEMENT_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;


// ==========================================
// LOCATION TREE NODE
// ==========================================

struct LocationNode
{
    string name;
    string type;

    LocationNode *child;
    LocationNode *sibling;

    LocationNode(string n, string t)
    {
        name = n;
        type = t;
        child = NULL;
        sibling = NULL;
    }
};


// ==========================================
// LOCATION MANAGEMENT
// ==========================================

void locationMenu();

void enterLocation();

void displayHierarchy();


// ==========================================
// ROAD / GRAPH FUNCTIONS
// ==========================================

void addRoad();

void blockRoad();

void openRoad();

void displayRoadNetwork();


// ==========================================
// BFS
// ==========================================

bool isReachable(int source, int destination);

std::vector<int> findRoute(int source, int destination);

void checkReachability();


// ==========================================
// SEARCH LOCATION
// ==========================================

int findLocation(string name);

#endif