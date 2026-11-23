#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "Game.h"

namespace
{
void writeObservation(const Game& game)
{
    const std::vector<float> observation = game.getObservation();
    for(std::size_t i = 0; i < observation.size(); ++i){
        if(i > 0) std::cout << ',';
        std::cout << observation[i];
    }
}

int runAgentMode(bool headless)
{
    Game game(headless, true);
    std::string line;
    std::cout << std::setprecision(8);

    while(std::getline(std::cin, line)){
        std::istringstream request(line);
        std::string command;
        request >> command;

        if(command == "RESET"){
            unsigned seed;
            if(!(request >> seed)){
                std::cerr << "RESET requires an unsigned seed.\n";
                return 1;
            }
            std::srand(seed);
            game.resetGame();
            if(!headless) game.render();
            writeObservation(game);
            std::cout << std::endl;
            if(!headless && !game.running()){
                return 0;
            }
        }
        else if(command == "STEP"){
            int action;
            int frameCount;
            if(!(request >> action >> frameCount)){
                std::cerr << "STEP requires an action and frame count.\n";
                return 1;
            }
            try{
                const float reward = game.stepAgent(action, frameCount);
                std::cout << reward << ',' << (game.getEndGame() ? 1 : 0) << ',';
                writeObservation(game);
                std::cout << std::endl;
            }
            catch(const std::exception& error){
                std::cerr << error.what() << '\n';
                return 1;
            }
        }
        else if(command == "QUIT"){
            return 0;
        }
        else if(!command.empty()){
            std::cerr << "Unknown agent command: " << command << '\n';
            return 1;
        }
    }
    return 0;
}
}

int main(int argc, char* argv[]){
    if(argc > 1 && std::string(argv[1]) == "--rl"){
        return runAgentMode(true);
    }
    if(argc > 1 && std::string(argv[1]) == "--play"){
        return runAgentMode(false);
    }

    // init game engine
    std::srand(static_cast<unsigned>(time(NULL)));
    Game game;

    while(game.running()){

        // update
        game.update();
        // Render
        game.render();
    }

    return 0;
}