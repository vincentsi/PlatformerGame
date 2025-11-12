#pragma once

#include "ui/Menu.h"
#include "core/InputConfig.h"
#include <functional>

class KeyBindingMenu : public Menu {
public:
    KeyBindingMenu();

    void setCallbacks(std::function<void()> onBack);
    void handleInput(const sf::Event& event) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct BindingDisplay {
        std::string actionName;
        std::string action;
        sf::Text labelText;
        sf::Text valueText;
    };

    std::vector<BindingDisplay> bindings;
    bool isWaitingForKey;
    std::string currentAction;

    void updateBindingDisplays();
    void startRebinding(const std::string& action);
    void completeRebinding(sf::Keyboard::Key key);
    void cancelRebinding();
};
