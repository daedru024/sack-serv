#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <functional>

// 顏色表
inline const std::array<sf::Color, 5> PLAYER_COLORS = {
    sf::Color(0,120,255),   // Blue
    sf::Color(255,60,60),   // Red
    sf::Color(0,200,0),     // Green
    sf::Color(255,220,0),   // Yellow
    sf::Color(255,140,0)    // Orange
};

class ColorSelector
{
public:
    int limit = 5;          // 可選顏色上限（由 maxPlayers 指定）
    int selected = -1;      // 自己選到哪個顏色

    std::vector<sf::RectangleShape> boxes;
    sf::RectangleShape preview;

    float baseX = 480;
    float baseY = 210;

    ColorSelector(float x, float y) : baseX(x), baseY(y)
    {
        preview.setSize({60, 60});
        preview.setFillColor(sf::Color(200,200,200));
        preview.setOutlineColor(sf::Color::Black);
        preview.setOutlineThickness(3);
        preview.setPosition(x, y);

        // 預設生成 5 個方塊
        boxes.resize(5);
        for (auto &b : boxes) {
            b.setSize({40, 40});
            b.setOutlineColor(sf::Color::Black);
            b.setOutlineThickness(3);
        }
    }

    void setLimit(int n) {
        limit = n;
        if (limit < 1) limit = 1;
        if (limit > 5) limit = 5;
    }

    // 自動排列位置，不會超出畫面
    void computePositions(float startX, float startY)
    {
        baseX = startX;
        baseY = startY;

        float boxSize  = 40.f;
        float spacing  = 8.f;

        float totalW = limit * boxSize + (limit - 1) * spacing;

        float x0 = startX - totalW / 2.f;   // 水平置中

        for (int i = 0; i < limit; i++)
        {
            float x = x0 + i * (boxSize + spacing);
            boxes[i].setSize({boxSize, boxSize});
            boxes[i].setFillColor(sf::Color::White);
            boxes[i].setPosition(x, baseY);
        }
    }

    // 點擊
    void updateClick(sf::Event &event, sf::RenderWindow &win,
        std::function<bool(int)> isTaken)
    {
        if (event.type != sf::Event::MouseButtonReleased) return;

        sf::Vector2f mp(event.mouseButton.x, event.mouseButton.y);
        mp = win.mapPixelToCoords((sf::Vector2i)mp);

        for (int i = 0; i < limit; i++)
        {
            if (boxes[i].getGlobalBounds().contains(mp))
            {
                if (!isTaken(i))
                    selected = i;
            }
        }
    }

    // 顯示所有可選顏色
    void draw(sf::RenderWindow &win,
              std::function<bool(int)> isTaken)
    {
        for (int i = 0; i < limit; i++)
        {
            if (isTaken(i) && i != selected) continue;

            boxes[i].setFillColor(PLAYER_COLORS[i]);
            win.draw(boxes[i]);
        }
    }
};

