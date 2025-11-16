#pragma once
#include "ui_element.hpp"
#include <SFML/Graphics.hpp>
#include <string>

class TextBox {
public:
    sf::RectangleShape box;
    sf::Text text;
    std::string buffer;
    bool focused = false;

    TextBox(sf::Font* font, float x, float y, float w = 300, float h = 50) {
        box.setSize({w, h});
        box.setPosition(x, y);
        box.setFillColor(sf::Color::White);
        box.setOutlineThickness(2);
        box.setOutlineColor(sf::Color::Black);

        text.setFont(*font);
        text.setCharacterSize(28);
        text.setFillColor(sf::Color::Black);
        text.setPosition(x + 10, y + 8);
    }

    void handleEvent(sf::Event& e, sf::RenderWindow& win) {
        // 滑鼠點擊 → 決定是否 focus
        if (e.type == sf::Event::MouseButtonPressed &&
            e.mouseButton.button == sf::Mouse::Left) {

            sf::Vector2f world = win.mapPixelToCoords(
                { e.mouseButton.x, e.mouseButton.y }
            );

            if (box.getGlobalBounds().contains(world)) {
                focused = true;
                box.setOutlineColor(sf::Color(0, 120, 255));
            } else {
                focused = false;
                box.setOutlineColor(sf::Color::Black);
            }
        }

        // 輸入文字
        if (focused && e.type == sf::Event::TextEntered) {
            if (e.text.unicode == 8) { // Backspace
                if (!buffer.empty()) buffer.pop_back();
            } else if (e.text.unicode >= 32 && e.text.unicode < 128) {
                buffer.push_back(static_cast<char>(e.text.unicode));
            }
            text.setString(buffer);
        }
    }

    void draw(sf::RenderWindow& win) {
        win.draw(box);
        win.draw(text);
    }
};
