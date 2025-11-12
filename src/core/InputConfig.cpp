#include "core/InputConfig.h"
#include <fstream>
#include <iostream>
#include <map>

InputConfig& InputConfig::getInstance() {
    static InputConfig instance;
    return instance;
}

void InputConfig::setBinding(const std::string& action, sf::Keyboard::Key key) {
    if (action == "moveLeft") bindings.moveLeft = key;
    else if (action == "moveRight") bindings.moveRight = key;
    else if (action == "jump") bindings.jump = key;
    else if (action == "menuUp") bindings.menuUp = key;
    else if (action == "menuDown") bindings.menuDown = key;
    else if (action == "menuSelect") bindings.menuSelect = key;
    else if (action == "pause") bindings.pause = key;
}

sf::Keyboard::Key InputConfig::getBinding(const std::string& action) const {
    if (action == "moveLeft") return bindings.moveLeft;
    else if (action == "moveRight") return bindings.moveRight;
    else if (action == "jump") return bindings.jump;
    else if (action == "menuUp") return bindings.menuUp;
    else if (action == "menuDown") return bindings.menuDown;
    else if (action == "menuSelect") return bindings.menuSelect;
    else if (action == "pause") return bindings.pause;
    return sf::Keyboard::Unknown;
}

std::string InputConfig::getKeyName(sf::Keyboard::Key key) const {
    static const std::map<sf::Keyboard::Key, std::string> keyNames = {
        {sf::Keyboard::A, "A"}, {sf::Keyboard::B, "B"}, {sf::Keyboard::C, "C"},
        {sf::Keyboard::D, "D"}, {sf::Keyboard::E, "E"}, {sf::Keyboard::F, "F"},
        {sf::Keyboard::G, "G"}, {sf::Keyboard::H, "H"}, {sf::Keyboard::I, "I"},
        {sf::Keyboard::J, "J"}, {sf::Keyboard::K, "K"}, {sf::Keyboard::L, "L"},
        {sf::Keyboard::M, "M"}, {sf::Keyboard::N, "N"}, {sf::Keyboard::O, "O"},
        {sf::Keyboard::P, "P"}, {sf::Keyboard::Q, "Q"}, {sf::Keyboard::R, "R"},
        {sf::Keyboard::S, "S"}, {sf::Keyboard::T, "T"}, {sf::Keyboard::U, "U"},
        {sf::Keyboard::V, "V"}, {sf::Keyboard::W, "W"}, {sf::Keyboard::X, "X"},
        {sf::Keyboard::Y, "Y"}, {sf::Keyboard::Z, "Z"},
        {sf::Keyboard::Num0, "0"}, {sf::Keyboard::Num1, "1"}, {sf::Keyboard::Num2, "2"},
        {sf::Keyboard::Num3, "3"}, {sf::Keyboard::Num4, "4"}, {sf::Keyboard::Num5, "5"},
        {sf::Keyboard::Num6, "6"}, {sf::Keyboard::Num7, "7"}, {sf::Keyboard::Num8, "8"},
        {sf::Keyboard::Num9, "9"},
        {sf::Keyboard::Escape, "ESC"}, {sf::Keyboard::LControl, "LCtrl"},
        {sf::Keyboard::LShift, "LShift"}, {sf::Keyboard::LAlt, "LAlt"},
        {sf::Keyboard::LSystem, "LSystem"}, {sf::Keyboard::RControl, "RCtrl"},
        {sf::Keyboard::RShift, "RShift"}, {sf::Keyboard::RAlt, "RAlt"},
        {sf::Keyboard::RSystem, "RSystem"}, {sf::Keyboard::Menu, "Menu"},
        {sf::Keyboard::LBracket, "["}, {sf::Keyboard::RBracket, "]"},
        {sf::Keyboard::Semicolon, ";"}, {sf::Keyboard::Comma, ","},
        {sf::Keyboard::Period, "."}, {sf::Keyboard::Quote, "'"},
        {sf::Keyboard::Slash, "/"}, {sf::Keyboard::Backslash, "\\"},
        {sf::Keyboard::Tilde, "~"}, {sf::Keyboard::Equal, "="},
        {sf::Keyboard::Hyphen, "-"}, {sf::Keyboard::Space, "Space"},
        {sf::Keyboard::Enter, "Enter"}, {sf::Keyboard::Backspace, "Backspace"},
        {sf::Keyboard::Tab, "Tab"}, {sf::Keyboard::PageUp, "PageUp"},
        {sf::Keyboard::PageDown, "PageDown"}, {sf::Keyboard::End, "End"},
        {sf::Keyboard::Home, "Home"}, {sf::Keyboard::Insert, "Insert"},
        {sf::Keyboard::Delete, "Delete"}, {sf::Keyboard::Add, "+"},
        {sf::Keyboard::Subtract, "-"}, {sf::Keyboard::Multiply, "*"},
        {sf::Keyboard::Divide, "/"}, {sf::Keyboard::Left, "Left"},
        {sf::Keyboard::Right, "Right"}, {sf::Keyboard::Up, "Up"},
        {sf::Keyboard::Down, "Down"}, {sf::Keyboard::Numpad0, "Num0"},
        {sf::Keyboard::Numpad1, "Num1"}, {sf::Keyboard::Numpad2, "Num2"},
        {sf::Keyboard::Numpad3, "Num3"}, {sf::Keyboard::Numpad4, "Num4"},
        {sf::Keyboard::Numpad5, "Num5"}, {sf::Keyboard::Numpad6, "Num6"},
        {sf::Keyboard::Numpad7, "Num7"}, {sf::Keyboard::Numpad8, "Num8"},
        {sf::Keyboard::Numpad9, "Num9"}, {sf::Keyboard::F1, "F1"},
        {sf::Keyboard::F2, "F2"}, {sf::Keyboard::F3, "F3"}, {sf::Keyboard::F4, "F4"},
        {sf::Keyboard::F5, "F5"}, {sf::Keyboard::F6, "F6"}, {sf::Keyboard::F7, "F7"},
        {sf::Keyboard::F8, "F8"}, {sf::Keyboard::F9, "F9"}, {sf::Keyboard::F10, "F10"},
        {sf::Keyboard::F11, "F11"}, {sf::Keyboard::F12, "F12"},
        {sf::Keyboard::Pause, "Pause"}
    };

    auto it = keyNames.find(key);
    if (it != keyNames.end()) {
        return it->second;
    }
    return "Unknown";
}

void InputConfig::resetToDefaults() {
    bindings = InputBindings();
}

bool InputConfig::saveToFile(const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cout << "Warning: Could not save input config to " << path << "\n";
        return false;
    }

    file << static_cast<int>(bindings.moveLeft) << "\n";
    file << static_cast<int>(bindings.moveRight) << "\n";
    file << static_cast<int>(bindings.jump) << "\n";
    file << static_cast<int>(bindings.menuUp) << "\n";
    file << static_cast<int>(bindings.menuDown) << "\n";
    file << static_cast<int>(bindings.menuSelect) << "\n";
    file << static_cast<int>(bindings.pause) << "\n";

    file.close();
    std::cout << "Input config saved successfully\n";
    return true;
}

bool InputConfig::loadFromFile(const std::string& filename) {
    std::string path = getSavePath(filename);
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "No saved input config found, using defaults\n";
        return false;
    }

    // Helper function to validate key code
    auto isValidKey = [](int keyCode) -> bool {
        // SFML key codes range from 0 to ~100 (Keyboard::KeyCount)
        // Check if key is within valid range
        return keyCode >= 0 && keyCode < static_cast<int>(sf::Keyboard::KeyCount);
    };

    int key;
    bool valid = true;

    // Read and validate each key binding
    if (file >> key && isValidKey(key)) {
        bindings.moveLeft = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid moveLeft key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.moveRight = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid moveRight key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.jump = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid jump key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.menuUp = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid menuUp key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.menuDown = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid menuDown key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.menuSelect = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid menuSelect key in config file, using default\n";
    }

    if (file >> key && isValidKey(key)) {
        bindings.pause = static_cast<sf::Keyboard::Key>(key);
    } else {
        valid = false;
        std::cout << "Warning: Invalid pause key in config file, using default\n";
    }

    file.close();

    // If any key was invalid, reset to defaults for safety
    if (!valid) {
        std::cout << "Config file contained invalid keys, resetting to defaults\n";
        resetToDefaults();
        return false;
    }

    std::cout << "Input config loaded successfully\n";
    return true;
}

std::string InputConfig::getSavePath(const std::string& filename) {
    return filename;
}
