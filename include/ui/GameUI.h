#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class GameUI {
public:
    GameUI();
    ~GameUI() = default;

    void update(float dt);
    void draw(sf::RenderWindow& window);

    void incrementDeaths();
    void resetDeaths();
    int getDeathCount() const { return deathCount; }

    void setTimer(float time);
    float getTimer() const { return timer; }

    void showVictoryMessage();
    void hideVictoryMessage();
    bool isVictoryMessageShown() const { return victoryMessageVisible; }

    // Health display
    void setHealth(int health, int maxHealth);

private:
    void updateTexts();
    void updateHearts();

private:
    sf::Font font;
    bool fontLoaded;

    // Death counter
    sf::Text deathText;
    int deathCount;

    // Timer
    sf::Text timerText;
    float timer;

    // Victory message
    sf::Text victoryText;
    sf::RectangleShape victoryBackground;
    bool victoryMessageVisible;

    // Health display
    int currentHealth;
    int maxHealthValue;
    std::vector<sf::RectangleShape> hearts;
};
