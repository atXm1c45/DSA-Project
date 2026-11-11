#include "../include/PokemonSummary.h"
#include "../include/ConsoleUtils.h"
#include "../include/GameSettings.h"
#include "../include/MenuUtils.h"
#include "../include/TypeTags.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <sstream>
#include <string>
#include <algorithm>

#include "../include/stb_image.h"

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void setupConsole()
{
    SetConsoleOutputCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#endif

using namespace std;

string toTitleCase(string text)
{
    if (text.empty())
        return "";

    for (char &c : text)
    {
        if (c == '-')
            c = ' ';
    }

    bool capitalizeNext = true;
    for (char &c : text)
    {
        const unsigned char character = static_cast<unsigned char>(c);
        if (isspace(character))
        {
            capitalizeNext = true;
        }
        else if (capitalizeNext)
        {
            c = static_cast<char>(toupper(character));
            capitalizeNext = false;
        }
        else
        {
            c = static_cast<char>(tolower(character));
        }
    }
    return text;
}

string formatAbilities(string abilitiesStr)
{
    if (abilitiesStr.empty())
        return "";

    size_t pos = 0;
    while ((pos = abilitiesStr.find('|')) != string::npos)
    {
        abilitiesStr.replace(pos, 1, ", ");
    }

    return toTitleCase(abilitiesStr);
}

void playPokemonCry(int pokedexID)
{
    if (!getGameSettings().soundEnabled)
        return;
#ifdef _WIN32
    string cryPath = "assets/cries/" + to_string(pokedexID) + ".wav";
    PlaySoundA(cryPath.c_str(), NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void renderAutoCenteredSprite(const string &filepath, int targetBoxWidth, int targetBoxHeight)
{
    if (!getGameSettings().spritesEnabled)
        return;

    int fullWidth, height, channels;
    unsigned char *img = stbi_load(filepath.c_str(), &fullWidth, &height, &channels, 4);

    if (!img)
    {
        reportError("Could not load image: " + filepath);
        return;
    }

    int frameWidth = (fullWidth >= 64) ? fullWidth / 2 : fullWidth;

    int minX = frameWidth, maxX = -1;
    int minY = height, maxY = -1;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < frameWidth; x++)
        {
            int alphaIdx = (y * fullWidth + x) * 4 + 3;
            if (img[alphaIdx] >= 128)
            {
                if (x < minX)
                    minX = x;
                if (x > maxX)
                    maxX = x;
                if (y < minY)
                    minY = y;
                if (y > maxY)
                    maxY = y;
            }
        }
    }

    if (maxX == -1 || maxY == -1)
    {
        cout << "[Empty Sprite]\n";
        stbi_image_free(img);
        return;
    }

    int contentWidth = (maxX - minX) + 1;
    int contentHeight = (maxY - minY) + 1;

    int autoMarginX = (max)(0, (targetBoxWidth - contentWidth) / 2);
    int autoMarginY = (max)(0, (targetBoxHeight - contentHeight) / 2);

    int cropMinX = (max)(0, minX - autoMarginX);
    int cropMaxX = (min)(frameWidth - 1, maxX + autoMarginX);
    int cropMinY = (max)(0, minY - autoMarginY);
    int cropMaxY = (min)(height - 1, maxY + autoMarginY);

    if (cropMinY % 2 != 0)
    {
        cropMinY = (max)(0, cropMinY - 1);
    }

    ostringstream renderedSprite;
    for (int y = cropMinY; y <= cropMaxY; y += 2)
    {
        for (int x = cropMinX; x <= cropMaxX; x++)
        {
            int topIdx = (y * fullWidth + x) * 4;
            unsigned char tR = img[topIdx], tG = img[topIdx + 1], tB = img[topIdx + 2], tA = img[topIdx + 3];

            int bR = 0, bG = 0, bB = 0, bA = 0;
            if (y + 1 <= cropMaxY)
            {
                int botIdx = ((y + 1) * fullWidth + x) * 4;
                bR = img[botIdx];
                bG = img[botIdx + 1];
                bB = img[botIdx + 2];
                bA = img[botIdx + 3];
            }

            if (tA < 128 && bA < 128)
            {
                renderedSprite << " ";
                continue;
            }

            renderedSprite << "\033[38;2;" << (int)tR << ";" << (int)tG << ";" << (int)tB << "m"
                           << "\033[48;2;" << (int)bR << ";" << (int)bG << ";" << (int)bB << "m"
                           << "▀\033[0m";
        }
        renderedSprite << "\n";
    }

    stbi_image_free(img);
    cout << renderedSprite.str() << flush;
}

namespace
{
string getSpeciesSpritePath(const SpeciesData *species, bool isShiny)
{
    const string spriteFolder = isShiny ? "assets/spritesShiny/" : "assets/sprites/";
    const string formSprite = spriteFolder + to_string(species->internalID) + ".png";
    const string baseSprite = spriteFolder + to_string(species->pokedexID) + ".png";
    const string fallbackSprite = spriteFolder + "0.png";
    if (ifstream(formSprite, ios::binary).good())
        return formSprite;
    if (ifstream(baseSprite, ios::binary).good())
        return baseSprite;
    return fallbackSprite;
}

void printSummary(const PokemonNode *pokemon, const SpeciesData *species,
                  bool playCry, bool showSprite, bool waitForInput = true)
{
    clearScreen();
    if (!species)
    {
        reportError("Invalid species data.");
        return;
    }

    if (playCry)
    {
        playPokemonCry(species->pokedexID);
    }

    if (showSprite)
    {
        renderAutoCenteredSprite(getSpeciesSpritePath(
            species, pokemon != nullptr && pokemon->isShiny));
    }

    cout << "\n====================================================================\n";
    cout << "  #" << species->pokedexID << " " << toTitleCase(species->name);
    if (!species->genus.empty())
    {
        cout << " (" << toTitleCase(species->genus) << ")";
    }
    cout << "\n  Type: " << colorTypeTag(species->type1);
    if (!species->type2.empty())
    {
        cout << " " << colorTypeTag(species->type2);
    }
    cout << "\n====================================================================\n";

    if (pokemon != nullptr)
    {
        cout << "  [CAUGHT POKÉMON]\n";
        cout << "    Nickname       : " << pokemon->nickname << endl;
        if (!pokemon->gender.empty())
        {
            cout << "    Gender         : " << toTitleCase(pokemon->gender) << endl;
        }
        cout << "    Level          : " << pokemon->level << endl;
        cout << "    Nature         : " << toTitleCase(pokemon->nature) << endl;
        cout << "    Current Ability: " << formatAbilities(pokemon->ability) << endl;
        cout << "    Shiny          : " << (pokemon->isShiny ? "Yes" : "No") << endl;
        cout << "    Date Caught    : " << pokemon->dateCaught << endl;

        cout << "--------------------------------------------------------------------\n";
    }

    // Metadata & Classification
    cout << "  [CLASSIFICATION & METADATA]\n";
    cout << "    Generation   : Gen " << species->generation << endl;
    cout << "    Category     : ";
    if (species->isLegendary)
        cout << "Legendary Pokémon\n";
    else if (species->isMythical)
        cout << "Mythical Pokémon\n";
    else
        cout << "Standard Pokémon\n";

    cout << "    Abilities    : " << formatAbilities(species->abilities);
    if (!species->hiddenAbility.empty())
    {
        cout << " (Hidden: " << formatAbilities(species->hiddenAbility) << ")";
    }
    cout << endl;
    cout << "    Capture Rate : " << species->captureRate << endl;

    cout << "--------------------------------------------------------------------\n";

    // Physical Traits & Habitat
    cout << "  [PHYSICAL CHARACTERISTICS]\n";
    cout << "    Height          : " << fixed << setprecision(2)
         << species->heightM << "m" << endl;
    cout << "    Weight          : " << fixed << setprecision(2)
         << species->weightKg << "kg" << endl;
    cout << "    Color           : " << toTitleCase(species->color) << endl;
    cout << "    Shape           : " << toTitleCase(species->shape) << endl;
    cout << "    Egg Groups      : " << formatAbilities(species->eggGroups) << endl;
    if (!species->habitat.empty())
    {
        cout << "    Habitat         : " << toTitleCase(species->habitat) << endl;
    }

    cout << "--------------------------------------------------------------------\n";

    // Base Stats
    cout << "  [STATS] (BST: " << species->baseStatTotal << ")\n";
    cout << "    HP      : " << species->hp << endl;
    cout << "    Attack  : " << species->attack << endl;
    cout << "    Defense : " << species->defense << endl;
    cout << "    Sp. Atk : " << species->spAttack << endl;
    cout << "    Sp. Def : " << species->spDefense << endl;
    cout << "    Speed   : " << species->speed << endl;

    cout << "--------------------------------------------------------------------\n";

    // Pokédex Flavor Text
    cout << "  [DESCRIPTION]\n";
    ConsoleUtils::printTypewriterLine("   \"" + species->flavorText + "\"");
    cout << "====================================================================\n\n";
    if (waitForInput)
        waitForContinue();
}
} // namespace

void printPokemonSummary(const PokemonNode *pokemon, bool playCry, bool showSprite)
{
    if (pokemon == nullptr)
    {
        clearScreen();
        reportError("Invalid Pokémon data.");
        return;
    }

    printSummary(pokemon, pokemon->species, playCry, showSprite);
}

int browsePokemonSummaries(const DoublyLinkedList &list, int position)
{
    if (position < 1 || position > list.getCount())
        return position;

    while (true)
    {
        const PokemonNode *pokemon = list.getHead();
        for (int current = 1; pokemon != nullptr && current < position; ++current)
            pokemon = pokemon->next;

        if (pokemon == nullptr)
        {
            reportError("Could not find that Pokémon.");
            return position;
        }

        printSummary(pokemon, pokemon->species, true, true, false);
        while (true)
        {
            const int navigation = readSlotOrNav(
                "N = Next Pokémon | P = Previous Pokémon | 0 = Back: ",
                0);
            if (navigation == NAV_INVALID)
            {
                printSummary(pokemon, pokemon->species, false, true, false);
                continue;
            }
            if (navigation == 0)
                return position;
            if (navigation == NAV_NEXT && position < list.getCount())
            {
                ++position;
                break;
            }
            if (navigation == NAV_PREV && position > 1)
            {
                --position;
                break;
            }

            reportInvalidInput("No Pokémon in that direction.");
            printSummary(pokemon, pokemon->species, false, true, false);
        }
    }
}

void printSpeciesSummary(const SpeciesData *species, bool encountered, bool caught,
                         bool showSprite, bool waitForInput)
{
    if (caught)
    {
        printSummary(nullptr, species, false, showSprite, waitForInput);
        return;
    }

    clearScreen();
    if (species == nullptr)
    {
        reportError("Invalid species data.");
        return;
    }

    if (showSprite && encountered)
    {
        cout << "\n";
        playPokemonCry(species->pokedexID);
        renderAutoCenteredSprite(getSpeciesSpritePath(species, false));
    }

    cout << "\n====================================================================\n";
    cout << "  #" << species->pokedexID << " "
         << (encountered ? toTitleCase(species->name) : "???") << "\n";
    cout << "====================================================================\n\n";
    cout << "  ???\n\n";
    if (waitForInput)
        waitForContinue();
}