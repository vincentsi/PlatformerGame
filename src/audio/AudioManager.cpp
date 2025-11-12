#include "audio/AudioManager.h"
#include <iostream>
#include <algorithm>
#include <set>

AudioManager::AudioManager()
    : soundVolume(100.0f)
    , musicVolume(50.0f)
    , masterVolume(100.0f)
    , soundsMuted(false)
    , musicMuted(false)
{
    activeSounds.reserve(20); // Pre-allocate for performance
}

bool AudioManager::loadSound(const std::string& name, const std::string& filepath) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filepath)) {
        std::cout << "Warning: Could not load sound: " << filepath << "\n";
        return false;
    }
    soundBuffers[name] = buffer;
    return true;
}

bool AudioManager::loadMusic(const std::string& name, const std::string& filepath) {
    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(filepath)) {
        std::cout << "Warning: Could not load music: " << filepath << "\n";
        return false;
    }
    musicTracks[name] = std::move(music);
    return true;
}

void AudioManager::playSound(const std::string& name, float volume) {
    if (soundsMuted) return;

    auto it = soundBuffers.find(name);
    if (it == soundBuffers.end()) {
        // Sound not loaded - silently ignore (audio is optional)
        // Only log once per missing sound to avoid spam
        static std::set<std::string> warnedSounds;
        if (warnedSounds.find(name) == warnedSounds.end()) {
            warnedSounds.insert(name);
            // Don't spam console - audio is optional
        }
        return;
    }

    // Clean up finished sounds
    activeSounds.erase(
        std::remove_if(activeSounds.begin(), activeSounds.end(),
            [](const std::unique_ptr<sf::Sound>& sound) {
                return sound->getStatus() == sf::Sound::Stopped;
            }),
        activeSounds.end()
    );

    // Create and play new sound
    auto sound = std::make_unique<sf::Sound>();
    sound->setBuffer(it->second);
    float finalVolume = (volume / 100.0f) * (soundVolume / 100.0f) * (masterVolume / 100.0f) * 100.0f;
    sound->setVolume(finalVolume);
    sound->play();

    activeSounds.push_back(std::move(sound));
}

void AudioManager::playMusic(const std::string& name, bool loop, float volume) {
    if (musicMuted) return;

    auto it = musicTracks.find(name);
    if (it == musicTracks.end()) {
        std::cout << "Warning: Music not found: " << name << "\n";
        return;
    }

    // Stop any currently playing music
    for (auto& [musicName, music] : musicTracks) {
        if (music->getStatus() == sf::Music::Playing) {
            music->stop();
        }
    }

    float finalVolume = (volume / 100.0f) * (musicVolume / 100.0f) * (masterVolume / 100.0f) * 100.0f;
    it->second->setVolume(finalVolume);
    it->second->setLoop(loop);
    it->second->play();
}

void AudioManager::stopMusic() {
    for (auto& [name, music] : musicTracks) {
        if (music->getStatus() == sf::Music::Playing) {
            music->stop();
        }
    }
}

void AudioManager::setSoundVolume(float volume) {
    soundVolume = std::max(0.0f, std::min(100.0f, volume));
}

void AudioManager::setMusicVolume(float volume) {
    musicVolume = std::max(0.0f, std::min(100.0f, volume));

    // Update currently playing music
    for (auto& [name, music] : musicTracks) {
        if (music->getStatus() == sf::Music::Playing) {
            float finalVolume = (musicVolume / 100.0f) * (masterVolume / 100.0f) * 100.0f;
            music->setVolume(finalVolume);
        }
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = std::max(0.0f, std::min(100.0f, volume));

    // Update all active sounds and music
    for (auto& sound : activeSounds) {
        float finalVolume = (soundVolume / 100.0f) * (masterVolume / 100.0f) * 100.0f;
        sound->setVolume(finalVolume);
    }

    for (auto& [name, music] : musicTracks) {
        if (music->getStatus() == sf::Music::Playing) {
            float finalVolume = (musicVolume / 100.0f) * (masterVolume / 100.0f) * 100.0f;
            music->setVolume(finalVolume);
        }
    }
}

void AudioManager::muteSounds(bool mute) {
    soundsMuted = mute;
    if (mute) {
        for (auto& sound : activeSounds) {
            sound->stop();
        }
    }
}

void AudioManager::muteMusic(bool mute) {
    musicMuted = mute;
    if (mute) {
        for (auto& [name, music] : musicTracks) {
            if (music->getStatus() == sf::Music::Playing) {
                music->pause();
            }
        }
    } else {
        for (auto& [name, music] : musicTracks) {
            if (music->getStatus() == sf::Music::Paused) {
                music->play();
            }
        }
    }
}
