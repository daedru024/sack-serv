#pragma once
#include <SFML/Graphics.hpp>
#include "ui_element.hpp"

class Button {
public:
    sf::RectangleShape shape;
    sf::Text text;

    Button() {}

    Button(sf::Font* font,
           const std::string& label,
           float x, float y,
           float w, float h)
    {
        shape.setSize({w, h});
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color(220, 220, 220));
        shape.setOutlineThickness(3);
        shape.setOutlineColor(sf::Color(80, 80, 80));

        text.setFont(*font);
        text.setString(label);
        text.setCharacterSize(32);
        text.setFillColor(sf::Color::Black);

        // 粗略置中
        sf::FloatRect tb = text.getLocalBounds();
        text.setPosition(
            x + (w - tb.width) / 2.f - tb.left,
            y + (h - tb.height) / 2.f - tb.top
        );
    }

    // hover 效果（要每 frame 呼叫）
    void update(sf::RenderWindow& window) {
        sf::Vector2i pixel = sf::Mouse::getPosition(window);
        sf::Vector2f world = window.mapPixelToCoords(pixel);

        if (shape.getGlobalBounds().contains(world)) {
            shape.setFillColor(sf::Color(235, 235, 235));
        } else {
            shape.setFillColor(sf::Color(220, 220, 220));
        }
    }

    // 檢查是否被點擊（針對這個 event）
    bool clicked(sf::Event& e, sf::RenderWindow& window) {
        if (e.type != sf::Event::MouseButtonReleased
            || e.mouseButton.button != sf::Mouse::Left)
            return false;

        sf::Vector2f world = window.mapPixelToCoords(
            { e.mouseButton.x, e.mouseButton.y }
        );
        return shape.getGlobalBounds().contains(world);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};
