// verify_collision_geometry.cpp
// XML Game Engine
// author: beefviper
// date: Sept 27, 2026
//
// Not part of the build (see CMakeLists.txt - it isn't listed there).
// A standalone, no-dependencies numeric check of the edge/sign math in
// CollisionDetector::rectangleRectangle/circleRectangle and
// CommandExecutor's bounceOffEdge, kept around for the next time this area
// changes. It only touches SFML's header-only Vector2/Rect types (no
// window/graphics module, so it builds and runs even where the rest of the
// engine can't link, e.g. without X11 dev headers). Build and run it with:
//   g++ -std=c++20 -I<path-to-SFML-include> tests/verify_collision_geometry.cpp -o verify && ./verify

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>

enum class Edge { Top, Bottom, Left, Right };

const char* name(Edge e)
{
    switch (e)
    {
    case Edge::Top: return "Top";
    case Edge::Bottom: return "Bottom";
    case Edge::Left: return "Left";
    case Edge::Right: return "Right";
    }
    return "?";
}

Edge opposite(Edge edge)
{
    switch (edge)
    {
    case Edge::Left: return Edge::Right;
    case Edge::Right: return Edge::Left;
    case Edge::Top: return Edge::Bottom;
    case Edge::Bottom: return Edge::Top;
    }
    return edge;
}

// --- copy of CollisionDetector::rectangleRectangle, taking raw bounds ---
std::optional<Edge> rectangleRectangle(sf::FloatRect boundsA, sf::FloatRect boundsB)
{
    const float overlapLeft = (boundsA.position.x + boundsA.size.x) - boundsB.position.x;
    const float overlapRight = (boundsB.position.x + boundsB.size.x) - boundsA.position.x;
    const float overlapTop = (boundsA.position.y + boundsA.size.y) - boundsB.position.y;
    const float overlapBottom = (boundsB.position.y + boundsB.size.y) - boundsA.position.y;

    if (overlapLeft <= 0 || overlapRight <= 0 || overlapTop <= 0 || overlapBottom <= 0)
    {
        return std::nullopt;
    }

    const float overlapX = std::min(overlapLeft, overlapRight);
    const float overlapY = std::min(overlapTop, overlapBottom);

    if (overlapX < overlapY)
    {
        return (overlapLeft < overlapRight) ? Edge::Left : Edge::Right;
    }

    return (overlapTop < overlapBottom) ? Edge::Top : Edge::Bottom;
}

// --- copy of CollisionDetector::circleRectangle, taking raw center/radius/rect ---
std::optional<Edge> circleRectangle(sf::Vector2f circleCenter, float circleRadius, sf::FloatRect rectBounds)
{
    const auto midpoint = circleCenter;
    const auto rectLeft = rectBounds.position.x;
    const auto rectRight = rectLeft + rectBounds.size.x;
    const auto rectTop = rectBounds.position.y;
    const auto rectBottom = rectTop + rectBounds.size.y;

    sf::Vector2f nearestPoint;
    nearestPoint.x = std::clamp(midpoint.x, rectLeft, rectRight);
    nearestPoint.y = std::clamp(midpoint.y, rectTop, rectBottom);

    const auto rayToNearest = nearestPoint - midpoint;
    const auto distance = std::sqrt(rayToNearest.x * rayToNearest.x + rayToNearest.y * rayToNearest.y);

    auto overlap = circleRadius - distance;
    if (std::isnan(overlap)) overlap = 0;
    if (overlap <= 0) return std::nullopt;

    if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectLeft) return Edge::Left;
    if (midpoint.y > rectTop - midpoint.y && midpoint.y < rectBottom + midpoint.y && nearestPoint.x == rectRight) return Edge::Right;
    if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectTop) return Edge::Top;
    if (midpoint.x > rectLeft - midpoint.x && midpoint.x < rectRight + midpoint.x && nearestPoint.y == rectBottom) return Edge::Bottom;
    return std::nullopt;
}

// applies bounceOffEdge's sign rule to a velocity, given a *self-relative* edge
sf::Vector2f bounceOffEdge(sf::Vector2f v, Edge edge)
{
    switch (edge)
    {
    case Edge::Left:   v.x = std::abs(v.x); break;
    case Edge::Right:  v.x = -std::abs(v.x); break;
    case Edge::Top:    v.y = std::abs(v.y); break;
    case Edge::Bottom: v.y = -std::abs(v.y); break;
    }
    return v;
}

int failures = 0;
void expect(bool cond, const char* what)
{
    std::printf("%s: %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) failures++;
}

int main()
{
    // Scenario 1: rectangle A (moving right, vx=+3) approaches rectangle B from the left.
    {
        sf::FloatRect a({ 90.f, 0.f }, { 20.f, 20.f });   // right edge at 110
        sf::FloatRect b({ 100.f, 0.f }, { 20.f, 20.f });  // left edge at 100 -> overlap of 10 on X, full 20 on Y
        auto edgeOfB = rectangleRectangle(a, b);
        expect(edgeOfB && *edgeOfB == Edge::Left, "rect A-from-left hits B's Left edge");

        sf::Vector2f va{ 3.f, 0.f };
        auto va2 = bounceOffEdge(va, opposite(*edgeOfB));
        expect(va2.x < 0, "mover A bounces back leftward (away from B)");

        // If B is ALSO moving (e.g. two bouncy objects), its own bounce
        // correctly reflects using its own current velocity...
        sf::Vector2f vbMoving{ -1.f, 0.f };
        auto vbMoving2 = bounceOffEdge(vbMoving, *edgeOfB);
        expect(vbMoving2.x > 0, "a *moving* target B reflects rightward (away from A)");

        // ...but bounceOffEdge reflects existing velocity, it doesn't impart
        // a new one: a target sitting at rest gets no impulse from being hit.
        // None of the 4 sample games rely on a stationary object bouncing
        // (only the always-moving ball/bullet declare 'bounce'), so this is a
        // known, documented limitation rather than a bug to fix here.
        sf::Vector2f vbAtRest{ 0.f, 0.f };
        auto vbAtRest2 = bounceOffEdge(vbAtRest, *edgeOfB);
        expect(vbAtRest2.x == 0.f, "(documented limitation) a stationary target stays at rest, even if it declares bounce");
    }

    // Scenario 2: rectangle A (moving left, vx=-3) approaches rectangle B from the right.
    {
        sf::FloatRect a({ 110.f, 0.f }, { 20.f, 20.f });  // left edge at 110
        sf::FloatRect b({ 100.f, 0.f }, { 20.f, 20.f });  // right edge at 120 -> overlap of 10 on X
        auto edgeOfB = rectangleRectangle(a, b);
        expect(edgeOfB && *edgeOfB == Edge::Right, "rect A-from-right hits B's Right edge");

        sf::Vector2f va{ -3.f, 0.f };
        auto va2 = bounceOffEdge(va, opposite(*edgeOfB));
        expect(va2.x > 0, "mover A bounces back rightward (away from B)");
    }

    // Scenario 3: rectangle A approaches from above (moving down, vy=+3).
    {
        sf::FloatRect a({ 0.f, 90.f }, { 20.f, 20.f });   // bottom edge at 110
        sf::FloatRect b({ 0.f, 100.f }, { 20.f, 20.f });  // top edge at 100
        auto edgeOfB = rectangleRectangle(a, b);
        expect(edgeOfB && *edgeOfB == Edge::Top, "rect A-from-above hits B's Top edge");

        sf::Vector2f va{ 0.f, 3.f };
        auto va2 = bounceOffEdge(va, opposite(*edgeOfB));
        expect(va2.y < 0, "mover A bounces back upward (away from B)");
    }

    // Scenario 4: no overlap at all.
    {
        sf::FloatRect a({ 0.f, 0.f }, { 10.f, 10.f });
        sf::FloatRect b({ 100.f, 100.f }, { 10.f, 10.f });
        expect(!rectangleRectangle(a, b), "far-apart rectangles: no collision");
    }

    // Scenario 5: circle (the ball) moving right (vx=+5) into a rectangle (paddle) from the left.
    // Ball radius 10, centered at (95, 50); paddle spans x[100,120] y[30,70].
    {
        sf::Vector2f ballCenter{ 95.f, 50.f };
        float radius = 10.f;
        sf::FloatRect paddle({ 100.f, 30.f }, { 20.f, 40.f });
        auto edgeOfPaddle = circleRectangle(ballCenter, radius, paddle);
        expect(edgeOfPaddle && *edgeOfPaddle == Edge::Left, "ball-from-left hits paddle's Left edge");

        sf::Vector2f ballVel{ 5.f, 0.f };
        auto ballVel2 = bounceOffEdge(ballVel, opposite(*edgeOfPaddle));
        expect(ballVel2.x < 0, "ball bounces back leftward off the paddle (matches original pong behaviour)");
    }

    // Scenario 6: circle moving left (vx=-5) into a rectangle from the right (mirrors b-is-circle path
    // in Game::checkObjectCollision, which computes circleRectangle(circle, rect) then inverts for 'a').
    {
        sf::Vector2f ballCenter{ 125.f, 50.f };
        float radius = 10.f;
        sf::FloatRect paddle({ 100.f, 30.f }, { 20.f, 40.f });
        auto edgeOfPaddle = circleRectangle(ballCenter, radius, paddle);
        expect(edgeOfPaddle && *edgeOfPaddle == Edge::Right, "ball-from-right hits paddle's Right edge");

        // Simulate checkObjectCollision's b-is-circle branch: a=paddle, b=ball.
        // edgeOfA (from circleRectangle(ball, paddle)) is "edge of paddle" = Right.
        // edgeOfB (self-relative for ball) = opposite(Right) = Left.
        Edge edgeOfB = opposite(*edgeOfPaddle);
        expect(edgeOfB == Edge::Left, "ball's self-relative edge is Left when it hit paddle from the right");

        sf::Vector2f ballVel{ -5.f, 0.f };
        auto ballVel2 = bounceOffEdge(ballVel, edgeOfB);
        expect(ballVel2.x > 0, "ball bounces back rightward off the paddle");
    }

    std::printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
