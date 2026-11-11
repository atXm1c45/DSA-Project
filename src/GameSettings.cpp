#include "../include/GameSettings.h"

using namespace std;

GameSettings &getGameSettings()
{
    static GameSettings settings;
    return settings;
}
