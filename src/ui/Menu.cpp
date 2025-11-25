#include "ui/Menu.h"
#include "core/Config.h"
#include "core/InputConfig.h"
#include <iostream>

// MenuItem implementation
MenuItem::MenuItem(const std::string& text, std::function<void()> callback)
    : callback(callback)
    , font(nullptr)
    , hasFont(false)
{
    background.setSize(sf::Vector2f(400.0f, 60.0f));
    background.setFillColor(sf::Color(50, 50, 50, 200));
    background.setOutlineThickness(2.0f);
    background.setOutlineColor(sf::Color(100, 100, 100));

    this->text.setString(text);
    this->text.setCharacterSize(30);
    this->text.setFillColor(sf::Color::White);
}

void MenuItem::draw(sf::RenderWindow& window, bool isSelected) {
    if (isSelected) {
        background.setFillColor(sf::Color(100, 150, 200, 220));
        background.setOutlineColor(sf::Color(150, 200, 255));
    } else {
        background.setFillColor(sf::Color(50, 50, 50, 200));
        background.setOutlineColor(sf::Color(100, 100, 100));
    }

    window.draw(background);
    if (hasFont) {
        window.draw(text);
    }
}

void MenuItem::setPosition(float x, float y) {
    background.setPosition(x, y);

    if (hasFont) {
        // Center text in background
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            x + (background.getSize().x - textBounds.width) / 2.0f - textBounds.left,
            y + (background.getSize().y - textBounds.height) / 2.0f - textBounds.top
        );
    }
}

sf::FloatRect MenuItem::getBounds() const {
    return background.getGlobalBounds();
}

// Menu implementation
Menu::Menu()
    : selectedIndex(0)
    , fontLoaded(false)
{
    // Try to load font (optional)
    if (font.loadFromFile("assets/fonts/arial.ttf")) {
        fontLoaded = true;
        titleText.setFont(font);
    } else {
        std::cout << "Warning: Could not load menu font\n";
    }

    titleText.setCharacterSize(50);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(Config::WINDOW_WIDTH / 2.0f - 150.0f, 100.0f);

    // Semi-transparent dark background
    background.setSize(sf::Vector2f(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT));
    background.setFillColor(sf::Color(0, 0, 0, 180));
}

void Menu::addItem(const std::string& text, std::function<void()> callback) {
    MenuItem item(text, callback);

    if (fontLoaded) {
        item.font = &font;
        item.text.setFont(font);
        item.hasFont = true;
    }

    items.push_back(item);
    updateLayout();
}

void Menu::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        const InputBindings& bindings = InputConfig::getInstance().getBindings();

        if (event.key.code == sf::Keyboard::Up || event.key.code == bindings.menuUp) {
            selectPrevious();
        }
        else if (event.key.code == sf::Keyboard::Down || event.key.code == bindings.menuDown) {
            selectNext();
        }
        else if (event.key.code == bindings.menuSelect) {
            activate();
        }
    }
}

void Menu::handleMouseMove(const sf::Vector2f& mousePos) {
    // Check if mouse is hovering over any item
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].getBounds().contains(mousePos)) {
            selectedIndex = static_cast<int>(i);
            break;
        }
    }
}

void Menu::handleMouseClick(const sf::Vector2f& mousePos) {
    // Check if click is on any item
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].getBounds().contains(mousePos)) {
            selectedIndex = static_cast<int>(i);
            activate();
            break;
        }
    }
}

void Menu::update(float /*dt*/) {
    // Animation could go here (pulsing, etc.)
}

void Menu::draw(sf::RenderWindow& window) {
    window.draw(background);

    if (fontLoaded) {
        window.draw(titleText);
    }

    for (size_t i = 0; i < items.size(); i++) {
        items[i].draw(window, i == selectedIndex);
    }
}

void Menu::setTitle(const std::string& title) {
    titleText.setString(title);

    if (fontLoaded) {
        sf::FloatRect bounds = titleText.getLocalBounds();
        titleText.setPosition(
            Config::WINDOW_WIDTH / 2.0f - bounds.width / 2.0f,
            100.0f
        );
    }
}

void Menu::selectNext() {
    if (!items.empty()) {
        selectedIndex = (selectedIndex + 1) % items.size();
    }
}

void Menu::selectPrevious() {
    if (!items.empty()) {
        selectedIndex--;
        if (selectedIndex < 0) {
            selectedIndex = static_cast<int>(items.size()) - 1;
        }
    }
}

void Menu::activate() {
    if (selectedIndex >= 0 && selectedIndex < items.size()) {
        if (items[selectedIndex].callback) {
            items[selectedIndex].callback();
        }
    }
}

void Menu::updateLayout() {
    float startY = 250.0f;
    float spacing = 80.0f;
    float itemWidth = 400.0f;
    float startX = Config::WINDOW_WIDTH / 2.0f - itemWidth / 2.0f;

    for (size_t i = 0; i < items.size(); i++) {
        items[i].setPosition(startX, startY + i * spacing);
    }
}
