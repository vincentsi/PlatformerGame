// Full-screen black overlay used to fade between levels / states.
#include "effects/ScreenTransition.h"

ScreenTransition::ScreenTransition()
    : state(State::None)
    , alpha(0.0f)
    , fadeSpeed(0.0f)
{
    overlay.setSize(sf::Vector2f(1280.0f, 720.0f));
    overlay.setFillColor(sf::Color(0, 0, 0, 0));
}

void ScreenTransition::startFadeOut(float duration) {
    state = State::FadingOut;
    alpha = 0.0f;
    fadeSpeed = 255.0f / duration;
}

void ScreenTransition::startFadeIn(float duration) {
    state = State::FadingIn;
    alpha = 255.0f;
    fadeSpeed = 255.0f / duration;
}

void ScreenTransition::update(float dt) {
    if (state == State::FadingOut) {
        alpha += fadeSpeed * dt;
        if (alpha >= 255.0f) {
            alpha = 255.0f;
        }
    }
    else if (state == State::FadingIn) {
        alpha -= fadeSpeed * dt;
        if (alpha <= 0.0f) {
            alpha = 0.0f;
            state = State::None;
        }
    }

    overlay.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));
}

void ScreenTransition::draw(sf::RenderWindow& window) {
    if (state != State::None) {
        // Always draw in screen space so the fade ignores the world camera.
        sf::View oldView = window.getView();
        window.setView(window.getDefaultView());
        window.draw(overlay);
        window.setView(oldView);
    }
}
