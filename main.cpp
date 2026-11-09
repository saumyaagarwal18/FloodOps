
#ifdef FLOODOPS_ENABLE_SFML

#include <SFML/Graphics.hpp>

#include <iostream>
#include <limits>
#include <optional>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cctype>

#include "LocationManagement.h"
#include "LinkedList.h"
#include "PriorityQueue.h"
#include "Queue.h"
#include "RequestManagement.h"
#include "RequestHashTable.h"
#include "ShelterCapacityTracker.h"
#include "Stack.h"

// Access existing data structures from LocationManagement.cpp without editing it
extern LocationNode *districtRoot;
extern string locationNames[];
extern int locationCount;
extern int road[50][50];
void addChild(LocationNode *parent, LocationNode *newNode);

RequestHashTable activeRequests;
ShelterCapacityTracker shelterTracker;
Stack operationHistory;
int nextRequestId = 101;
int currentTab = 0; // 0: Network, 1: Hierarchy, 2: Requests
std::string bfsResultText = "Ready to coordinate relief.";
sf::Color bfsResultColor = sf::Color(78, 111, 137);
std::string locationNameInput;
std::string shelterNameInput;
std::string shelterCapacityInput;
std::string requestLookupInput;
enum class DialogMode {
    None, AddLocation, ManageRoad, AddRequest, CheckRoute, ShowRoute,
    AddShelter, ConnectRegions, LookupRequest, ReleaseShelter
};
DialogMode dialogMode = DialogMode::None;
int roadSourceSelection = 0;
int roadDestinationSelection = 1;
int roadStatusSelection = 0;
int requestLocationSelection = 0;
int requestPrioritySelection = 1;
int shelterRegionSelection = 0;
int shelterInputFocus = 0;
std::string requestPeopleInput;
int routeSourceSelection = 0;
int routeDestinationSelection = 1;
std::vector<int> currentRoute;
std::size_t routeScrollOffset = 0;

std::vector<std::string> getReachableShelterRegions(const std::string &region) {
    std::vector<std::string> reachableRegions;
    int sourceIndex = findLocation(region);
    if (sourceIndex < 0 && districtRoot != NULL) {
        for (LocationNode *village = districtRoot->child;
             village != NULL && sourceIndex < 0; village = village->sibling) {
            for (LocationNode *area = village->child;
                 area != NULL; area = area->sibling) {
                if (area->name == region) {
                    sourceIndex = findLocation(village->name);
                    break;
                }
            }
        }
    }
    if (sourceIndex < 0) {
        return reachableRegions;
    }

    const std::vector<ShelterCapacity> shelters = shelterTracker.getShelters();
    for (std::size_t i = 0; i < shelters.size(); ++i) {
        const int shelterRegionIndex = findLocation(shelters[i].region);
        if (shelterRegionIndex >= 0 &&
            (shelterRegionIndex == sourceIndex ||
             isReachable(sourceIndex, shelterRegionIndex))) {
            reachableRegions.push_back(shelters[i].region);
        }
    }
    return reachableRegions;
}

bool parseRequestId(const std::string &value, int &requestId) {
    if (value.empty()) {
        return false;
    }

    requestId = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char digit = value[i];
        if (digit < '0' || digit > '9') {
            return false;
        }
        const int numericDigit = digit - '0';
        if (requestId > (std::numeric_limits<int>::max() - numericDigit) / 10) {
            return false;
        }
        requestId = requestId * 10 + numericDigit;
    }
    return true;
}

void lookupPendingRequest() {
    int requestId = 0;
    if (!parseRequestId(requestLookupInput, requestId)) {
        bfsResultText = "Enter a valid request ID.";
        bfsResultColor = sf::Color(190, 68, 68);
        return;
    }

    const ReliefRequest *request = activeRequests.find(requestId);
    if (request != NULL) {
        bfsResultText = "ID #" + std::to_string(request->id) + " [PENDING]: " +
            request->village + ", " + std::to_string(request->people);
        bfsResultColor = sf::Color(39, 126, 177);
        return;
    }

    const std::vector<OperationRecord> operations = operationHistory.getAll();
    for (std::size_t i = 0; i < operations.size(); ++i) {
        if (operations[i].requestId == requestId) {
            std::string shelterNames;
            for (std::size_t j = 0; j < operations[i].assignments.size(); ++j) {
                if (!shelterNames.empty()) {
                    shelterNames += ", ";
                }
                shelterNames += operations[i].assignments[j].shelterName;
            }
            if (shelterNames.empty()) {
                shelterNames = "Unknown shelter";
            }

            bfsResultText = "ID #" + std::to_string(operations[i].requestId) +
                " [DISPATCHED]: " + operations[i].village + " -> " +
                shelterNames;
            bfsResultColor = sf::Color(39, 125, 91);
            return;
        }
    }

    bfsResultText = "Request #" + std::to_string(requestId) + " not found";
    bfsResultColor = sf::Color(190, 68, 68);
}

bool addReliefRequest(const std::string &village, int people, int priority, int &id) {
    ReliefRequest request{nextRequestId, village, people, priority};
    if (!activeRequests.insert(request)) {
        return false;
    }

    id = nextRequestId++;
    return true;
}

bool dispatchNextRequest(std::string &message) {
    const std::vector<ReliefRequest> requests = activeRequests.getAll();
    if (requests.empty()) {
        message = "No requests are waiting for dispatch.";
        return false;
    }

    const ReliefRequest *nextRequest = &requests[0];
    for (std::size_t i = 1; i < requests.size(); ++i) {
        if (requests[i].priority > nextRequest->priority ||
            (requests[i].priority == nextRequest->priority &&
             requests[i].id < nextRequest->id)) {
            nextRequest = &requests[i];
        }
    }

    std::vector<ShelterAssignment> assignments;
    const std::vector<std::string> reachableRegions =
        getReachableShelterRegions(nextRequest->village);
    if (!shelterTracker.allocate(nextRequest->id, nextRequest->village,
                                 nextRequest->people, reachableRegions,
                                 assignments)) {
        message = "No reachable shelter capacity; request remains queued.";
        return false;
    }

    OperationRecord operation;
    operation.requestId = nextRequest->id;
    operation.village = nextRequest->village;
    operation.people = nextRequest->people;
    operation.priority = nextRequest->priority;
    operation.assignments = assignments;
    operationHistory.push(operation);
    const int dispatchedId = nextRequest->id;
    activeRequests.erase(dispatchedId);

    message = "Request #" + std::to_string(dispatchedId) + " assigned: ";
    for (std::size_t i = 0; i < assignments.size(); ++i) {
        if (i > 0) {
            message += ", ";
        }
        message += assignments[i].shelterName + " (" +
                   std::to_string(assignments[i].evacuees) + ")";
    }
    return true;
}

bool addVillageLocation(const std::string &name) {
    if (name.empty() || locationCount >= 50 || districtRoot == NULL) {
        return false;
    }

    if (shelterTracker.findShelter(name) != NULL) {
        return false;
    }

    for (int i = 0; i < locationCount; ++i) {
        if (locationNames[i].size() == name.size() &&
            std::equal(locationNames[i].begin(), locationNames[i].end(), name.begin(),
                [](unsigned char left, unsigned char right) {
                    return std::tolower(left) == std::tolower(right);
                })) {
            return false;
        }
    }

    addChild(districtRoot, new LocationNode(name, "Village"));
    //shelterTracker.addShelter(name, 500);

    const int newLocationIndex = locationCount++;
    locationNames[newLocationIndex] = name;
    return true;
}

std::string trim(const std::string &value) {
    const auto first = std::find_if_not(value.begin(), value.end(),
        [](unsigned char character) { return std::isspace(character); });
    const auto last = std::find_if_not(value.rbegin(), value.rend(),
        [](unsigned char character) { return std::isspace(character); }).base();
    return first < last ? std::string(first, last) : std::string();
}

struct UiButton {
    float x;
    float y;
    float width;
    float height;
    std::string label;
    std::string detail;
    sf::Color accent;
};

bool contains(const UiButton &button, sf::Vector2f point) {
    return point.x >= button.x && point.x <= button.x + button.width &&
           point.y >= button.y && point.y <= button.y + button.height;
}

void drawMetricCard(sf::RenderWindow &window, const sf::Font &font, bool fontLoaded,
                    float x, const std::string &label, const std::string &value,
                    sf::Color accent) {
    sf::RectangleShape card({214.0f, 58.0f});
    card.setPosition({x, 136.0f});
    card.setFillColor(sf::Color(255, 255, 255));
    card.setOutlineThickness(1.0f);
    card.setOutlineColor(sf::Color(222, 230, 240));
    window.draw(card);

    sf::RectangleShape marker({4.0f, 38.0f});
    marker.setPosition({x + 11.0f, 148.0f});
    marker.setFillColor(accent);
    window.draw(marker);

    if (fontLoaded) {
        sf::Text valueText(font, value, 20);
        valueText.setStyle(sf::Text::Bold);
        valueText.setPosition({x + 23.0f, 141.0f});
        valueText.setFillColor(sf::Color(30, 57, 78));
        window.draw(valueText);

        sf::Text labelText(font, label, 10);
        labelText.setPosition({x + 24.0f, 166.0f});
        labelText.setFillColor(sf::Color(94, 119, 140));
        window.draw(labelText);
    }
}

void drawControlButton(sf::RenderWindow &window, const sf::Font &font, bool fontLoaded,
                       const UiButton &button, sf::Vector2f mousePosition, bool isPressed,
                       bool enabled) {
    const bool hovered = enabled && contains(button, mousePosition);
    sf::RectangleShape shape({button.width, button.height});
    shape.setPosition({button.x, button.y + (isPressed ? 1.0f : 0.0f)});
    shape.setFillColor(!enabled ? sf::Color(232, 237, 245)
                                : isPressed ? sf::Color(225, 235, 246)
                                : hovered ? sf::Color(246, 249, 253)
                                          : sf::Color(255, 255, 255));
    shape.setOutlineThickness(hovered ? 2.0f : 0.0f);
    shape.setOutlineColor(button.accent);
    window.draw(shape);

    sf::RectangleShape accent({4.0f, button.height});
    accent.setPosition({button.x, button.y});
    accent.setFillColor(enabled ? button.accent : sf::Color(185, 196, 207));
    window.draw(accent);

    if (fontLoaded) {
        sf::Text label(font, button.label, 12);
        label.setStyle(sf::Text::Bold);
        label.setPosition({button.x + 14.0f, button.y + 5.0f});
        label.setFillColor(enabled ? sf::Color(24, 48, 74) : sf::Color(145, 160, 173));
        window.draw(label);

        sf::Text detail(font, button.detail, 9);
        detail.setPosition({button.x + 14.0f, button.y + 26.0f});
        detail.setFillColor(enabled ? sf::Color(91, 112, 137) : sf::Color(155, 168, 179));
        window.draw(detail);
    }
}

void drawDialogField(sf::RenderWindow &window, const sf::Font &font,
                     const std::string &label, const std::string &value, float y,
                     sf::Color accent) {
    sf::Text fieldLabel(font, label, 10);
    fieldLabel.setStyle(sf::Text::Bold);
    fieldLabel.setPosition({315.0f, y - 17.0f});
    fieldLabel.setFillColor(sf::Color(83, 105, 130));
    window.draw(fieldLabel);

    sf::RectangleShape field({410.0f, 44.0f});
    field.setPosition({315.0f, y});
    field.setFillColor(sf::Color(247, 250, 253));
    field.setOutlineThickness(1.0f);
    field.setOutlineColor(accent);
    window.draw(field);

    sf::Text fieldValue(font, value, 13);
    fieldValue.setPosition({329.0f, y + 12.0f});
    fieldValue.setFillColor(sf::Color(34, 58, 82));
    window.draw(fieldValue);
}

void drawDialogButton(sf::RenderWindow &window, const sf::Font &font,
                      const std::string &label, float x, float y, sf::Color fill) {
    sf::RectangleShape button({100.0f, 42.0f});
    button.setPosition({x, y});
    button.setFillColor(fill);
    window.draw(button);

    sf::Text text(font, label, 12);
    text.setStyle(sf::Text::Bold);
    text.setPosition({x + 19.0f, y + 12.0f});
    text.setFillColor(sf::Color(255, 255, 255));
    window.draw(text);
}

// Helper: Thick Line Drawing for SFML
void drawThickLine(sf::RenderWindow &window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color) {
    sf::Vector2f diff = p2 - p1;
    float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (length == 0.0f) return;
    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setFillColor(color);
    line.setPosition(p1);
    float angle = std::atan2(diff.y, diff.x) * 180.0f / 3.14159265f;
    line.setRotation(sf::degrees(angle));
    line.setOrigin({0.0f, thickness / 2.0f});
    window.draw(line);
}

// Helper: Seed demo disaster scenario into existing structures
void seedDemoScenario() {
    if (locationCount > 0) return; // Prevent duplicate population

    // 1. Populate Tree Hierarchy
    districtRoot = new LocationNode("Dehradun", "District");
    LocationNode *v1 = new LocationNode("Rishikesh", "Village");
    LocationNode *v2 = new LocationNode("Doiwala", "Village");
    LocationNode *v3 = new LocationNode("Vikasnagar", "Village");
    LocationNode *v4 = new LocationNode("Sahaspur", "Village");

    addChild(districtRoot, v1);
    addChild(districtRoot, v2);
    addChild(districtRoot, v3);
    addChild(districtRoot, v4);

    addChild(v1, new LocationNode("Tapovan", "Area"));
    addChild(v2, new LocationNode("Baluwala", "Area"));
    addChild(v3, new LocationNode("Dakpathar", "Area"));

    // 2. Populate Graph Network (Roads)
    locationNames[0] = "Dehradun";
    locationNames[1] = "Rishikesh";
    locationNames[2] = "Doiwala";
    locationNames[3] = "Vikasnagar";
    locationNames[4] = "Sahaspur";
    locationCount = 5;

    // 1 = Open, 2 = Blocked
    road[0][1] = 1; road[1][0] = 1; // Dehradun <-> Rishikesh (Open)
    road[0][2] = 1; road[2][0] = 1; // Dehradun <-> Doiwala (Open)
    road[1][2] = 2; road[2][1] = 2; // Rishikesh <-> Doiwala (Blocked)
    road[0][3] = 1; road[3][0] = 1; // Dehradun <-> Vikasnagar (Open)
    road[3][4] = 2; road[4][3] = 2; // Vikasnagar <-> Sahaspur (Blocked)
    road[0][4] = 1; road[4][0] = 1; // Dehradun <-> Sahaspur (Open)

    shelterTracker.addShelter("Dehradun Shelter", "Dehradun", 500);

    // 3. Populate Relief Requests
    int requestId = 0;
    addReliefRequest("Rishikesh", 450, 1, requestId);
    addReliefRequest("Sahaspur", 120, 0, requestId);
    addReliefRequest("Dakpathar", 310, 1, requestId);
    addReliefRequest("Baluwala", 80, 0, requestId);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({1040u, 680u}), "FloodOps - Disaster Relief Operations Hub");
    window.setFramerateLimit(60);

    // Cross-platform font fallback search
    sf::Font font;
    bool fontLoaded = false;
    std::string fontPaths[] = {
        "arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };

    for (const auto &path : fontPaths) {
        if (font.openFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }

    if (!fontLoaded) {
        std::cerr << "Warning: Could not load default font. Text rendering may fail.\n";
    }

    // Auto-seed initial state so UI is immediately visible on launch
    seedDemoScenario();

    const UiButton actionButtons[] = {
        {765.0f, 154.0f, 230.0f, 40.0f, "Manage roads", "Connect locations; set open / blocked", sf::Color(42, 178, 155)},
        {765.0f, 200.0f, 230.0f, 40.0f, "Check route with BFS", "Rishikesh to Sahaspur", sf::Color(71, 150, 224)},
        {765.0f, 246.0f, 230.0f, 40.0f, "Create relief request", "Choose location, people and urgency", sf::Color(244, 174, 68)},
        {765.0f, 292.0f, 230.0f, 40.0f, "Dispatch next request", "Emergency requests are prioritized", sf::Color(226, 111, 102)},
        {765.0f, 338.0f, 230.0f, 40.0f, "Add location", "Register a new village", sf::Color(123, 113, 226)},
        {765.0f, 384.0f, 230.0f, 40.0f, "Add shelter", "Register beds in a region", sf::Color(42, 178, 155)}
    };
    const float tabX[] = {410.0f, 590.0f, 770.0f};
    const float tabWidth = 160.0f;
    const float tabY = 18.0f;
    const float tabHeight = 40.0f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                continue;
            }

            if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>();
                keyPressed && dialogMode != DialogMode::None) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    dialogMode = DialogMode::None;
                    bfsResultText = "Action cancelled.";
                    bfsResultColor = sf::Color(110, 126, 148);
                } else if (dialogMode == DialogMode::ShowRoute &&
                           (keyPressed->code == sf::Keyboard::Key::Down ||
                            keyPressed->code == sf::Keyboard::Key::PageDown)) {
                    const std::size_t step = keyPressed->code == sf::Keyboard::Key::PageDown ? 7 : 1;
                    const std::size_t lastOffset = currentRoute.size() > 8
                        ? currentRoute.size() - 8 : 0;
                    routeScrollOffset = std::min(routeScrollOffset + step, lastOffset);
                } else if (dialogMode == DialogMode::ShowRoute &&
                           (keyPressed->code == sf::Keyboard::Key::Up ||
                            keyPressed->code == sf::Keyboard::Key::PageUp)) {
                    const std::size_t step = keyPressed->code == sf::Keyboard::Key::PageUp ? 7 : 1;
                    routeScrollOffset = routeScrollOffset > step ? routeScrollOffset - step : 0;
                } else if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                    std::string *input = NULL;
                    if (dialogMode == DialogMode::AddLocation) {
                        input = &locationNameInput;
                    } else if (dialogMode == DialogMode::AddRequest) {
                        input = &requestPeopleInput;
                    } else if (dialogMode == DialogMode::AddShelter) {
                        input = shelterInputFocus == 0
                            ? &shelterNameInput : &shelterCapacityInput;
                    } else if (dialogMode == DialogMode::LookupRequest ||
                               dialogMode == DialogMode::ReleaseShelter) {
                        input = &requestLookupInput;
                    }
                    if (input != NULL && !input->empty()) {
                        input->pop_back();
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::AddShelter) {
                    const std::string name = trim(shelterNameInput);
                    if (name.empty() || shelterCapacityInput.empty() ||
                        locationCount == 0 || std::stoi(shelterCapacityInput) <= 0) {
                        bfsResultText = "Enter a shelter name and positive capacity.";
                        bfsResultColor = sf::Color(190, 68, 68);
                    } else if (shelterTracker.addShelter(
                                   name, locationNames[shelterRegionSelection],
                                   std::stoi(shelterCapacityInput))) {
                        bfsResultText = "Shelter " + name + " added in " +
                            locationNames[shelterRegionSelection] + ".";
                        bfsResultColor = sf::Color(34, 139, 98);
                        dialogMode = DialogMode::None;
                    } else {
                        bfsResultText = "Shelter name is already in use.";
                        bfsResultColor = sf::Color(190, 68, 68);
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::LookupRequest) {
                    lookupPendingRequest();
                    currentTab = 2;
                    dialogMode = DialogMode::None;
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::ReleaseShelter) {
                    int requestId = 0;
                    if (parseRequestId(requestLookupInput, requestId) &&
                        shelterTracker.release(requestId)) {
                        bfsResultText = "Released shelter allocation for request #" +
                            std::to_string(requestId) + ".";
                        bfsResultColor = sf::Color(34, 139, 98);
                    } else {
                        bfsResultText =
                            "No shelter allocation found for that request ID.";
                        bfsResultColor = sf::Color(190, 68, 68);
                    }
                    currentTab = 3;
                    dialogMode = DialogMode::None;
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::ConnectRegions) {
                    if (locationCount > 1 &&
                        roadSourceSelection != roadDestinationSelection) {
                        road[roadSourceSelection][roadDestinationSelection] = 1;
                        road[roadDestinationSelection][roadSourceSelection] = 1;
                        bfsResultText = "Shelters connected: " +
                            locationNames[roadSourceSelection] + " <-> " +
                            locationNames[roadDestinationSelection] + ".";
                        bfsResultColor = sf::Color(34, 139, 98);
                        dialogMode = DialogMode::None;
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::AddLocation) {
                    const std::string name = trim(locationNameInput);
                    if (addVillageLocation(name)) {
                        bfsResultText = "Village added. Set its road connections.";
                        bfsResultColor = sf::Color(34, 139, 98);
                        dialogMode = DialogMode::None;
                    } else {
                        bfsResultText = name.empty() ? "Enter a village name." :
                            "Name is already used or the 50-location limit was reached.";
                        bfsResultColor = sf::Color(190, 68, 68);
                    }
                } else if (keyPressed->code == sf::Keyboard::Key::Enter &&
                           dialogMode == DialogMode::AddRequest) {
                    if (requestPeopleInput.empty() || std::stoi(requestPeopleInput) <= 0 ||
                        locationCount == 0) {
                        bfsResultText = "Enter a positive affected-person count.";
                        bfsResultColor = sf::Color(190, 68, 68);
                    } else {
                        int newId = 0;
                        if (addReliefRequest(locationNames[requestLocationSelection],
                                             std::stoi(requestPeopleInput),
                                             requestPrioritySelection, newId)) {
                            bfsResultText = "Request #" + std::to_string(newId) + " created.";
                            bfsResultColor = sf::Color(39, 126, 177);
                            currentTab = 2;
                            dialogMode = DialogMode::None;
                        } else {
                            bfsResultText = "Could not create the relief request.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        }
                    }
                }
                continue;
            }

            if (const auto *textEntered = event->getIf<sf::Event::TextEntered>();
                textEntered && dialogMode != DialogMode::None && textEntered->unicode >= 32 &&
                textEntered->unicode <= 126) {
                if (dialogMode == DialogMode::AddLocation && locationNameInput.size() < 32) {
                    locationNameInput += static_cast<char>(textEntered->unicode);
                } else if (dialogMode == DialogMode::AddRequest &&
                           textEntered->unicode >= '0' && textEntered->unicode <= '9' &&
                           requestPeopleInput.size() < 7) {
                    requestPeopleInput += static_cast<char>(textEntered->unicode);
                } else if (dialogMode == DialogMode::AddShelter &&
                           shelterInputFocus == 1 &&
                           textEntered->unicode >= '0' && textEntered->unicode <= '9' &&
                           shelterCapacityInput.size() < 7) {
                    shelterCapacityInput += static_cast<char>(textEntered->unicode);
                } else if (dialogMode == DialogMode::AddShelter &&
                           shelterInputFocus == 0 && shelterNameInput.size() < 32) {
                    shelterNameInput += static_cast<char>(textEntered->unicode);
                } else if ((dialogMode == DialogMode::LookupRequest ||
                            dialogMode == DialogMode::ReleaseShelter) &&
                           textEntered->unicode >= '0' && textEntered->unicode <= '9' &&
                           requestLookupInput.size() < 10) {
                    requestLookupInput += static_cast<char>(textEntered->unicode);
                }
                continue;
            }

            if (const auto *mouseButton = event->getIf<sf::Event::MouseButtonPressed>();
                mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
                sf::Vector2f mouse(static_cast<float>(mouseButton->position.x),
                                   static_cast<float>(mouseButton->position.y));

                if (dialogMode == DialogMode::AddShelter) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f) {
                        shelterInputFocus = 0;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 338.0f && mouse.y <= 382.0f &&
                               locationCount > 0) {
                        shelterRegionSelection =
                            (shelterRegionSelection + 1) % locationCount;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 396.0f && mouse.y <= 440.0f) {
                        shelterInputFocus = 1;
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        const std::string name = trim(shelterNameInput);
                        if (name.empty() || shelterCapacityInput.empty() ||
                            locationCount == 0 ||
                            std::stoi(shelterCapacityInput) <= 0) {
                            bfsResultText =
                                "Enter a shelter name and positive capacity.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        } else if (shelterTracker.addShelter(
                                       name, locationNames[shelterRegionSelection],
                                       std::stoi(shelterCapacityInput))) {
                            bfsResultText = "Shelter " + name + " added in " +
                                locationNames[shelterRegionSelection] + ".";
                            bfsResultColor = sf::Color(34, 139, 98);
                            dialogMode = DialogMode::None;
                        } else {
                            bfsResultText = "Shelter name is already in use.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::ConnectRegions) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f &&
                        locationCount > 1) {
                        roadSourceSelection =
                            (roadSourceSelection + 1) % locationCount;
                        if (roadSourceSelection == roadDestinationSelection) {
                            roadSourceSelection =
                                (roadSourceSelection + 1) % locationCount;
                        }
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 338.0f && mouse.y <= 382.0f &&
                               locationCount > 1) {
                        roadDestinationSelection =
                            (roadDestinationSelection + 1) % locationCount;
                        if (roadDestinationSelection == roadSourceSelection) {
                            roadDestinationSelection =
                                (roadDestinationSelection + 1) % locationCount;
                        }
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        if (locationCount > 1 &&
                            roadSourceSelection != roadDestinationSelection) {
                            road[roadSourceSelection][roadDestinationSelection] = 1;
                            road[roadDestinationSelection][roadSourceSelection] = 1;
                            bfsResultText = "Shelters connected: " +
                                locationNames[roadSourceSelection] + " <-> " +
                                locationNames[roadDestinationSelection] + ".";
                            bfsResultColor = sf::Color(34, 139, 98);
                            dialogMode = DialogMode::None;
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::LookupRequest) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f) {
                        requestLookupInput.clear();
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        lookupPendingRequest();
                        currentTab = 2;
                        dialogMode = DialogMode::None;
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::ReleaseShelter) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f) {
                        requestLookupInput.clear();
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        int requestId = 0;
                        if (parseRequestId(requestLookupInput, requestId) &&
                            shelterTracker.release(requestId)) {
                            bfsResultText =
                                "Released shelter allocation for request #" +
                                std::to_string(requestId) + ".";
                            bfsResultColor = sf::Color(34, 139, 98);
                        } else {
                            bfsResultText =
                                "No shelter allocation found for that request ID.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        }
                        currentTab = 3;
                        dialogMode = DialogMode::None;
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::AddLocation) {
                    if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                        mouse.y >= 400.0f && mouse.y <= 442.0f) {
                        const std::string name = trim(locationNameInput);
                        if (addVillageLocation(name)) {
                            bfsResultText = "Village added. Set its road connections.";
                            bfsResultColor = sf::Color(34, 139, 98);
                            dialogMode = DialogMode::None;
                        } else {
                            bfsResultText = name.empty() ? "Enter a village name." :
                                "Name is already used or the 50-location limit was reached.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 400.0f && mouse.y <= 442.0f) {
                        dialogMode = DialogMode::None;
                        bfsResultText = "Adding location cancelled.";
                    }
                    continue;
                }

                if (dialogMode == DialogMode::ManageRoad) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f && locationCount > 0) {
                        roadSourceSelection = (roadSourceSelection + 1) % locationCount;
                        if (roadSourceSelection == roadDestinationSelection && locationCount > 1)
                            roadSourceSelection = (roadSourceSelection + 1) % locationCount;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 338.0f && mouse.y <= 382.0f && locationCount > 1) {
                        roadDestinationSelection = (roadDestinationSelection + 1) % locationCount;
                        if (roadDestinationSelection == roadSourceSelection)
                            roadDestinationSelection = (roadDestinationSelection + 1) % locationCount;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 396.0f && mouse.y <= 440.0f) {
                        roadStatusSelection = (roadStatusSelection + 1) % 3;
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        if (locationCount > 1 && roadSourceSelection != roadDestinationSelection) {
                            road[roadSourceSelection][roadDestinationSelection] = roadStatusSelection;
                            road[roadDestinationSelection][roadSourceSelection] = roadStatusSelection;
                            const std::string status = roadStatusSelection == 0 ? "removed" :
                                roadStatusSelection == 1 ? "opened" : "blocked";
                            bfsResultText = "Road " + status + ": " +
                                locationNames[roadSourceSelection] + " <-> " +
                                locationNames[roadDestinationSelection];
                            bfsResultColor = roadStatusSelection == 2
                                ? sf::Color(190, 68, 68) : sf::Color(34, 139, 98);
                            dialogMode = DialogMode::None;
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::CheckRoute) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f && locationCount > 0) {
                        routeSourceSelection = (routeSourceSelection + 1) % locationCount;
                        if (routeSourceSelection == routeDestinationSelection && locationCount > 1)
                            routeSourceSelection = (routeSourceSelection + 1) % locationCount;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 338.0f && mouse.y <= 382.0f && locationCount > 1) {
                        routeDestinationSelection = (routeDestinationSelection + 1) % locationCount;
                        if (routeDestinationSelection == routeSourceSelection)
                            routeDestinationSelection = (routeDestinationSelection + 1) % locationCount;
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        currentRoute = findRoute(routeSourceSelection, routeDestinationSelection);
                        routeScrollOffset = 0;
                        if (currentRoute.empty()) {
                            bfsResultText = "No open-road route found.";
                            bfsResultColor = sf::Color(190, 68, 68);
                            dialogMode = DialogMode::None;
                        } else {
                            bfsResultText = "BFS found a route with " +
                                std::to_string(currentRoute.size() - 1) + " road(s).";
                            bfsResultColor = sf::Color(34, 139, 98);
                            dialogMode = DialogMode::ShowRoute;
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (dialogMode == DialogMode::ShowRoute) {
                    if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                        mouse.y >= 576.0f && mouse.y <= 618.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                if (currentTab == 2 && dialogMode == DialogMode::None &&
                    mouse.x >= 520.0f && mouse.x <= 710.0f &&
                    mouse.y >= 212.0f && mouse.y <= 258.0f) {
                    requestLookupInput.clear();
                    dialogMode = DialogMode::LookupRequest;
                    continue;
                }

                if (dialogMode == DialogMode::AddRequest) {
                    if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                        mouse.y >= 280.0f && mouse.y <= 324.0f && locationCount > 0) {
                        requestLocationSelection = (requestLocationSelection + 1) % locationCount;
                    } else if (mouse.x >= 315.0f && mouse.x <= 725.0f &&
                               mouse.y >= 396.0f && mouse.y <= 440.0f) {
                        requestPrioritySelection = mouse.x < 520.0f ? 1 : 0;
                    } else if (mouse.x >= 545.0f && mouse.x <= 645.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        if (requestPeopleInput.empty() || std::stoi(requestPeopleInput) <= 0 ||
                            locationCount == 0) {
                            bfsResultText = "Enter a positive affected-person count.";
                            bfsResultColor = sf::Color(190, 68, 68);
                        } else {
                            int newId = 0;
                            if (addReliefRequest(locationNames[requestLocationSelection],
                                                 std::stoi(requestPeopleInput),
                                                 requestPrioritySelection, newId)) {
                                bfsResultText = "Request #" + std::to_string(newId) + " created.";
                                bfsResultColor = sf::Color(39, 126, 177);
                                currentTab = 2;
                                dialogMode = DialogMode::None;
                            } else {
                                bfsResultText = "Could not create the relief request.";
                                bfsResultColor = sf::Color(190, 68, 68);
                            }
                        }
                    } else if (mouse.x >= 660.0f && mouse.x <= 760.0f &&
                               mouse.y >= 456.0f && mouse.y <= 498.0f) {
                        dialogMode = DialogMode::None;
                    }
                    continue;
                }

                for (int i = 0; i < 3; ++i) {
                    if (mouse.x >= tabX[i] && mouse.x <= tabX[i] + tabWidth &&
                        mouse.y >= tabY && mouse.y <= tabY + tabHeight) {
                        currentTab = i;
                        break;
                    }
                }

                if (contains(actionButtons[4], mouse) && locationCount < 50) {
                    locationNameInput.clear();
                    dialogMode = DialogMode::AddLocation;
                } else if (contains(actionButtons[4], mouse) && locationCount >= 50) {
                    bfsResultText = "Location limit reached (50).";
                    bfsResultColor = sf::Color(190, 68, 68);
                }

                if (contains(actionButtons[0], mouse) && locationCount > 1) {
                    roadSourceSelection = 0;
                    roadDestinationSelection = 1;
                    roadStatusSelection = road[0][1];
                    dialogMode = DialogMode::ManageRoad;
                }

                if (contains(actionButtons[1], mouse) && locationCount > 1) {
                    routeSourceSelection = 0;
                    routeDestinationSelection = 1;
                    dialogMode = DialogMode::CheckRoute;
                }

                if (contains(actionButtons[2], mouse) && locationCount > 0) {
                    requestLocationSelection = 0;
                    requestPeopleInput.clear();
                    requestPrioritySelection = 1;
                    dialogMode = DialogMode::AddRequest;
                }

                if (contains(actionButtons[3], mouse) && !activeRequests.isEmpty()) {
                    if (dispatchNextRequest(bfsResultText)) {
                        bfsResultColor = sf::Color(169, 119, 30);
                    } else {
                        bfsResultColor = sf::Color(190, 68, 68);
                    }
                }

                if (contains(actionButtons[5], mouse) && locationCount > 0) {
                    shelterNameInput.clear();
                    shelterCapacityInput.clear();
                    shelterRegionSelection = 0;
                    shelterInputFocus = 0;
                    dialogMode = DialogMode::AddShelter;
                }
            }
        }

        // ================= RENDERING =================
        window.clear(sf::Color(237, 243, 249));

        // 1. Header and navigation
        sf::RectangleShape topBar({1040.0f, 76.0f});
        topBar.setFillColor(sf::Color(18, 39, 62));
        window.draw(topBar);

        sf::RectangleShape topAccent({1040.0f, 3.0f});
        topAccent.setFillColor(sf::Color(52, 190, 190));
        window.draw(topAccent);

        const std::string tabs[] = {
            "Road network", "Location tree", "Relief queue"
        };
        for (int i = 0; i < 3; ++i) {
            const sf::Vector2f mousePosition = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
            const bool hovered = mousePosition.x >= tabX[i] && mousePosition.x <= tabX[i] + tabWidth &&
                                 mousePosition.y >= tabY && mousePosition.y <= tabY + tabHeight;
            sf::RectangleShape tabBtn({tabWidth, tabHeight});
            tabBtn.setPosition({tabX[i], tabY});
            tabBtn.setFillColor(currentTab == i ? sf::Color(43, 84, 113)
                                                : hovered ? sf::Color(31, 62, 89)
                                                          : sf::Color(18, 39, 62));
            tabBtn.setOutlineThickness(currentTab == i || hovered ? 1.5f : 1.0f);
            tabBtn.setOutlineColor(currentTab == i ? sf::Color(63, 198, 191)
                                                   : sf::Color(44, 67, 89));
            window.draw(tabBtn);

            if (fontLoaded) {
                sf::Text tabText(font, tabs[i], 13);
                const float textWidth = tabText.getLocalBounds().size.x;
                tabText.setPosition({tabX[i] + (tabWidth - textWidth) / 2.0f, tabY + 11.0f});
                tabText.setFillColor(currentTab == i ? sf::Color(255, 255, 255)
                                                     : sf::Color(191, 210, 226));
                window.draw(tabText);
            }
        }

        if (fontLoaded) {
            sf::Text brand(font, "FLOODOPS", 20);
            brand.setStyle(sf::Text::Bold);
            brand.setPosition({25.0f, 11.0f});
            brand.setFillColor(sf::Color(255, 255, 255));
            window.draw(brand);

            sf::Text tagline(font, "DISTRICT RELIEF OPERATIONS", 9);
            tagline.setPosition({27.0f, 42.0f});
            tagline.setFillColor(sf::Color(157, 184, 204));
            window.draw(tagline);
        }

        // 2. Dashboard summary
        int openRoads = 0;
        int blockedRoads = 0;
        int emergencyRequests = 0;
        for (int i = 0; i < locationCount; ++i) {
            for (int j = i + 1; j < locationCount; ++j) {
                if (road[i][j] == 1) ++openRoads;
                else if (road[i][j] == 2) ++blockedRoads;
            }
        }
        const std::vector<ReliefRequest> currentRequests = activeRequests.getAll();
        for (const auto &request : currentRequests)
            if (request.priority == 1) ++emergencyRequests;

        if (fontLoaded) {
            const std::string titles[] = {
                "Road network", "Location hierarchy", "Relief requests"
            };
            const std::string descriptions[] = {
                "See how road closures change access between communities.",
                "Explore the district, village and area structure.",
                "Track urgent needs and dispatch the highest-priority request."
            };
            sf::Text sectionTitle(font, titles[currentTab], 18);
            sectionTitle.setStyle(sf::Text::Bold);
            sectionTitle.setPosition({30.0f, 91.0f});
            sectionTitle.setFillColor(sf::Color(31, 57, 77));
            window.draw(sectionTitle);

            sf::Text sectionDescription(font, descriptions[currentTab], 10);
            sectionDescription.setPosition({31.0f, 116.0f});
            sectionDescription.setFillColor(sf::Color(91, 117, 139));
            window.draw(sectionDescription);
        }

        drawMetricCard(window, font, fontLoaded, 30.0f, "LOCATIONS", std::to_string(locationCount),
                       sf::Color(52, 153, 199));
        drawMetricCard(window, font, fontLoaded, 258.0f, "OPEN / BLOCKED ROADS",
                       std::to_string(openRoads) + " / " + std::to_string(blockedRoads),
                       sf::Color(50, 157, 116));
        drawMetricCard(window, font, fontLoaded, 486.0f, "URGENT REQUESTS",
                       std::to_string(emergencyRequests), sf::Color(201, 91, 82));

        // 3. Interactive demonstration panel
        sf::RectangleShape panel({270.0f, 570.0f});
        panel.setPosition({750.0f, 90.0f});
        panel.setFillColor(sf::Color(22, 45, 69));
        panel.setOutlineThickness(1);
        panel.setOutlineColor(sf::Color(38, 67, 94));
        window.draw(panel);

        if (fontLoaded) {
            sf::Text panelHeader(font, "LIVE DEMO", 15);
            panelHeader.setStyle(sf::Text::Bold);
            panelHeader.setPosition({770.0f, 104.0f});
            panelHeader.setFillColor(sf::Color(245, 250, 255));
            window.draw(panelHeader);
            sf::Text panelDescription(font, "Click an action to change the live scenario.", 9);
            panelDescription.setPosition({770.0f, 127.0f});
            panelDescription.setFillColor(sf::Color(164, 186, 205));
            window.draw(panelDescription);
        }

        const sf::Vector2f mousePosition = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
        const bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        for (int i = 0; i < 6; ++i) {
            const bool enabled = (i != 3 || !activeRequests.isEmpty()) &&
                                 (i != 4 || locationCount < 50) &&
                                 (i != 0 || locationCount > 1) &&
                                 (i != 2 || locationCount > 0) &&
                                 (i != 5 || locationCount > 0);
            drawControlButton(window, font, fontLoaded, actionButtons[i], mousePosition,
                              mouseDown && contains(actionButtons[i], mousePosition), enabled);
        }

        sf::RectangleShape statusBox({230.0f, 76.0f});
        statusBox.setPosition({765.0f, 478.0f});
        statusBox.setFillColor(sf::Color(30, 58, 84));
        statusBox.setOutlineThickness(1);
        statusBox.setOutlineColor(sf::Color(58, 88, 114));
        window.draw(statusBox);

        if (fontLoaded) {
            sf::Text statusTitle(font, "LATEST UPDATE", 9);
            statusTitle.setPosition({777.0f, 486.0f});
            statusTitle.setFillColor(sf::Color(157, 187, 208));
            window.draw(statusTitle);

            sf::Text statusContent(font, bfsResultText, 10);
            statusContent.setPosition({777.0f, 507.0f});
            statusContent.setFillColor(bfsResultColor);
            window.draw(statusContent);
        }

        if (fontLoaded) {
            sf::Text conceptTitle(font, "WHAT THIS DEMO SHOWS", 9);
            conceptTitle.setStyle(sf::Text::Bold);
            conceptTitle.setPosition({770.0f, 560.0f});
            conceptTitle.setFillColor(sf::Color(181, 204, 220));
            window.draw(conceptTitle);

            const std::string conceptLines[] = {
                "Graph  -  connected roads",
                "BFS  -  route reachability",
                "Priority  -  urgent needs first"
            };
            for (int i = 0; i < 3; ++i) {
                sf::Text conceptText(font, conceptLines[i], 10);
                conceptText.setPosition({770.0f, 580.0f + static_cast<float>(i) * 18.0f});
                conceptText.setFillColor(sf::Color(147, 174, 195));
                window.draw(conceptText);
            }
            sf::Text capacityText(font,
                "Shelter beds available: " + std::to_string(shelterTracker.totalAvailable()),
                9);
            capacityText.setPosition({770.0f, 637.0f});
            capacityText.setFillColor(sf::Color(147, 205, 190));
            window.draw(capacityText);
        }

        sf::RectangleShape workspaceCard({710.0f, 450.0f});
        workspaceCard.setPosition({20.0f, 202.0f});
        workspaceCard.setFillColor(sf::Color(255, 255, 255));
        workspaceCard.setOutlineThickness(1.0f);
        workspaceCard.setOutlineColor(sf::Color(222, 230, 240));
        window.draw(workspaceCard);

        // 4. Tab contents
        if (currentTab == 0) {
            // ================= ROAD GRAPH VIEW =================
            const sf::Vector2f center(370, 425);
            std::vector<sf::Vector2f> nodePositions(locationCount);

            for (int i = 0; i < locationCount; ++i) {
                int ringSize = locationCount;
                int ringIndex = i;
                float radius = locationCount > 24 ? 160.0f : locationCount > 12 ? 155.0f : 148.0f;
                if (locationCount > 12) {
                    const int rings = locationCount > 24 ? 3 : 2;
                    const int baseSize = locationCount / rings;
                    const int remainder = locationCount % rings;
                    int firstIndex = 0;
                    int ring = 0;
                    for (; ring < rings; ++ring) {
                        ringSize = baseSize + (ring < remainder ? 1 : 0);
                        if (i < firstIndex + ringSize) {
                            ringIndex = i - firstIndex;
                            break;
                        }
                        firstIndex += ringSize;
                    }
                    radius = rings == 3
                        ? (ring == 0 ? 160.0f : ring == 1 ? 108.0f : 58.0f)
                        : (ring == 0 ? 155.0f : 78.0f);
                }
                const float angle = static_cast<float>(ringIndex) *
                                    (2.0f * 3.14159265f / static_cast<float>(ringSize)) -
                                    (3.14159265f / 2.0f);
                nodePositions[i] = sf::Vector2f(center.x + radius * std::cos(angle),
                                                center.y + radius * std::sin(angle));
            }

            // Road lines sit behind the location markers.
            for (int i = 0; i < locationCount; ++i) {
                for (int j = i + 1; j < locationCount; ++j) {
                    if (road[i][j] != 0) {
                        sf::Color edgeCol = (road[i][j] == 1) ? sf::Color(48, 158, 116) : sf::Color(196, 76, 76);
                        drawThickLine(window, nodePositions[i], nodePositions[j],
                                      locationCount > 20 ? 2.0f : 4.0f, edgeCol);
                    }
                }
            }

            const std::vector<ShelterCapacity> graphShelters =
                shelterTracker.getShelters();
            for (int i = 0; i < locationCount; ++i) {
                const float nodeRadius = locationCount > 20 ? 10.0f :
                                         locationCount > 12 ? 16.0f : 23.0f;
                int shelterCount = 0;
                int totalCapacity = 0;
                for (std::size_t shelterIndex = 0;
                     shelterIndex < graphShelters.size(); ++shelterIndex) {
                    if (graphShelters[shelterIndex].region == locationNames[i]) {
                        ++shelterCount;
                        totalCapacity += graphShelters[shelterIndex].capacity;
                    }
                }

                sf::CircleShape circle(nodeRadius);
                circle.setOrigin({nodeRadius, nodeRadius});
                circle.setPosition(nodePositions[i]);
                const sf::Color nodeColors[] = {
                    sf::Color(69, 112, 214), sf::Color(40, 174, 156),
                    sf::Color(130, 103, 215), sf::Color(232, 156, 67)
                };
                circle.setFillColor(nodeColors[i % 4]);
                circle.setOutlineThickness(shelterCount > 0 ? 4.0f : 2.0f);
                circle.setOutlineColor(shelterCount > 0
                    ? sf::Color(48, 158, 116)
                    : sf::Color(255, 255, 255));
                window.draw(circle);

                if (fontLoaded) {
                    const unsigned int labelSize = locationCount > 20 ? 8 :
                                                   locationCount > 12 ? 10 : 12;
                    sf::Text lbl(font, locationNames[i], labelSize);
                    lbl.setStyle(sf::Text::Bold);
                    lbl.setFillColor(sf::Color(46, 66, 91));
                    lbl.setOrigin({lbl.getLocalBounds().size.x / 2.0f, 0.0f});
                    lbl.setPosition({nodePositions[i].x, nodePositions[i].y + nodeRadius + 3.0f});
                    window.draw(lbl);

                    const std::string shelterLabel = shelterCount > 0
                        ? std::to_string(shelterCount) +
                            (shelterCount == 1 ? " shelter (" : " shelters (") +
                            std::to_string(totalCapacity) + " beds)"
                        : "No shelter";
                    sf::Text shelterText(font, shelterLabel,
                                         labelSize > 10 ? 9 : labelSize > 8 ? 8 : 7);
                    shelterText.setOrigin(
                        {shelterText.getLocalBounds().size.x / 2.0f, 0.0f});
                    shelterText.setPosition({nodePositions[i].x,
                        nodePositions[i].y + nodeRadius + 3.0f + labelSize + 1.0f});
                    shelterText.setFillColor(shelterCount > 0
                        ? sf::Color(39, 125, 91)
                        : sf::Color(92, 118, 139));
                    window.draw(shelterText);

                    sf::Text idText(font, std::to_string(i), locationCount > 20 ? 8 : 11);
                    idText.setOrigin({idText.getLocalBounds().size.x / 2.0f,
                                      idText.getLocalBounds().size.y / 2.0f});
                    idText.setPosition(nodePositions[i]);
                    window.draw(idText);
                }
            }

            // Legend
            if (fontLoaded) {
                sf::CircleShape openDot(4.0f);
                openDot.setPosition({31.0f, 632.0f});
                openDot.setFillColor(sf::Color(48, 158, 116));
                window.draw(openDot);
                sf::Text leg1(font, "Open road", 10);
                leg1.setFillColor(sf::Color(39, 125, 91));
                leg1.setPosition({44.0f, 628.0f});
                window.draw(leg1);

                sf::CircleShape blockedDot(4.0f);
                blockedDot.setPosition({139.0f, 632.0f});
                blockedDot.setFillColor(sf::Color(196, 76, 76));
                window.draw(blockedDot);
                sf::Text leg2(font, "Blocked road", 10);
                leg2.setFillColor(sf::Color(165, 61, 61));
                leg2.setPosition({152.0f, 628.0f});
                window.draw(leg2);

                sf::Text hint(font, "Use the live demo controls to open or block Rishikesh-Doiwala.", 9);
                hint.setPosition({275.0f, 629.0f});
                hint.setFillColor(sf::Color(92, 118, 139));
                window.draw(hint);
            }

        } else if (currentTab == 1) {
            // ================= HIERARCHY TREE VIEW =================
            if (fontLoaded) {
                const float startY = 212.0f;
                if (districtRoot != NULL) {
                    // District Node Box
                    sf::RectangleShape distBox({220.0f, 36.0f});
                    distBox.setPosition({40.0f, startY});
                    distBox.setFillColor(sf::Color(231, 222, 246));
                    distBox.setOutlineThickness(1.0f);
                    distBox.setOutlineColor(sf::Color(181, 155, 216));
                    window.draw(distBox);

                    sf::Text distText(font, districtRoot->name + " [" + districtRoot->type + "]", 13);
                    distText.setPosition({50.0f, startY + 8.0f});
                    distText.setFillColor(sf::Color(77, 57, 105));
                    window.draw(distText);

                    LocationNode *village = districtRoot->child;
                    float vY = startY + 50.0f;

                    while (village != NULL) {
                        drawThickLine(window, {150.0f, startY + 36.0f}, {190.0f, vY}, 2.0f,
                                      sf::Color(166, 188, 204));
                        // Village Box
                        sf::RectangleShape vBox({200.0f, 32.0f});
                        vBox.setPosition({90.0f, vY});
                        vBox.setFillColor(sf::Color(222, 239, 249));
                        vBox.setOutlineThickness(1.0f);
                        vBox.setOutlineColor(sf::Color(132, 189, 218));
                        window.draw(vBox);

                        sf::Text vText(font, "+ " + village->name + " [" + village->type + "]", 12);
                        vText.setPosition({98.0f, vY + 7.0f});
                        vText.setFillColor(sf::Color(41, 85, 112));
                        window.draw(vText);

                        LocationNode *area = village->child;
                        float aY = vY + 38.0f;

                        while (area != NULL) {
                            drawThickLine(window, {190.0f, vY + 32.0f}, {230.0f, aY}, 2.0f,
                                          sf::Color(166, 188, 204));
                            // Area Box
                            sf::RectangleShape aBox({180.0f, 28.0f});
                            aBox.setPosition({140.0f, aY});
                            aBox.setFillColor(sf::Color(224, 243, 235));
                            aBox.setOutlineThickness(1.0f);
                            aBox.setOutlineColor(sf::Color(131, 195, 161));
                            window.draw(aBox);

                            sf::Text aText(font, "--> " + area->name + " [" + area->type + "]", 11);
                            aText.setPosition({148.0f, aY + 6.0f});
                            aText.setFillColor(sf::Color(48, 105, 73));
                            window.draw(aText);

                            aY += 34.0f;
                            area = area->sibling;
                        }

                        vY = (aY > vY + 40.0f) ? aY + 10.0f : vY + 44.0f;
                        village = village->sibling;
                    }
                }
            }

        } else if (currentTab == 2) {
            // ================= RELIEF REQUESTS VIEW =================
            const UiButton requestLookupButton = {
                520.0f, 212.0f, 190.0f, 46.0f, "Look up request ID",
                "Search pending requests", sf::Color(72, 163, 183)
            };
            const sf::Vector2f requestMousePosition =
                static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
            const bool requestMouseDown =
                sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
            drawControlButton(window, font, fontLoaded, requestLookupButton,
                requestMousePosition,
                requestMouseDown &&
                    contains(requestLookupButton, requestMousePosition),
                true);

            if (fontLoaded) {
                std::vector<ReliefRequest> orderedRequests = currentRequests;
                std::stable_sort(orderedRequests.begin(), orderedRequests.end(),
                    [](const ReliefRequest &left, const ReliefRequest &right) {
                        return left.priority > right.priority;
                    });

                float cardY = 270.0f;
                const std::size_t visibleRequestCount =
                    std::min<std::size_t>(orderedRequests.size(), 5);
                for (std::size_t requestIndex = 0;
                     requestIndex < visibleRequestCount; ++requestIndex) {
                    const ReliefRequest &req = orderedRequests[requestIndex];
                    sf::RectangleShape card({680.0f, 50.0f});
                    card.setPosition({30.0f, cardY});
                    card.setFillColor(sf::Color(255, 255, 255));
                    card.setOutlineThickness(1);
                    card.setOutlineColor(req.priority == 1 ? sf::Color(196, 76, 76) : sf::Color(86, 163, 201));
                    window.draw(card);

                    // Badge
                    sf::RectangleShape badge({100.0f, 24.0f});
                    badge.setPosition({40.0f, cardY + 13.0f});
                    badge.setFillColor(req.priority == 1 ? sf::Color(196, 76, 76) : sf::Color(67, 153, 192));
                    window.draw(badge);

                    sf::Text badgeText(font, req.priority == 1 ? "EMERGENCY" : "NORMAL", 11);
                    badgeText.setStyle(sf::Text::Bold);
                    badgeText.setPosition({46.0f, cardY + 17.0f});
                    window.draw(badgeText);

                    std::string info = "ID #" + std::to_string(req.id) + "  |  Village: " + req.village +
                                       "  |  Affected: " + std::to_string(req.people) + " citizens";
                    sf::Text infoText(font, info, 13);
                    infoText.setPosition({160.0f, cardY + 16.0f});
                    infoText.setFillColor(sf::Color(49, 74, 94));
                    window.draw(infoText);

                    cardY += 60.0f;
                }

                if (orderedRequests.empty()) {
                    sf::Text emptyMessage(font, "No active requests. Add an emergency from the live demo panel.", 12);
                    emptyMessage.setPosition({40.0f, 280.0f});
                    emptyMessage.setFillColor(sf::Color(91, 117, 139));
                    window.draw(emptyMessage);
                    cardY = 255.0f;
                } else if (orderedRequests.size() > visibleRequestCount) {
                    sf::Text remainingMessage(font,
                        std::to_string(orderedRequests.size() - visibleRequestCount) +
                        " additional active request(s) are queued.",
                        10);
                    remainingMessage.setPosition({40.0f, cardY});
                    remainingMessage.setFillColor(sf::Color(91, 117, 139));
                    window.draw(remainingMessage);
                    cardY += 25.0f;
                }

                const std::vector<OperationRecord> recentOperations = operationHistory.getAll();
                if (!recentOperations.empty() && cardY + 36.0f < 640.0f) {
                    sf::Text historyTitle(font, "PROCESSED OPERATIONS  (newest first)", 10);
                    historyTitle.setStyle(sf::Text::Bold);
                    historyTitle.setPosition({40.0f, cardY + 3.0f});
                    historyTitle.setFillColor(sf::Color(54, 91, 112));
                    window.draw(historyTitle);

                    const std::size_t historyRows = std::min<std::size_t>(
                        recentOperations.size(),
                        static_cast<std::size_t>((632.0f - cardY - 25.0f) / 25.0f));
                    for (std::size_t i = 0; i < historyRows; ++i) {
                        const OperationRecord &operation = recentOperations[i];
                        std::string shelterSummary;
                        for (std::size_t j = 0;
                             j < operation.assignments.size() && j < 2; ++j) {
                            if (!shelterSummary.empty()) shelterSummary += ", ";
                            shelterSummary += operation.assignments[j].shelterName +
                                " " + std::to_string(operation.assignments[j].evacuees);
                        }
                        if (operation.assignments.size() > 2) shelterSummary += ", ...";

                        sf::Text historyRow(font,
                            "#" + std::to_string(operation.requestId) + "  " +
                            operation.village + "  |  " +
                            std::to_string(operation.people) + " evacuees  |  " +
                            shelterSummary,
                            10);
                        historyRow.setPosition({48.0f, cardY + 24.0f +
                                                 static_cast<float>(i) * 25.0f});
                        historyRow.setFillColor(sf::Color(73, 99, 117));
                        window.draw(historyRow);
                    }
                }

            }
        }

        if (dialogMode != DialogMode::None) {
            sf::RectangleShape backdrop({1040.0f, 680.0f});
            backdrop.setFillColor(sf::Color(20, 37, 52, 175));
            window.draw(backdrop);

            const bool compactDialog = dialogMode == DialogMode::AddLocation;
            const bool routeResultDialog = dialogMode == DialogMode::ShowRoute;
            const float dialogHeight = compactDialog ? 260.0f : routeResultDialog ? 460.0f : 350.0f;
            sf::RectangleShape dialog({520.0f, dialogHeight});
            dialog.setPosition({260.0f, compactDialog ? 210.0f : 170.0f});
            dialog.setFillColor(sf::Color(255, 255, 255));
            dialog.setOutlineThickness(1.0f);
            dialog.setOutlineColor(sf::Color(213, 224, 236));
            window.draw(dialog);

            sf::RectangleShape dialogHeader({520.0f, 54.0f});
            dialogHeader.setPosition({260.0f, compactDialog ? 210.0f : 170.0f});
            dialogHeader.setFillColor(sf::Color(26, 63, 91));
            window.draw(dialogHeader);

            if (fontLoaded) {
                const std::string titleText =
                    dialogMode == DialogMode::AddLocation ? "Register a location" :
                    dialogMode == DialogMode::ManageRoad ? "Road connection manager" :
                    dialogMode == DialogMode::CheckRoute ? "Find an open-road route" :
                    dialogMode == DialogMode::ShowRoute ? "BFS route result" :
                    dialogMode == DialogMode::AddShelter ? "Register a shelter" :
                    dialogMode == DialogMode::ConnectRegions ? "Connect shelter regions" :
                    dialogMode == DialogMode::LookupRequest ? "Look up a request" :
                    dialogMode == DialogMode::ReleaseShelter ? "Release shelter capacity" :
                    "Create a relief request";
                sf::Text title(font, titleText, 19);
                title.setStyle(sf::Text::Bold);
                title.setPosition({284.0f, compactDialog ? 226.0f : 186.0f});
                title.setFillColor(sf::Color(255, 255, 255));
                window.draw(title);

                if (dialogMode == DialogMode::AddLocation) {
                    sf::Text prompt(font, "Add a village to the location tree. Roads are set separately.", 11);
                    prompt.setPosition({295.0f, 278.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);

                    drawDialogField(window, font, "VILLAGE NAME", locationNameInput + "_",
                                    307.0f, sf::Color(126, 117, 224));
                    sf::Text hint(font, "Enter to save  |  Backspace to edit  |  Esc to cancel", 9);
                    hint.setPosition({295.0f, 367.0f});
                    hint.setFillColor(sf::Color(115, 131, 150));
                    window.draw(hint);
                    drawDialogButton(window, font, "Save", 545.0f, 400.0f,
                                     sf::Color(89, 108, 211));
                    drawDialogButton(window, font, "Cancel", 660.0f, 400.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::AddShelter &&
                           locationCount > 0) {
                    sf::Text prompt(font,
                        "Register shelter beds at a region in the location network.",
                        11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);

                    drawDialogField(window, font, "SHELTER NAME  (CLICK, THEN TYPE)",
                        shelterNameInput + (shelterInputFocus == 0 ? "_" : ""),
                        280.0f, sf::Color(42, 178, 155));
                    drawDialogField(window, font, "REGION  (CLICK TO CHANGE)",
                        locationNames[shelterRegionSelection], 338.0f,
                        sf::Color(71, 150, 224));
                    drawDialogField(window, font, "BED CAPACITY  (CLICK, THEN TYPE)",
                        shelterCapacityInput +
                            (shelterInputFocus == 1 ? "_" : ""),
                        396.0f, sf::Color(244, 174, 68));
                    drawDialogButton(window, font, "Create", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::ManageRoad && locationCount > 1) {
                    sf::Text prompt(font, "Select any two locations, then choose their road status.", 11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);

                    drawDialogField(window, font, "FROM  (CLICK TO CHANGE)",
                                    locationNames[roadSourceSelection], 280.0f,
                                    sf::Color(71, 150, 224));
                    drawDialogField(window, font, "TO  (CLICK TO CHANGE)",
                                    locationNames[roadDestinationSelection], 338.0f,
                                    sf::Color(42, 178, 155));
                    const std::string statuses[] = {"No road", "Open", "Blocked"};
                    const sf::Color statusColors[] = {
                        sf::Color(110, 126, 148), sf::Color(34, 139, 98),
                        sf::Color(190, 68, 68)
                    };
                    drawDialogField(window, font, "ROAD STATUS  (CLICK TO CYCLE)",
                                    statuses[roadStatusSelection], 396.0f,
                                    statusColors[roadStatusSelection]);
                    drawDialogButton(window, font, "Apply", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::ConnectRegions &&
                           locationCount > 1) {
                    sf::Text prompt(font,
                        "Open a two-way road so evacuees can reach shelters in either region.",
                        11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);
                    drawDialogField(window, font, "FROM REGION  (CLICK TO CHANGE)",
                        locationNames[roadSourceSelection], 280.0f,
                        sf::Color(71, 150, 224));
                    drawDialogField(window, font, "TO REGION  (CLICK TO CHANGE)",
                        locationNames[roadDestinationSelection], 338.0f,
                        sf::Color(42, 178, 155));
                    drawDialogButton(window, font, "Connect", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::LookupRequest) {
                    sf::Text prompt(font,
                        "Search the pending-request hash table by request ID.",
                        11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);
                    drawDialogField(window, font, "REQUEST ID  (TYPE A NUMBER)",
                        requestLookupInput + "_", 280.0f,
                        sf::Color(72, 163, 183));
                    drawDialogButton(window, font, "Search", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::ReleaseShelter) {
                    sf::Text prompt(font,
                        "Release all beds allocated to a processed request.",
                        11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);
                    drawDialogField(window, font, "PROCESSED REQUEST ID",
                        requestLookupInput + "_", 280.0f,
                        sf::Color(123, 113, 226));
                    drawDialogButton(window, font, "Release", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::CheckRoute && locationCount > 1) {
                    sf::Text prompt(font, "Choose any registered locations. BFS follows open roads only.",
                                    11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);

                    drawDialogField(window, font, "CURRENT LOCATION  (CLICK TO CHANGE)",
                                    locationNames[routeSourceSelection], 280.0f,
                                    sf::Color(71, 150, 224));
                    drawDialogField(window, font, "DESTINATION  (CLICK TO CHANGE)",
                                    locationNames[routeDestinationSelection], 338.0f,
                                    sf::Color(42, 178, 155));
                    drawDialogButton(window, font, "Find route", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                } else if (dialogMode == DialogMode::ShowRoute) {
                    sf::Text summary(font,
                        locationNames[routeSourceSelection] + " to " +
                        locationNames[routeDestinationSelection] + "  |  " +
                        std::to_string(currentRoute.size() - 1) + " road(s)",
                        12);
                    summary.setStyle(sf::Text::Bold);
                    summary.setPosition({295.0f, 235.0f});
                    summary.setFillColor(sf::Color(34, 139, 98));
                    window.draw(summary);

                    const std::size_t visibleRouteEntries = 8;
                    const std::size_t routeEnd = std::min(
                        routeScrollOffset + visibleRouteEntries, currentRoute.size());
                    for (std::size_t routeIndex = routeScrollOffset;
                         routeIndex < routeEnd; ++routeIndex) {
                        const float rowY = 263.0f +
                            static_cast<float>(routeIndex - routeScrollOffset) * 34.0f;
                        sf::RectangleShape routeRow({450.0f, 29.0f});
                        routeRow.setPosition({295.0f, rowY});
                        routeRow.setFillColor(routeIndex % 2 == 0
                            ? sf::Color(241, 247, 251) : sf::Color(255, 255, 255));
                        window.draw(routeRow);

                        sf::Text stepText(font,
                            std::to_string(routeIndex + 1) + ".  " +
                            locationNames[currentRoute[routeIndex]], 12);
                        stepText.setPosition({307.0f, rowY + 6.0f});
                        stepText.setFillColor(sf::Color(40, 69, 94));
                        window.draw(stepText);
                    }

                    if (currentRoute.size() > visibleRouteEntries) {
                        sf::Text scrollHint(font, "Use Up/Down or Page Up/Page Down to view the full route.",
                                            9);
                        scrollHint.setPosition({295.0f, 545.0f});
                        scrollHint.setFillColor(sf::Color(102, 120, 140));
                        window.draw(scrollHint);
                    }
                    drawDialogButton(window, font, "Close", 545.0f, 576.0f,
                                     sf::Color(71, 112, 148));
                } else if (dialogMode == DialogMode::AddRequest && locationCount > 0) {
                    sf::Text prompt(font, "Record the affected community and urgency for dispatch.", 11);
                    prompt.setPosition({315.0f, 235.0f});
                    prompt.setFillColor(sf::Color(92, 111, 132));
                    window.draw(prompt);

                    drawDialogField(window, font, "LOCATION  (CLICK TO CHANGE)",
                                    locationNames[requestLocationSelection], 280.0f,
                                    sf::Color(71, 150, 224));
                    drawDialogField(window, font, "PEOPLE AFFECTED  (TYPE A NUMBER)",
                                    requestPeopleInput + "_", 338.0f,
                                    sf::Color(244, 174, 68));
                    sf::Text priorityLabel(font, "REQUEST TYPE  (SELECT ONE)", 10);
                    priorityLabel.setStyle(sf::Text::Bold);
                    priorityLabel.setPosition({315.0f, 379.0f});
                    priorityLabel.setFillColor(sf::Color(83, 105, 130));
                    window.draw(priorityLabel);

                    const sf::Color emergencyFill = requestPrioritySelection == 1
                        ? sf::Color(222, 93, 89) : sf::Color(250, 242, 242);
                    const sf::Color normalFill = requestPrioritySelection == 0
                        ? sf::Color(71, 150, 224) : sf::Color(240, 246, 252);
                    sf::RectangleShape emergencyChoice({195.0f, 42.0f});
                    emergencyChoice.setPosition({315.0f, 396.0f});
                    emergencyChoice.setFillColor(emergencyFill);
                    emergencyChoice.setOutlineThickness(1.0f);
                    emergencyChoice.setOutlineColor(sf::Color(222, 93, 89));
                    window.draw(emergencyChoice);
                    sf::RectangleShape normalChoice({195.0f, 42.0f});
                    normalChoice.setPosition({530.0f, 396.0f});
                    normalChoice.setFillColor(normalFill);
                    normalChoice.setOutlineThickness(1.0f);
                    normalChoice.setOutlineColor(sf::Color(71, 150, 224));
                    window.draw(normalChoice);

                    sf::Text emergencyLabel(font, "EMERGENCY", 11);
                    emergencyLabel.setStyle(sf::Text::Bold);
                    emergencyLabel.setPosition({364.0f, 409.0f});
                    emergencyLabel.setFillColor(requestPrioritySelection == 1
                        ? sf::Color(255, 255, 255) : sf::Color(160, 69, 68));
                    window.draw(emergencyLabel);
                    sf::Text normalLabel(font, "NORMAL", 11);
                    normalLabel.setStyle(sf::Text::Bold);
                    normalLabel.setPosition({595.0f, 409.0f});
                    normalLabel.setFillColor(requestPrioritySelection == 0
                        ? sf::Color(255, 255, 255) : sf::Color(57, 105, 147));
                    window.draw(normalLabel);

                    drawDialogButton(window, font, "Create", 545.0f, 456.0f,
                                     sf::Color(37, 155, 137));
                    drawDialogButton(window, font, "Cancel", 660.0f, 456.0f,
                                     sf::Color(112, 130, 150));
                }
            }
        }

        window.display();
    }

    return 0;
}

#else

#include <iostream>

#include "LocationManagement.h"
#include "RequestManagement.h"

int main()
{
    int choice = 0;
    do
    {
        std::cout << "\nFloodOps Console\n"
                  << "1. Location and road management\n"
                  << "2. Relief request management\n"
                  << "3. Exit\n"
                  << "Choice: ";

        if (!(std::cin >> choice))
        {
            std::cerr << "\nInvalid input. Exiting.\n";
            return 1;
        }

        switch (choice)
        {
            case 1:
                locationMenu();
                break;
            case 2:
                requestMenu();
                break;
            case 3:
                break;
            default:
                std::cout << "Choose 1, 2, or 3.\n";
        }
    } while (choice != 3);

    return 0;
}

#endif