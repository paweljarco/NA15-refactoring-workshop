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

class Map {

public:
    Map() = default;
    Map(int width, int height) {
        m_mapDimension = std::make_pair(width, height);
    }

    std::pair<int, int> mapDimension() const { 
        return m_mapDimension; 
    }

    bool isPositionOutsideMap(int x, int y) const {
        return x < 0 or y < 0 or x >= m_mapDimension.first or y >= m_mapDimension.second;
    }
private:
    std::pair<int, int> m_mapDimension;
};
class SegmentColection {

public:
    bool isSegmentAtPosition(int x, int y) const;
    void updateSegmentsIfSuccessfullMove(Segment const& newHead, Map const& m_map);
    void addHeadSegment(Segment const& newHead), IPort& m_displayPort;
    void removeTailSegmentIfNotScored(Segment const& newHead);
    void removeTailSegment();
    void add(Segment segment) {
        m_segments.push_back(segment);
    }
    Segment front() {
        return m_segments.front();
    }

private:
    std::list<Segment> m_segments;
};


class Segment {

public:
    Segment() = default;
    Segment(int x, int y) {
        position = std::make_pair(x,y);
    }

    std::pair<int, int> getPosition() const{
        return position;
    }

private:
    std::pair<int, int> position;
};

class Controller : public IEventHandler
{
public:
    Controller(IPort& p_displayPort, IPort& p_foodPort, IPort& p_scorePort, std::string const& p_config);

    Controller(Controller const& p_rhs) = delete;
    Controller& operator=(Controller const& p_rhs) = delete;

    void receive(std::unique_ptr<Event> e) override;

private:
    IPort& m_displayPort;
    IPort& m_foodPort;
    IPort& m_scorePort;

    Map m_map;
    SegmentColection segments;
    std::pair<int, int> m_foodPosition;
/*
    struct Segment
    {
        int x;
        int y;
    };
*/
    ;
    Direction m_currentDirection;

    void handleTimeoutInd();
    void handleDirectionInd(std::unique_ptr<Event>);
    void handleFoodInd(std::unique_ptr<Event>);
    void handleFoodResp(std::unique_ptr<Event>);
    void handlePauseInd(std::unique_ptr<Event>);

    
    Segment calculateNewHead(SegmentColection& m_segments) const;
    void updateFoodPosition(int x, int y, std::function<void()> clearPolicy);
    void sendClearOldFood();
    void sendPlaceNewFood(int x, int y);

    bool m_paused;
};

} // namespace Snake
