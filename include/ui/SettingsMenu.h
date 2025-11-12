#pragma once

#include "ui/Menu.h"
#include "audio/AudioManager.h"
#include <functional>

class SettingsMenu : public Menu {
public:
    SettingsMenu(AudioManager* audioManager);

    void setCallbacks(std::function<void()> onControls, std::function<void()> onBack);
    void handleInput(const sf::Event& event) override;
    void draw(sf::RenderWindow& window) override;

private:
    AudioManager* audioManager;
    int masterVolume;
    int soundVolume;
    int musicVolume;

    sf::Text volumeText;
    sf::Text soundVolumeText;
    sf::Text musicVolumeText;

    void updateVolumeDisplay();
    void adjustMasterVolume(int delta);
    void adjustSoundVolume(int delta);
    void adjustMusicVolume(int delta);
};
