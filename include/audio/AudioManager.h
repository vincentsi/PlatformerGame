#pragma once

#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <memory>

class AudioManager {
public:
    AudioManager();
    ~AudioManager() = default;

    // Load audio files
    bool loadSound(const std::string& name, const std::string& filepath);
    bool loadMusic(const std::string& name, const std::string& filepath);

    // Play audio
    void playSound(const std::string& name, float volume = 100.0f);
    void playMusic(const std::string& name, bool loop = true, float volume = 50.0f);
    void stopMusic();

    // Volume control
    void setSoundVolume(float volume);  // 0-100
    void setMusicVolume(float volume);  // 0-100
    void setMasterVolume(float volume); // 0-100

    // Mute
    void muteSounds(bool mute);
    void muteMusic(bool mute);

private:
    std::map<std::string, sf::SoundBuffer> soundBuffers;
    std::map<std::string, std::unique_ptr<sf::Music>> musicTracks;
    std::vector<std::unique_ptr<sf::Sound>> activeSounds;

    float soundVolume;
    float musicVolume;
    float masterVolume;

    bool soundsMuted;
    bool musicMuted;
};
