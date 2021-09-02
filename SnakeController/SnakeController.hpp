#pragma once

#include <list>
#include <memory>
#include <functional>
#include <stdexcept>

#include "IEventHandler.hpp"
#include "SnakeInterface.hpp"

class Event;
class IPort;

namespace Snake
{
struct ConfigurationError : std::logic_error
{
    ConfigurationError();
};

struct UnexpectedEventException : std::runtime_error
{
    UnexpectedEventException();
};

class Segment {

public:
    Segment() = default;
    Segment(int x, int y) {
        position = std::make_pair(x,y);
    }

    std::pair<int, int>& getPosition(){
        return position;
    }
    std::pair<int, int> getPosition() const{
        return position;
    }

private:
    std::pair<int, int> position;
};

class Controller;

class Map {

public:
    Map() = default;
    Map(int width, int height) {
        m_mapDimension = std::make_pair(width, height);
    }

    Segment calculateNewHead(Controller& controller) const;
    void updateFoodPosition(int x, int y, Controller& controller, std::function<void()> clearPolicy);
    void sendClearOldFood(Controller& controller);
    void sendPlaceNewFood(int x, int y, Controller& controller);

    std::pair<int, int>& getMapDimension()  { 
        return m_mapDimension; 
    }
    
    void setFoodPosition(int x, int y) {
        m_foodPosition = std::make_pair(x,y);
    } 

    std::pair<int, int>& getFoodPosition()  { 
        return m_foodPosition; 
    }

    bool isPositionOutsideMap(int x, int y) const {
        return x < 0 or y < 0 or x >= m_mapDimension.first or y >= m_mapDimension.second;
    }
private:
    std::pair<int, int> m_mapDimension;
    std::pair<int, int> m_foodPosition;
};

class SegmentColection {

public:
    bool isSegmentAtPosition(int x, int y) const;
    void updateSegmentsIfSuccessfullMove(Segment const& newHead,  Controller& controller);
    void addHeadSegment(Segment const& newHead, Controller& controller);
    void removeTailSegmentIfNotScored(Segment const& newHead, Controller& controller);
    void removeTailSegment(Controller& controller );
    void add(Segment segment) {
        m_segments.push_back(segment);
    }
    Segment front() {
        return m_segments.front();
    }

private:
    std::list<Segment> m_segments;
};



class Controller : public IEventHandler
{
public:
    Controller(IPort& p_displayPort, IPort& p_foodPort, IPort& p_scorePort, std::string const& p_config);

    Controller(Controller const& p_rhs) = delete;
    Controller& operator=(Controller const& p_rhs) = delete;

    void receive(std::unique_ptr<Event> e) override;

    IPort& getDisplayPort() {
        return m_displayPort;
    }
    IPort& getFoodPort() {
        return m_foodPort;
    }
    IPort& getScorePort() {
        return m_scorePort;
    }

    Map& getMap(){
        return m_map;
    }

    Direction& getCurrentDirection() {
        return m_currentDirection;
    }

    SegmentColection& getSegments() {
        return segments;
    }


private:
    IPort& m_displayPort;
    IPort& m_foodPort;
    IPort& m_scorePort;

    Map m_map;
    SegmentColection segments;

    Direction m_currentDirection;

    void handleTimeoutInd();
    void handleDirectionInd(std::unique_ptr<Event>);
    void handleFoodInd(std::unique_ptr<Event>);
    void handleFoodResp(std::unique_ptr<Event>);
    void handlePauseInd(std::unique_ptr<Event>);

    
 

    bool m_paused;
};

} // namespace Snake
