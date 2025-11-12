#include "ui/KeyBindingMenu.h"
#include "core/Config.h"

KeyBindingMenu::KeyBindingMenu()
    : isWaitingForKey(false)
{
    setTitle("KEY BINDINGS");

    // Initialize binding displays
    std::vector<std::pair<std::string, std::string>> bindingList = {
        {"Move Left", "moveLeft"},
        {"Move Right", "moveRight"},
        {"Jump", "jump"},
        {"Menu Up", "menuUp"},
        {"Menu Down", "menuDown"},
        {"Menu Select", "menuSelect"}
    };

    for (const auto& [name, action] : bindingList) {
        BindingDisplay display;
        display.actionName = name;
        display.action = action;

        if (fontLoaded) {
            display.labelText.setFont(font);
            display.valueText.setFont(font);

            display.labelText.setCharacterSize(25);
            display.valueText.setCharacterSize(25);

            display.labelText.setFillColor(sf::Color::White);
            display.valueText.setFillColor(sf::Color(100, 200, 255));

            display.labelText.setString(name + ":");
        }

        bindings.push_back(display);
    }

    updateBindingDisplays();
}

void KeyBindingMenu::setCallbacks(std::function<void()> onBack) {
    items.clear();

    // Add binding items (invisible, will be covered by custom text)
    float startY = 180.0f;
    float spacing = 50.0f;
    float centerX = Config::WINDOW_WIDTH / 2.0f;
    float itemWidth = 400.0f;

    for (size_t i = 0; i < bindings.size(); i++) {
        addItem("", [this, i]() {
            startRebinding(bindings[i].action);
        });
    }

    // Add special options (these will use normal menu rendering)
    addItem("Reset to Defaults", [this]() {
        InputConfig::getInstance().resetToDefaults();
        updateBindingDisplays();
    });

    addItem("Back", onBack);

    // Custom layout for binding items
    for (size_t i = 0; i < bindings.size(); i++) {
        items[i].setPosition(centerX - itemWidth / 2.0f, startY + i * spacing);
    }

    // Position the Reset and Back buttons below bindings
    float buttonsY = startY + bindings.size() * spacing + 20.0f;
    items[bindings.size()].setPosition(centerX - itemWidth / 2.0f, buttonsY);
    items[bindings.size() + 1].setPosition(centerX - itemWidth / 2.0f, buttonsY + 70.0f);

    selectedIndex = 0;
}

void KeyBindingMenu::updateBindingDisplays() {
    InputConfig& config = InputConfig::getInstance();

    float centerX = Config::WINDOW_WIDTH / 2.0f;
    float labelX = centerX - 250.0f;
    float valueX = centerX + 50.0f;
    float startY = 180.0f;
    float spacing = 50.0f;

    for (size_t i = 0; i < bindings.size(); i++) {
        sf::Keyboard::Key key = config.getBinding(bindings[i].action);
        std::string keyName = config.getKeyName(key);

        bindings[i].valueText.setString("< " + keyName + " >");

        bindings[i].labelText.setPosition(labelX, startY + i * spacing);
        bindings[i].valueText.setPosition(valueX, startY + i * spacing);
    }
}

void KeyBindingMenu::startRebinding(const std::string& action) {
    if (isWaitingForKey) return;

    isWaitingForKey = true;
    currentAction = action;

    // Update display to show waiting state
    for (auto& binding : bindings) {
        if (binding.action == action) {
            binding.valueText.setString("< Press Key >");
            binding.valueText.setFillColor(sf::Color::Yellow);
            break;
        }
    }
}

void KeyBindingMenu::completeRebinding(sf::Keyboard::Key key) {
    if (!isWaitingForKey) return;

    InputConfig::getInstance().setBinding(currentAction, key);
    InputConfig::getInstance().saveToFile();

    isWaitingForKey = false;
    currentAction = "";

    updateBindingDisplays();

    // Restore color
    for (auto& binding : bindings) {
        binding.valueText.setFillColor(sf::Color(100, 200, 255));
    }
}

void KeyBindingMenu::cancelRebinding() {
    if (!isWaitingForKey) return;

    isWaitingForKey = false;
    currentAction = "";

    updateBindingDisplays();

    // Restore color
    for (auto& binding : bindings) {
        binding.valueText.setFillColor(sf::Color(100, 200, 255));
    }
}

void KeyBindingMenu::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (isWaitingForKey) {
            // Cancel rebinding on Escape
            if (event.key.code == sf::Keyboard::Escape) {
                cancelRebinding();
            } else {
                // Assign the new key
                completeRebinding(event.key.code);
            }
            return;
        }

        // Check for Escape to go back (only if not waiting for key)
        if (event.key.code == sf::Keyboard::Escape && !isWaitingForKey) {
            // Activate the last item (Back button)
            if (!items.empty()) {
                selectedIndex = static_cast<int>(items.size()) - 1;
                activate();
            }
            return;
        }

        // Normal menu navigation
        if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
            selectPrevious();
        }
        else if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
            selectNext();
        }
        else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
            activate();
        }
    }
}

void KeyBindingMenu::draw(sf::RenderWindow& window) {
    // Draw base menu
    Menu::draw(window);

    // Draw binding displays
    if (fontLoaded) {
        for (const auto& binding : bindings) {
            window.draw(binding.labelText);
            window.draw(binding.valueText);
        }
    }

    // Draw instruction text if waiting for key
    if (isWaitingForKey && fontLoaded) {
        sf::Text instructionText;
        instructionText.setFont(font);
        instructionText.setCharacterSize(20);
        instructionText.setFillColor(sf::Color::Yellow);
        instructionText.setString("Press any key to rebind, or ESC to cancel");

        sf::FloatRect bounds = instructionText.getLocalBounds();
        instructionText.setPosition(
            Config::WINDOW_WIDTH / 2.0f - bounds.width / 2.0f,
            Config::WINDOW_HEIGHT - 100.0f
        );

        window.draw(instructionText);
    }
}
