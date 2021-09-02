#include "SnakeController.hpp"

#include <algorithm>
#include <sstream>

#include "EventT.hpp"
#include "IPort.hpp"

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
        //m_mapDimension = std::make_pair(width, height);
        m_map = Map(width, height);
        m_map.setFoodPosition(foodX, foodY);

        istr >> d;
        switch (d) {
            case 'U':
                m_currentDirection = Direction_UP;
                break;
            case 'D':
                m_currentDirection = Direction_DOWN;
                break;
            case 'L':
                m_currentDirection = Direction_LEFT;
                break;
            case 'R':
                m_currentDirection = Direction_RIGHT;
                break;
            default:
                throw ConfigurationError();
        }
        istr >> length;

        while (length--) {
            Segment seg;
            int x,y;
            istr >> x >> y;
            seg = Segment(x,y);
            segments.add(seg);
        }
    } else {
        throw ConfigurationError();
    }
}

bool SegmentColection::isSegmentAtPosition(int x, int y) const
{
    return m_segments.end() !=  std::find_if(m_segments.cbegin(), m_segments.cend(),
        [x, y](auto const& segment){ return segment.getPosition().first == x and segment.getPosition().second == y; });
}

void Map::sendPlaceNewFood(int x, int y, Controller& controller)
{
    m_foodPosition = std::make_pair(x, y);

    DisplayInd placeNewFood;
    placeNewFood.x = x;
    placeNewFood.y = y;
    placeNewFood.value = Cell_FOOD;

    controller.getDisplayPort().send(std::make_unique<EventT<DisplayInd>>(placeNewFood));
}

void Map::sendClearOldFood(Controller& controller)
{
    DisplayInd clearOldFood;
    clearOldFood.x = m_foodPosition.first;
    clearOldFood.y = m_foodPosition.second;
    clearOldFood.value = Cell_FREE;

    controller.getDisplayPort().send(std::make_unique<EventT<DisplayInd>>(clearOldFood));
}

namespace
{
bool isHorizontal(Direction direction)
{
    return Direction_LEFT == direction or Direction_RIGHT == direction;
}

bool isVertical(Direction direction)
{
    return Direction_UP == direction or Direction_DOWN == direction;
}

bool isPositive(Direction direction)
{
    return (isVertical(direction) and Direction_DOWN == direction)
        or (isHorizontal(direction) and Direction_RIGHT == direction);
}

bool perpendicular(Direction dir1, Direction dir2)
{
    return isHorizontal(dir1) == isVertical(dir2);
}
} // namespace

Segment Map::calculateNewHead(Controller& controller ) const
{
    Segment const& currentHead = controller.getSegments().front();

    Segment newHead;
    newHead.getPosition().first = currentHead.getPosition().first + (isHorizontal(controller.getCurrentDirection()) ? isPositive(controller.getCurrentDirection()) ? 1 : -1 : 0);
    newHead.getPosition().second = currentHead.getPosition().second + (isVertical(controller.getCurrentDirection()) ? isPositive(controller.getCurrentDirection()) ? 1 : -1 : 0);

    return newHead;
}

void SegmentColection::removeTailSegment(Controller& controller )
{
    auto tail = m_segments.back();

    DisplayInd l_evt;
    l_evt.x = tail.getPosition().first;
    l_evt.y = tail.getPosition().second;
    l_evt.value = Cell_FREE;
    controller.getDisplayPort().send(std::make_unique<EventT<DisplayInd>>(l_evt));

    m_segments.pop_back();
}

void SegmentColection::addHeadSegment(Segment const& newHead, Controller& controller)
{
    m_segments.push_front(newHead);

    DisplayInd placeNewHead;
    placeNewHead.x = newHead.getPosition().first;
    placeNewHead.y = newHead.getPosition().second;
    placeNewHead.value = Cell_SNAKE;

    controller.getDisplayPort().send(std::make_unique<EventT<DisplayInd>>(placeNewHead));
}

void SegmentColection::removeTailSegmentIfNotScored(Segment const& newHead, Controller& controller)
{
    if (std::make_pair(newHead.getPosition().first, newHead.getPosition().second) == controller.getMap().getFoodPosition()) {
        controller.getScorePort().send(std::make_unique<EventT<ScoreInd>>());
        controller.getFoodPort().send(std::make_unique<EventT<FoodReq>>());
    } else {
        removeTailSegment(controller);
    }
}

void SegmentColection::updateSegmentsIfSuccessfullMove(Segment const& newHead, Controller& controller)
{
    if (isSegmentAtPosition(newHead.getPosition().first, newHead.getPosition().second) or 
        controller.getMap().isPositionOutsideMap(newHead.getPosition().first, newHead.getPosition().second)) {
        controller.getScorePort().send(std::make_unique<EventT<LooseInd>>());
    } else {
        addHeadSegment(newHead, controller);
        removeTailSegmentIfNotScored(newHead, controller);
    }
}

void Controller::handleTimeoutInd()
{
    segments.updateSegmentsIfSuccessfullMove(m_map.calculateNewHead(*this), *this);
}

void Controller::handleDirectionInd(std::unique_ptr<Event> e)
{
    auto direction = payload<DirectionInd>(*e).direction;

    if (perpendicular(m_currentDirection, direction)) {
        m_currentDirection = direction;
    }
}

void Map::updateFoodPosition(int x, int y, Controller& controller, std::function<void()> clearPolicy)
{
    if (controller.getSegments().isSegmentAtPosition(x, y) || controller.getMap().isPositionOutsideMap(x,y)) {
        controller.getFoodPort().send(std::make_unique<EventT<FoodReq>>());
        return;
    }

    clearPolicy();
    sendPlaceNewFood(x, y, controller);
}

void Controller::handleFoodInd(std::unique_ptr<Event> e)
{
    auto receivedFood = payload<FoodInd>(*e);

    m_map.updateFoodPosition(receivedFood.x, receivedFood.y, *this, [&](){m_map.sendClearOldFood(*this);}); // std::bind<Controller>(&Map::sendClearOldFood, this)
}

void Controller::handleFoodResp(std::unique_ptr<Event> e)
{
    auto requestedFood = payload<FoodResp>(*e);

    m_map.updateFoodPosition(requestedFood.x, requestedFood.y, *this,[]{});
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
