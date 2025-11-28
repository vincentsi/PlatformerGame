#pragma once

#include "entities/Enemy.h"
#include <SFML/Graphics.hpp>

// Presets de stats pour différents types d'ennemis
namespace EnemyPresets {
    // Ennemi basique (faible HP, pas de tir)
    inline EnemyStats Basic() {
        EnemyStats stats;
        stats.maxHP = 1;
        stats.sizeX = 30.0f;
        stats.sizeY = 30.0f;
        stats.speed = 80.0f;
        stats.damage = 1;
        stats.color = sf::Color::Red;
        stats.canShoot = false;
        return stats;
    }
    
    // Ennemi moyen (2 HP, pas de tir)
    inline EnemyStats Medium() {
        EnemyStats stats;
        stats.maxHP = 2;
        stats.sizeX = 35.0f;
        stats.sizeY = 35.0f;
        stats.speed = 100.0f;
        stats.damage = 1;
        stats.color = sf::Color(255, 150, 0); // Orange
        stats.canShoot = false;
        return stats;
    }
    
    // Ennemi fort (3 HP, pas de tir)
    inline EnemyStats Strong() {
        EnemyStats stats;
        stats.maxHP = 3;
        stats.sizeX = 40.0f;
        stats.sizeY = 40.0f;
        stats.speed = 120.0f;
        stats.damage = 1;
        stats.color = sf::Color(200, 0, 200); // Magenta
        stats.canShoot = false;
        return stats;
    }
    
    // Tireur basique (1 HP, tire des projectiles)
    inline EnemyStats Shooter() {
        EnemyStats stats;
        stats.maxHP = 1;
        stats.sizeX = 32.0f;
        stats.sizeY = 32.0f;
        stats.speed = 60.0f;
        stats.damage = 1;
        stats.color = sf::Color(255, 200, 0); // Jaune
        stats.canShoot = true;
        stats.shootCooldown = 2.0f;
        stats.projectileSpeed = 300.0f;
        stats.projectileRange = 500.0f;
        stats.shootRange = 400.0f;
        return stats;
    }
    
    // Tireur rapide (1 HP, tire souvent)
    inline EnemyStats FastShooter() {
        EnemyStats stats;
        stats.maxHP = 1;
        stats.sizeX = 28.0f;
        stats.sizeY = 28.0f;
        stats.speed = 90.0f;
        stats.damage = 1;
        stats.color = sf::Color(255, 100, 255); // Rose
        stats.canShoot = true;
        stats.shootCooldown = 1.0f; // Plus rapide
        stats.projectileSpeed = 400.0f;
        stats.projectileRange = 600.0f;
        stats.shootRange = 500.0f;
        return stats;
    }
    
    // Boss (beaucoup de HP, tire des projectiles)
    inline EnemyStats Boss() {
        EnemyStats stats;
        stats.maxHP = 10;
        stats.sizeX = 60.0f;
        stats.sizeY = 60.0f;
        stats.speed = 50.0f;
        stats.damage = 2;
        stats.color = sf::Color(150, 0, 150); // Violet foncé
        stats.canShoot = true;
        stats.shootCooldown = 1.5f;
        stats.projectileSpeed = 350.0f;
        stats.projectileRange = 700.0f;
        stats.shootRange = 600.0f;
        return stats;
    }
    
    // Petit ennemi rapide (1 HP, très rapide, pas de tir)
    inline EnemyStats Fast() {
        EnemyStats stats;
        stats.maxHP = 1;
        stats.sizeX = 25.0f;
        stats.sizeY = 25.0f;
        stats.speed = 150.0f;
        stats.damage = 1;
        stats.color = sf::Color(0, 255, 255); // Cyan
        stats.canShoot = false;
        return stats;
    }
    
    // Ennemi volant basique (pour FlyingEnemy)
    inline EnemyStats FlyingBasic() {
        EnemyStats stats;
        stats.maxHP = 1;
        stats.sizeX = 28.0f;
        stats.sizeY = 28.0f;
        stats.speed = 60.0f;
        stats.damage = 1;
        stats.color = sf::Color(150, 0, 255); // Violet (déjà utilisé par FlyingEnemy)
        stats.canShoot = false;
        return stats;
    }
    
    // Ennemi volant tireur
    inline EnemyStats FlyingShooter() {
        EnemyStats stats;
        stats.maxHP = 2;
        stats.sizeX = 30.0f;
        stats.sizeY = 30.0f;
        stats.speed = 70.0f;
        stats.damage = 1;
        stats.color = sf::Color(200, 100, 255); // Violet clair
        stats.canShoot = true;
        stats.shootCooldown = 2.5f;
        stats.projectileSpeed = 280.0f;
        stats.projectileRange = 450.0f;
        stats.shootRange = 350.0f;
        return stats;
    }
}

