#include "SnakeController.hpp"

#include <algorithm>
#include <sstream>

#include "EventT.hpp"
#include "IPort.hpp"

#include "SnakeSegments.hpp"
#include "SnakeWorld.hpp"

namespace Snake
{

ConfigurationError::ConfigurationError()
    : std::logic_error("Bad configuration of Snake::Controller.")
{}

UnexpectedEventException::UnexpectedEventException()
    : std::runtime_error("Unexpected event received!")
{}

Controller::Controller(IPort& p_displayPort, IPort& p_foodPort, IPort& p_scorePort, std::string const& p_config)
    : m_displayPort(p_displayPort),
      m_foodPort(p_foodPort),
      m_scorePort(p_scorePort),
      m_paused(false)
{
    std::istringstream istr(p_config);
    char w, f, s, d;

    int width, height, length;
    int foodX, foodY;
    istr >> w >> width >> height >> f >> foodX >> foodY >> s;

    if (w == 'W' and f == 'F' and s == 'S') {
        m_world = std::make_unique<World>(Dimension({width, height}), Position({foodX, foodY}));

        Direction startDirection;
        istr >> d;
        switch (d) {
            case 'U':
                startDirection = Direction_UP;
                break;
            case 'D':
                startDirection = Direction_DOWN;
                break;
            case 'L':
                startDirection = Direction_LEFT;
                break;
            case 'R':
                startDirection = Direction_RIGHT;
                break;
            default:
                throw ConfigurationError();
        }
        m_segments = std::make_unique<Segments>(startDirection);
        istr >> length;

        while (length--) {
            int x, y;
            istr >> x >> y;
            m_segments->addSegment(x, y);
        }
    } else {
        throw ConfigurationError();
    }
}

Controller::~Controller()
{}

void Controller::sendPlaceNewFood(int x, int y)
{
    m_world->setFoodPosition(Position({x, y}));

    DisplayInd placeNewFood;
    placeNewFood.position.x = x;
    placeNewFood.position.y = y;
    placeNewFood.value = Cell_FOOD;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(placeNewFood));
}

void Controller::sendClearOldFood()
{
    auto foodPosition = m_world->getFoodPosition();

    DisplayInd clearOldFood;
    clearOldFood.position.x = foodPosition.x;
    clearOldFood.position.y = foodPosition.y;
    clearOldFood.value = Cell_FREE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(clearOldFood));
}

void Controller::removeTailSegment()
{
    auto tail = m_segments->removeTail();

    DisplayInd clearTail;
    clearTail.position.x = tail.first;
    clearTail.position.y = tail.second;
    clearTail.value = Cell_FREE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(clearTail));
}

void Controller::addHeadSegment(int x, int y)
{
    m_segments->addHead(x, y);

    DisplayInd placeNewHead;
    placeNewHead.position.x = x;
    placeNewHead.position.y = y;
    placeNewHead.value = Cell_SNAKE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(placeNewHead));
}

void Controller::removeTailSegmentIfNotScored(int x, int y)
{
    if (Position({x, y}) == m_world->getFoodPosition()) {
        m_scorePort.send(std::make_unique<EventT<ScoreInd>>());
        m_foodPort.send(std::make_unique<EventT<FoodReq>>());
    } else {
        removeTailSegment();
    }
}

void Controller::updateSegmentsIfSuccessfullMove(int x, int y)
{
    if (m_segments->isCollision(x, y) or not m_world->contains(x, y)) {
        m_scorePort.send(std::make_unique<EventT<LooseInd>>());
    } else {
        addHeadSegment(x, y);
        removeTailSegmentIfNotScored(x, y);
    }
}

void Controller::handleTimeoutInd()
{
    auto newHead = m_segments->nextHead();
    updateSegmentsIfSuccessfullMove(newHead.first, newHead.second);
}

void Controller::handleDirectionInd(std::unique_ptr<Event> e)
{
    m_segments->updateDirection(payload<DirectionInd>(*e).direction);
}

void Controller::updateFoodPosition(int x, int y, std::function<void()> clearPolicy)
{
    if (m_segments->isCollision(x, y) or not m_world->contains(x, y)) {
        m_foodPort.send(std::make_unique<EventT<FoodReq>>());
        return;
    }

    clearPolicy();
    sendPlaceNewFood(x, y);
}

void Controller::handleFoodInd(std::unique_ptr<Event> e)
{
    auto receivedFood = payload<FoodInd>(*e);

    updateFoodPosition(receivedFood.position.x, receivedFood.position.y, std::bind(&Controller::sendClearOldFood, this));
}

void Controller::handleFoodResp(std::unique_ptr<Event> e)
{
    auto requestedFood = payload<FoodResp>(*e);

    updateFoodPosition(requestedFood.position.x, requestedFood.position.y, []{});
}

void Controller::handlePauseInd(std::unique_ptr<Event> e)
{
    m_paused = not m_paused;
}

void Controller::receive(std::unique_ptr<Event> e)
{
    switch (e->getMessageId()) {
        case TimeoutInd::MESSAGE_ID:
            if (!m_paused) {
                return handleTimeoutInd();
            }
            return;
        case DirectionInd::MESSAGE_ID:
            if (!m_paused) {
                return handleDirectionInd(std::move(e));
            }
            return;
        case FoodInd::MESSAGE_ID:
            return handleFoodInd(std::move(e));
        case FoodResp::MESSAGE_ID:
            return handleFoodResp(std::move(e));
        case PauseInd::MESSAGE_ID:
            return handlePauseInd(std::move(e));
        default:
            throw UnexpectedEventException();
    }
}

} // namespace Snake
