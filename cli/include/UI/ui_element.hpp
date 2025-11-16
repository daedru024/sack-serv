#pragma once
#include <SFML/Graphics.hpp>

const float UI_WIDTH  = 800.f;
const float UI_HEIGHT = 600.f;

enum class Anchor {
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    CENTER
};

struct UIElement {
    Anchor anchor;
    sf::Vector2f offset;

    UIElement(Anchor a, sf::Vector2f o)
        : anchor(a), offset(o) {}

    sf::Vector2f resolve() const {
        switch (anchor) {
            case Anchor::TOP_LEFT:     return {offset.x, offset.y};
            case Anchor::TOP_RIGHT:    return {UI_WIDTH - offset.x, offset.y};
            case Anchor::BOTTOM_LEFT:  return {offset.x, UI_HEIGHT - offset.y};
            case Anchor::BOTTOM_RIGHT: return {UI_WIDTH - offset.x, UI_HEIGHT - offset.y};
            case Anchor::CENTER:       return {UI_WIDTH/2.f + offset.x, UI_HEIGHT/2.f + offset.y};
        }
        return offset;
    }
};
