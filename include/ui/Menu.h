#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

class MenuItem {
public:
    MenuItem(const std::string& text, std::function<void()> callback);

    void draw(sf::RenderWindow& window, bool isSelected);
    void setPosition(float x, float y);
    sf::FloatRect getBounds() const;

    std::function<void()> callback;

private:
    sf::RectangleShape background;
    sf::Text text;
    sf::Font* font; // Will be set by Menu
    bool hasFont;

    friend class Menu;
};

class Menu {
public:
    Menu();
    virtual ~Menu() = default;

    void addItem(const std::string& text, std::function<void()> callback);
    virtual void handleInput(const sf::Event& event);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseClick(const sf::Vector2f& mousePos);
    void update(float dt);
    virtual void draw(sf::RenderWindow& window);

    void setTitle(const std::string& title);
    void selectNext();
    void selectPrevious();
    void activate();

protected:
    std::vector<MenuItem> items;
    int selectedIndex;
    sf::Font font;
    bool fontLoaded;

    sf::Text titleText;
    sf::RectangleShape background;

    void updateLayout();
};
