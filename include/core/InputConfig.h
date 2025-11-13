#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <string>

struct InputBindings {
    sf::Keyboard::Key moveLeft;
    sf::Keyboard::Key moveRight;
    sf::Keyboard::Key jump;
    sf::Keyboard::Key ability;  // Special ability key
    sf::Keyboard::Key menuUp;
    sf::Keyboard::Key menuDown;
    sf::Keyboard::Key menuSelect;
    sf::Keyboard::Key pause;

    InputBindings()
        : moveLeft(sf::Keyboard::A)
        , moveRight(sf::Keyboard::D)
        , jump(sf::Keyboard::Space)
        , ability(sf::Keyboard::LShift)  // Default: Left Shift
        , menuUp(sf::Keyboard::W)
        , menuDown(sf::Keyboard::S)
        , menuSelect(sf::Keyboard::Enter)
        , pause(sf::Keyboard::Escape)
    {}
};

class InputConfig {
public:
    static InputConfig& getInstance();

    InputBindings& getBindings() { return bindings; }
    const InputBindings& getBindings() const { return bindings; }

    void setBinding(const std::string& action, sf::Keyboard::Key key);
    sf::Keyboard::Key getBinding(const std::string& action) const;
    std::string getKeyName(sf::Keyboard::Key key) const;

    void resetToDefaults();
    bool saveToFile(const std::string& filename = "keybindings.cfg");
    bool loadFromFile(const std::string& filename = "keybindings.cfg");

private:
    InputConfig() = default;
    InputConfig(const InputConfig&) = delete;
    InputConfig& operator=(const InputConfig&) = delete;

    InputBindings bindings;

    static std::string getSavePath(const std::string& filename);
};
