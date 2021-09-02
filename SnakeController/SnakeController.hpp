#pragma once

#include <list>
#include <memory>
#include <functional>
#include <stdexcept>

#include "IEventHandler.hpp"
#include "SnakeInterface.hpp"
#include "Map.hpp"
#include "SegmentColection.hpp"
#include "Segment.hpp"

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
