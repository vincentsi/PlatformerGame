#pragma once

#include "ui/Menu.h"
#include <functional>

class TitleScreen : public Menu {
public:
    TitleScreen();

    void setCallbacks(
        std::function<void()> onNewGame,
        std::function<void()> onContinue,
        std::function<void()> onSettings,
        std::function<void()> onQuit
    );

    void setCanContinue(bool canContinue);
};
