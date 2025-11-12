#include "ui/SettingsMenu.h"
#include "core/Config.h"

SettingsMenu::SettingsMenu(AudioManager* audioManager)
    : audioManager(audioManager)
    , masterVolume(70)
    , soundVolume(80)
    , musicVolume(50)
{
    setTitle("SETTINGS");

    if (fontLoaded) {
        volumeText.setFont(font);
        soundVolumeText.setFont(font);
        musicVolumeText.setFont(font);

        volumeText.setCharacterSize(30);
        soundVolumeText.setCharacterSize(30);
        musicVolumeText.setCharacterSize(30);

        volumeText.setFillColor(sf::Color::White);
        soundVolumeText.setFillColor(sf::Color::White);
        musicVolumeText.setFillColor(sf::Color::White);
    }

    updateVolumeDisplay();
}

void SettingsMenu::setCallbacks(std::function<void()> onControls, std::function<void()> onBack) {
    items.clear();

    addItem("", [this]() { /* Master volume handled by custom input */ });
    addItem("", [this]() { /* Sound volume handled by custom input */ });
    addItem("", [this]() { /* Music volume handled by custom input */ });
    addItem("Controls", onControls);
    addItem("Back", onBack);

    selectedIndex = 0;
    updateVolumeDisplay();
}

void SettingsMenu::updateVolumeDisplay() {
    volumeText.setString("Master Volume: < " + std::to_string(masterVolume) + " >");
    soundVolumeText.setString("Sound Volume:  < " + std::to_string(soundVolume) + " >");
    musicVolumeText.setString("Music Volume:  < " + std::to_string(musicVolume) + " >");

    // Position texts to match menu items layout
    float startY = 250.0f;
    float spacing = 80.0f;
    float centerX = Config::WINDOW_WIDTH / 2.0f;

    volumeText.setPosition(centerX - volumeText.getLocalBounds().width / 2.0f, startY);
    soundVolumeText.setPosition(centerX - soundVolumeText.getLocalBounds().width / 2.0f, startY + spacing);
    musicVolumeText.setPosition(centerX - musicVolumeText.getLocalBounds().width / 2.0f, startY + spacing * 2);

    if (audioManager) {
        audioManager->setMasterVolume(static_cast<float>(masterVolume));
        audioManager->setSoundVolume(static_cast<float>(soundVolume));
        audioManager->setMusicVolume(static_cast<float>(musicVolume));
    }
}

void SettingsMenu::adjustMasterVolume(int delta) {
    masterVolume = std::clamp(masterVolume + delta, 0, 100);
    updateVolumeDisplay();
}

void SettingsMenu::adjustSoundVolume(int delta) {
    soundVolume = std::clamp(soundVolume + delta, 0, 100);
    updateVolumeDisplay();
}

void SettingsMenu::adjustMusicVolume(int delta) {
    musicVolume = std::clamp(musicVolume + delta, 0, 100);
    updateVolumeDisplay();
}

void SettingsMenu::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        // Check for Escape to go back
        if (event.key.code == sf::Keyboard::Escape) {
            // Activate the last item (Back button)
            if (!items.empty()) {
                selectedIndex = static_cast<int>(items.size()) - 1;
                activate();
            }
            return;
        }

        // Handle left/right for volume adjustments
        if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
            if (selectedIndex == 0) adjustMasterVolume(-5);
            else if (selectedIndex == 1) adjustSoundVolume(-5);
            else if (selectedIndex == 2) adjustMusicVolume(-5);
        }
        else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
            if (selectedIndex == 0) adjustMasterVolume(5);
            else if (selectedIndex == 1) adjustSoundVolume(5);
            else if (selectedIndex == 2) adjustMusicVolume(5);
        }
    }

    // Call base class for normal navigation
    Menu::handleInput(event);
}

void SettingsMenu::draw(sf::RenderWindow& window) {
    // Draw base menu
    Menu::draw(window);

    // Override item text with volume displays
    if (fontLoaded) {
        window.draw(volumeText);
        window.draw(soundVolumeText);
        window.draw(musicVolumeText);
    }
}
