#include "ui/GameUI.h"
#include <sstream>
#include <iomanip>
#include <iostream>

GameUI::GameUI()
    : fontLoaded(false)
    , deathCount(0)
    , timer(0.0f)
    , victoryMessageVisible(false)
    , currentHealth(3)
    , maxHealthValue(3)
{
    // Try to load font (optional - will work without it)
    if (!font.loadFromFile("assets/fonts/arial.ttf")) {
        std::cout << "Warning: Could not load font for UI. Using default font.\n";
        fontLoaded = false;
    } else {
        fontLoaded = true;
    }

    // Setup death counter text
    deathText.setCharacterSize(24);
    deathText.setFillColor(sf::Color::White);
    deathText.setOutlineColor(sf::Color::Black);
    deathText.setOutlineThickness(2.0f);
    deathText.setPosition(10.0f, 10.0f);
    if (fontLoaded) {
        deathText.setFont(font);
    }

    // Setup timer text
    timerText.setCharacterSize(24);
    timerText.setFillColor(sf::Color::White);
    timerText.setOutlineColor(sf::Color::Black);
    timerText.setOutlineThickness(2.0f);
    timerText.setPosition(10.0f, 40.0f);
    if (fontLoaded) {
        timerText.setFont(font);
    }

    // Setup victory message (centered, large text)
    victoryText.setCharacterSize(64);
    victoryText.setFillColor(sf::Color::Yellow);
    victoryText.setOutlineColor(sf::Color::Black);
    victoryText.setOutlineThickness(3.0f);
    victoryText.setString("LEVEL COMPLETE!");
    if (fontLoaded) {
        victoryText.setFont(font);
        // Center the text (will be adjusted in showVictoryMessage)
    }

    // Setup semi-transparent background for victory message
    victoryBackground.setSize(sf::Vector2f(600.0f, 200.0f));
    victoryBackground.setFillColor(sf::Color(0, 0, 0, 200));
    victoryBackground.setOutlineColor(sf::Color::Yellow);
    victoryBackground.setOutlineThickness(5.0f);

    // Initialize hearts
    updateHearts();

    updateTexts();
}

void GameUI::update(float dt) {
    timer += dt;
    updateTexts();
}

void GameUI::draw(sf::RenderWindow& window) {
    if (fontLoaded) {
        window.draw(deathText);
        window.draw(timerText);

        // Draw hearts
        for (const auto& heart : hearts) {
            window.draw(heart);
        }

        // Draw victory message if visible
        if (victoryMessageVisible) {
            window.draw(victoryBackground);
            window.draw(victoryText);
        }
    }
}

void GameUI::incrementDeaths() {
    deathCount++;
    updateTexts();
}

void GameUI::resetDeaths() {
    deathCount = 0;
    timer = 0.0f;
    updateTexts();
}

void GameUI::setTimer(float time) {
    timer = time;
    updateTexts();
}

void GameUI::updateTexts() {
    // Update death counter
    deathText.setString("Deaths: " + std::to_string(deathCount));

    // Update timer (format: MM:SS.mm)
    int minutes = static_cast<int>(timer) / 60;
    int seconds = static_cast<int>(timer) % 60;
    int milliseconds = static_cast<int>((timer - static_cast<int>(timer)) * 100);

    std::ostringstream oss;
    oss << "Time: "
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds << "."
        << std::setfill('0') << std::setw(2) << milliseconds;
    timerText.setString(oss.str());
}

void GameUI::showVictoryMessage() {
    victoryMessageVisible = true;

    // Center the victory message on screen (assuming 1280x720 window)
    sf::FloatRect textBounds = victoryText.getLocalBounds();
    victoryText.setOrigin(textBounds.width / 2.0f, textBounds.height / 2.0f);
    victoryText.setPosition(640.0f, 360.0f);

    // Center the background
    victoryBackground.setOrigin(victoryBackground.getSize().x / 2.0f, victoryBackground.getSize().y / 2.0f);
    victoryBackground.setPosition(640.0f, 360.0f);
}

void GameUI::hideVictoryMessage() {
    victoryMessageVisible = false;
}

void GameUI::setHealth(int health, int maxHealth) {
    currentHealth = health;
    maxHealthValue = maxHealth;
    updateHearts();
}

void GameUI::updateHearts() {
    hearts.clear();

    float heartSize = 30.0f;
    float heartSpacing = 40.0f;
    float startX = 10.0f;
    float startY = 70.0f; // Below the timer

    for (int i = 0; i < maxHealthValue; ++i) {
        sf::RectangleShape heart;
        heart.setSize(sf::Vector2f(heartSize, heartSize));
        heart.setPosition(startX + i * heartSpacing, startY);
        heart.setOutlineColor(sf::Color::Black);
        heart.setOutlineThickness(2.0f);

        // Full heart if still has health, empty heart otherwise
        if (i < currentHealth) {
            heart.setFillColor(sf::Color::Red); // Full heart
        } else {
            heart.setFillColor(sf::Color(100, 100, 100)); // Empty heart (gray)
        }

        hearts.push_back(heart);
    }
}
