#pragma once
#include<iostream>
#include<vector>
#include<ctime>
#include<sstream>


#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>


/*
    Class that acts as game Engine
    wrapper class .
*/

class Game
{
    private:
    static constexpr unsigned windowWidth = 800;
    static constexpr unsigned windowHeight = 600;

    bool headless;
    bool agentControlled;

    // Variable
    // Window
    sf::RenderWindow* window;
   
    // Mouse position
    sf::Vector2i mousePosWindow;
    sf::Vector2f mousePosview;

    // Resources
    sf::Font font;

    // UI text
    sf::Text uiText{font};
    sf::Text gameOverText{font};

    // game logic
   
    unsigned points;
    bool endGame;
    int health;
    float enemySpawntimer;
    float enmyspawnTimeMax;
    int maxenemy;
    bool mouseHel;
    float fireTime;
    float fireMaxTime;
    


    // Game Object
    std::vector<sf::RectangleShape> enemies;
    sf::RectangleShape enemy;
    sf::RectangleShape ship;
    std::vector<sf::RectangleShape> bullets;
    sf::RectangleShape bullet;

    // private function
    void initVariable();
    void initwindow();
    void initship();
    void initbullet();
    void initenemy();
    void initfount();
    void initText();
    void moveShip(int direction);
    

    public:
        // constructor
        explicit Game(bool headless = false, bool agentControlled = false);
        virtual ~Game();
        // Accessor
        const bool running() const;
        const bool getEndGame() const;
        std::vector<float> getObservation() const;
        float stepAgent(int action, int frameCount);
        void resetGame();

        // function
        
        void spwanenemy();
        void bulletf();
        void poolevent();

        // update
        void updateMousePos();
        
        void updateBullet();
        void updateship();
        void updateenemy();
        void update();
        void updatetext();
        
        // render
        void renderenemy();
        void renderShip();
        void renderBullet();
        void render();
        void renderText(sf::RenderTarget& target);
};


