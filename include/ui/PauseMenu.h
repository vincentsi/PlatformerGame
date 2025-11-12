#pragma once

#include "ui/Menu.h"
#include <functional>

class PauseMenu : public Menu {
public:
    PauseMenu();

    void setCallbacks(
        std::function<void()> onResume,
        std::function<void()> onSettings,
        std::function<void()> onMainMenu,
        std::function<void()> onQuit
    );

    void handleInput(const sf::Event& event) override;
};
