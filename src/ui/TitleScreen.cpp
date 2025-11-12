#include "ui/TitleScreen.h"

TitleScreen::TitleScreen() {
    setTitle("PLATFORMER GAME");
}

void TitleScreen::setCallbacks(
    std::function<void()> onNewGame,
    std::function<void()> onContinue,
    std::function<void()> onSettings,
    std::function<void()> onQuit
) {
    items.clear();

    addItem("New Game", onNewGame);
    addItem("Continue", onContinue);
    addItem("Settings", onSettings);
    addItem("Quit", onQuit);

    selectedIndex = 0;
}

void TitleScreen::setCanContinue(bool canContinue) {
    // If there's no save, gray out or disable continue option
    // For now, we'll just keep it enabled but you could add visual feedback
}
