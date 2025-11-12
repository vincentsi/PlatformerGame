#include "ui/PauseMenu.h"

PauseMenu::PauseMenu() {
    setTitle("PAUSED");
}

void PauseMenu::setCallbacks(
    std::function<void()> onResume,
    std::function<void()> onSettings,
    std::function<void()> onMainMenu,
    std::function<void()> onQuit
) {
    items.clear();

    addItem("Resume", onResume);
    addItem("Settings", onSettings);
    addItem("Main Menu", onMainMenu);
    addItem("Quit", onQuit);

    selectedIndex = 0;
}

void PauseMenu::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        // Check for Escape to resume (first item)
        if (event.key.code == sf::Keyboard::Escape) {
            selectedIndex = 0;
            activate();
            return;
        }
    }

    // Call base class for normal navigation
    Menu::handleInput(event);
}
