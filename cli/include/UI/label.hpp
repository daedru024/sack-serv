#pragma once
#include <SFML/Graphics.hpp>
#include "ui_element.hpp"

class Label {
public:
    sf::Text text;

    Label(sf::Font* font, std::string str, float x, float y, int size = 30, 
          sf::Color color = sf::Color::White, 
          sf::Color outlineColor = sf::Color::Black,
          float outline = 3.f) 
    {
        text.setFont(*font);
        text.setString(str);
        text.setCharacterSize(size);
        text.setFillColor(color);
        text.setOutlineColor(outlineColor);
        text.setOutlineThickness(outline);
        text.setPosition(x, y);
    }

    void set(std::string s) { text.setString(s); }
    void draw(sf::RenderWindow& win) { win.draw(text); }
};
