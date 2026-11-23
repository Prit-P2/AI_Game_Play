#include "Game.h"
#include <algorithm>
#include <stdexcept>

Game::Game(bool headless, bool agentControlled)
    : headless(headless), agentControlled(agentControlled){
    this->initVariable();
    if(!this->headless){
        this->initwindow();
        this->initfount();
        this->initText();
    }
    this->initship();
    this->initbullet();
    this->initenemy();
}

Game::~Game(){
    delete this->window;


}
// Accessor
const bool Game::running() const{
    return this->window != nullptr && this->window->isOpen();
}
const bool Game::getEndGame() const{
    return this->endGame;
}

// function
void Game::spwanenemy(){
    /*

    spawn enemy colour ansd and position random and add to 
    vector and moves the enemy
    */
   this->enemy.setPosition(sf::Vector2f(
         static_cast<float>(rand()%static_cast<int>(windowWidth - this->enemy.getSize().x)),
        0.f
   ));
   // Randomize enemy type
   int type = rand()%5;

   switch (type)
   {
   case 0:
   this->enemy.setSize(sf::Vector2f(20.f,20.f));
    this->enemy.setFillColor(sf::Color::Magenta);
    /* code */
    break;
    case 1:
   this->enemy.setSize(sf::Vector2f(30.f,30.f));
    this->enemy.setFillColor(sf::Color::Blue);
    /* code */
    break;
    case 2:
   this->enemy.setSize(sf::Vector2f(40.f,40.f));
    this->enemy.setFillColor(sf::Color::White);
    /* code */
    break;
    case 3:
   this->enemy.setSize(sf::Vector2f(50.f,50.f));
    this->enemy.setFillColor(sf::Color::Cyan);
    /* code */
    break;
    case 4:
   this->enemy.setSize(sf::Vector2f(60.f,60.f));
    this->enemy.setFillColor(sf::Color::Green);
    /* code */
    break;
   
   default:
   this->enemy.setSize(sf::Vector2f(60.f,60.f));
    this->enemy.setFillColor(sf::Color::Green);
    break;
   }


   

   // spawn enemy
   this->enemies.push_back(this->enemy);

   // End of the screen


}
void Game::bulletf()
{
    this->bullet.setPosition(sf::Vector2f(
        this->ship.getPosition().x + (this->ship.getSize().x - this->bullet.getSize().x) / 2.f,
        this->ship.getPosition().y - this->bullet.getSize().y
    ));
    this->bullets.push_back(this->bullet);
}
void Game::poolevent()
{
    if(this->window == nullptr){
        return;
    }
    while(const std::optional event = this->window->pollEvent()){
            if(event->is<sf::Event::Closed>()){
                this->window->close();
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)){
                this->window->close();
            }
        }
}
void Game::updateMousePos(){
    /*
    return void 
    update the mpuse position
    - mouse position relative to window
    */
   this->mousePosWindow = sf::Mouse::getPosition(*this->window);
   this->mousePosview = this->window->mapPixelToCoords(this->mousePosWindow);
}

void Game::updateBullet()
{
    
    if(this->bullets.size()<100){
        if(this->fireTime >= this->fireMaxTime)
        {
            this->bulletf();
            this->fireTime=0;
        }else{
            this->fireTime+=1.f;
        }
        
    }
    for(int i=static_cast<int>(this->bullets.size())-1;i>=0;i--){
        this->bullets[i].move(sf::Vector2f(0.f,-5.f));

        if(this->bullets[i].getPosition().y + this->bullets[i].getSize().y < 0.f){
            this->bullets.erase(this->bullets.begin()+i);
        }
    }
}
void Game::updateship()
{
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)){
        this->moveShip(-1);
    }
    else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)){
        this->moveShip(1);
    }
}

void Game::moveShip(int direction)
{
    const float maxX = static_cast<float>(windowWidth) - this->ship.getSize().x;
    float x = this->ship.getPosition().x;
    if(direction < 0){
        x = std::max(0.f, x - 5.f);
    }
    else if(direction > 0){
        x = std::min(maxX, x + 5.f);
    }
    this->ship.setPosition(sf::Vector2f(x, this->ship.getPosition().y));
}
void Game::updateenemy()
{
    /*
        update the enemy spawn timer when the total 
        amount of enemy is spawn 
        removes the enemy at the edgs of the screen
    */
    // Updating the timer for enemy spaning
    if(this->enemies.size()<this->maxenemy)
    {
        if(this->enemySpawntimer >= this->enmyspawnTimeMax){
            // Spawining the enemy and reset the timer
            this->spwanenemy();
            this->enemySpawntimer = 0.f;
        }
        else
            this->enemySpawntimer += 1.f;
    }

    //Move the enemyes and updating
    for(int i=0; i<this->enemies.size();i++)
    {
        this->enemies[i].move(sf::Vector2f(0.f,5.f));
        bool deleted = false;

        // if enemy pass buttom of the screent
        if(this->enemies[i].getPosition().y > windowHeight){
            this->enemies.erase(this->enemies.begin()+i);
            this->health-=1;
            // std::cout << "Health: " << this->health << "\n";
        }
        
    }
    //check if clicked upon
    if(!this->agentControlled && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        if(this->mouseHel == false){

            this->mouseHel = true;
            bool deleted=false;
            for(size_t i = 0;i < this->enemies.size() && deleted==false;i++){
                // delett enemy
                if(this->enemies[i].getGlobalBounds().contains(this->mousePosview))
                {
                    // Gain points
                    if(this->enemies[i].getFillColor() == sf::Color::Magenta){
                    this->points+=10.f;
                    }
                    else if(this->enemies[i].getFillColor() == sf::Color::Blue){
                        this->points+=9.f;
                    }
                    else if(this->enemies[i].getFillColor() == sf::Color::White){
                        this->points+=8.f;
                    }
                    else if(this->enemies[i].getFillColor() == sf::Color::Cyan){
                        this->points+=7.f;
                    }
                    else if(this->enemies[i].getFillColor() == sf::Color::Green){
                        this->points+=6.f;
                    }
                    // std::cout << "Points: " << this->points << "\n";

                    
                    deleted=true;
                    this->enemies.erase(this->enemies.begin()+i);
                }
            }
        }
    }
    else
        {
            this->mouseHel = false;
        }
    bool deleted=false;
    for(size_t i = 0;i < this->enemies.size() && deleted==false;i++){
        // delett enemy
        for(int j=0;j<this->bullets.size();j++)
        {
            if(this->enemies[i].getGlobalBounds().findIntersection(this->bullets[j].getGlobalBounds()))
            {
                // Gain points
                if(this->enemies[i].getFillColor() == sf::Color::Magenta){
                this->points+=10.f;
                }
                else if(this->enemies[i].getFillColor() == sf::Color::Blue){
                    this->points+=9.f;
                }
                else if(this->enemies[i].getFillColor() == sf::Color::White){
                    this->points+=8.f;
                }
                else if(this->enemies[i].getFillColor() == sf::Color::Cyan){
                    this->points+=7.f;
                }
                else if(this->enemies[i].getFillColor() == sf::Color::Green){
                    this->points+=6.f;
                }
                // std::cout << "Points: " << this->points << "\n";

                
                deleted=true;
                this->enemies.erase(this->enemies.begin()+i);
                break;
            }
        }
    }
}
std::vector<float> Game::getObservation() const
{
    constexpr std::size_t enemySlots = 5;
    constexpr std::size_t bulletSlots = 100;
    std::vector<float> observation;
    observation.reserve(2 + enemySlots * 5 + bulletSlots * 3);

    observation.push_back(this->ship.getPosition().x / static_cast<float>(windowWidth));
    observation.push_back(static_cast<float>(this->health) / 20.f);

    std::vector<const sf::RectangleShape*> sortedEnemies;
    sortedEnemies.reserve(this->enemies.size());
    for(const auto& currentEnemy : this->enemies){
        sortedEnemies.push_back(&currentEnemy);
    }
    std::sort(sortedEnemies.begin(), sortedEnemies.end(),
        [](const sf::RectangleShape* left, const sf::RectangleShape* right){
            return left->getPosition().y > right->getPosition().y;
        });

    for(std::size_t i = 0; i < enemySlots; ++i){
        if(i < sortedEnemies.size()){
            const auto& currentEnemy = *sortedEnemies[i];
            const float size = currentEnemy.getSize().x;
            float value = 0.f;
            if(currentEnemy.getFillColor() == sf::Color::Magenta) value = 1.f;
            else if(currentEnemy.getFillColor() == sf::Color::Blue) value = 0.9f;
            else if(currentEnemy.getFillColor() == sf::Color::White) value = 0.8f;
            else if(currentEnemy.getFillColor() == sf::Color::Cyan) value = 0.7f;
            else if(currentEnemy.getFillColor() == sf::Color::Green) value = 0.6f;

            observation.push_back(currentEnemy.getPosition().x / static_cast<float>(windowWidth));
            observation.push_back(currentEnemy.getPosition().y / static_cast<float>(windowHeight));
            observation.push_back(size / 60.f);
            observation.push_back(value);
            observation.push_back(1.f);
        }
        else{
            observation.insert(observation.end(), 5, 0.f);
        }
    }

    for(std::size_t i = 0; i < bulletSlots; ++i){
        if(i < this->bullets.size()){
            observation.push_back(this->bullets[i].getPosition().x / static_cast<float>(windowWidth));
            observation.push_back(this->bullets[i].getPosition().y / static_cast<float>(windowHeight));
            observation.push_back(1.f);
        }
        else{
            observation.insert(observation.end(), 3, 0.f);
        }
    }
    return observation;
}

float Game::stepAgent(int action, int frameCount)
{
    if(action < 0 || action > 2){
        throw std::invalid_argument("Action must be 0 (stay), 1 (left), or 2 (right).");
    }
    if(frameCount <= 0){
        throw std::invalid_argument("Frame count must be positive.");
    }
    if(this->endGame){
        return 0.f;
    }

    const unsigned previousPoints = this->points;
    const int previousHealth = this->health;
    for(int frame = 0; frame < frameCount && !this->endGame; ++frame){
        if(this->window != nullptr){
            this->poolevent();
            if(!this->window->isOpen()){
                this->endGame = true;
                break;
            }
        }
        if(action == 1) this->moveShip(-1);
        else if(action == 2) this->moveShip(1);

        this->updateBullet();
        this->updateenemy();
        if(this->health <= 0){
            this->endGame = true;
        }
    }
    if(this->window != nullptr && this->window->isOpen()){
        this->updatetext();
        this->render();
    }
    return static_cast<float>(this->points - previousPoints) / 10.f
        - 2.f * static_cast<float>(previousHealth - this->health);
}
void Game::updatetext(){
    std::stringstream ss;
    ss<<"Points: "<< this->points<<"\n"
        <<"Health: "<< this->health<<"\n";
    this->uiText.setString(ss.str());
}
void Game::update(){
   this->poolevent();

   if(this->endGame){
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)){
        this->resetGame();
    }
    return;
   }
   
    this->updateMousePos();
    this->updatetext();
    this->updateship();
    this->updateBullet();
    this->updateenemy();

   if(this->health <= 0 ){
    this->endGame = true;
   }

}
void Game::renderenemy(){
    for(auto &e:this->enemies)
    {
        this->window->draw(e);
    }

}

void Game::renderShip()
{
    this->window->draw(this->ship);

}

void Game::renderBullet()
{
    for(auto &e:this->bullets){
        this->window->draw(e);
    }
}

void Game::render(){
    if(this->window == nullptr || !this->window->isOpen()){
        return;
    }
    this->window->clear(sf::Color::Black);
    
    // Draw game object
    this->renderShip();
    this->renderBullet();
    this->renderenemy();
    this->renderText(*this->window);
    if(this->endGame){
        this->window->draw(this->gameOverText);
    }
    this->window->display();


}
void Game::renderText(sf::RenderTarget& target){

    target.draw(this->uiText);
    
}

// private function
void Game::initVariable(){
    this->window=nullptr;
    
    // Game logic
    this->points = 0;
    this->enmyspawnTimeMax = 10.f;
    this->enemySpawntimer = this->enmyspawnTimeMax;
    this->maxenemy = 5;
    this->mouseHel = false;
    this->health = 20;
    this->endGame = false;

}
void Game::initenemy(){
    this->enemy.setPosition(sf::Vector2f(10.f,10.f));
    // this->enemy.setSize(sf::Vector2f(50.f,50.f));
    this->enemy.setFillColor(sf::Color::Cyan);
    // this->enemy.setOutlineColor(sf::Color::Green);
    // this->enemy.setOutlineThickness(1.f);



}
void Game::initship(){
    this->ship.setSize(sf::Vector2f(50.f,50.f));
    this->ship.setPosition(sf::Vector2f(
        10.f,
        static_cast<float>(windowHeight) - this->ship.getSize().y - 10.f
    ));
    this->ship.setFillColor(sf::Color::Red);
    
    
}
void Game::initbullet(){
    // this->bullet.setPosition(sf::Vector2f(20.f,20.f));
    this->bullet.setFillColor(sf::Color::Yellow);
    this->bullet.setSize(sf::Vector2f(10.f,10.f));
    this->fireMaxTime=5;
    this->fireTime=0;

}
void Game::initfount(){
    if(!this->font.openFromFile("Pixellettersfull-BnJ5.ttf")){
        std::cout << "not load ttf";
    }
}
void Game::initText(){
    this->uiText.setFont(this->font);
    this->uiText.setCharacterSize(24);
    this->uiText.setFillColor(sf::Color::White);
    this->uiText.setString("None");

    this->gameOverText.setFont(this->font);
    this->gameOverText.setCharacterSize(36);
    this->gameOverText.setFillColor(sf::Color::Red);
    this->gameOverText.setString("GAME OVER\nPress R to restart");
    this->gameOverText.setPosition(sf::Vector2f(220.f, 250.f));
}

void Game::resetGame(){
    this->points = 0;
    this->health = 20;
    this->endGame = false;
    this->enemySpawntimer = this->enmyspawnTimeMax;
    this->fireTime = 0.f;
    this->mouseHel = false;
    this->enemies.clear();
    this->bullets.clear();
    this->initship();
    this->updatetext();
}

void Game::initwindow()
{
    this->window = new sf::RenderWindow(sf::VideoMode({windowWidth, windowHeight}), "My window");
    this->window->setFramerateLimit(60);
}
