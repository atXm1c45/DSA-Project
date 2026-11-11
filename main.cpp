#include "include/CSVParser.h"
#include "include/CatchMinigame.h"
#include "include/ConsoleUtils.h"
#include "include/DeveloperTools.h"
#include "include/MenuUtils.h"
#include "include/PCMenu.h"
#include "include/PCSystem.h"
#include "include/PokedexMenu.h"
#include "include/GameSettings.h"
#include "include/SaveMenu.h"
#include "include/SaveSystem.h"
#include "include/TitleScreen.h"
#include <iostream>
#include <set>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef _WIN32

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")

void setupConsoleOutput()
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
void registerListSpecies(set<int> &registeredSpecies, set<int> &encounteredSpecies,
                         const DoublyLinkedList &list)
{
    for (const PokemonNode *pokemon = list.getHead(); pokemon != nullptr; pokemon = pokemon->next)
    {
        if (pokemon->species != nullptr)
        {
            registeredSpecies.insert(pokemon->species->internalID);
            encounteredSpecies.insert(pokemon->species->internalID);
        }
    }
}

void displayMainMenu(const DoublyLinkedList &party, const PCSystem &pc,
                     const SaveFileInfo &saveInfo, bool hasSaveInfo)
{
    clearScreen();
    cout << "\n╔══════════════════════════════════════╗" << endl;
    cout << "║               PokéCLI                ║" << endl;
    cout << "╚══════════════════════════════════════╝" << endl;
    cout << " Party: " << party.getCount() << "/" << MAX_PARTY_SIZE
         << "       Active Box: " << pc.getCurrentBoxNumber() << "/"
         << MAX_BOXES << endl;
    cout << " Save: " << (saveFileExists(SAVE_FILE) ? "Available" : "Not found") << endl;
    cout << " Last saved: "
         << (hasSaveInfo
                 ? (saveInfo.hasTimestamp ? saveInfo.savedAt
                                          : "Not available (older save)")
                 : (saveFileExists(SAVE_FILE) ? "Information unavailable" : "Never"))
         << endl;
    cout << "\n  ADVENTURE" << endl;
    cout << "  1. Start an Expedition" << endl;
    cout << "\n  MANAGEMENT" << endl;
    cout << "  2. View Party" << endl;
    cout << "  3. View PC" << endl;
    cout << "  4. Pokédex" << endl;
    cout << "\n  SYSTEM" << endl;
    cout << "  5. Save / Load" << endl;
    cout << "  6. Settings" << endl;
    if (getGameSettings().developerMode)
        cout << "  7. Developer Tools" << endl;
    cout << "  0. Exit" << endl;
}

void displaySettingsMenu(const GameSettings &settings)
{
    clearScreen();
    cout << "\n--- SETTINGS ---" << endl;
    cout << "1. Sprites: " << (settings.spritesEnabled ? "On" : "Off") << endl;
    cout << "2. Sound effects: " << (settings.soundEnabled ? "On" : "Off") << endl;
    cout << "3. Colored type tags: " << (settings.typeTagsEnabled ? "On" : "Off") << endl;
    cout << "4. Developer mode (enable developer tools): "
         << (settings.developerMode ? "On" : "Off") << endl;
    cout << "5. Confirm before releasing Pokémon: "
         << (settings.confirmRelease ? "On" : "Off") << endl;
    cout << "6. Confirm before overwriting save/session: "
         << (settings.confirmOverwrite ? "On" : "Off") << endl;
    const char *textSpeed = settings.textSpeed == TextSpeed::Slow ? "Slow"
                            : settings.textSpeed == TextSpeed::Fast ? "Fast"
                                                                    : "Medium";
    cout << "7. Text speed: " << textSpeed << endl;
    cout << "8. Reset settings to defaults" << endl;
    cout << "Settings are stored in the save file when you save the game." << endl;
    cout << "0. Back" << endl;
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    setupConsoleOutput();
#endif

    const string csvFilename = "pokemon_complete.csv";
    if (!checkCSVFile(csvFilename))
    {
        reportError("Could not find or open " + csvFilename + ".");
        return 1;
    }

    vector<SpeciesData *> pokedex = loadSpeciesDatabase(csvFilename);
    if (pokedex.empty())
    {
        reportError("Failed to load Pokedex database.");
        return 1;
    }

    displayTitleScreen();
    const int titleChoice = readMenuChoice("\nSelect option: ", 1, true,
                                           displayTitleScreen);
    if (titleChoice == 0)
    {
        for (SpeciesData *species : pokedex)
            delete species;
        clearScreen();
        return 0;
    }

    SaveFileInfo currentSaveInfo;
    string saveInfoError;
    bool hasCurrentSaveInfo = saveFileExists(SAVE_FILE) &&
                              readSaveFileInfo(SAVE_FILE, pokedex,
                                               currentSaveInfo, saveInfoError);

    {
        PCSystem pc;
        DoublyLinkedList party;
        set<int> registeredSpecies;
        set<int> encounteredSpecies;

        bool loaded = false;
        if (saveFileExists(SAVE_FILE))
        {
            loaded = loadGame(SAVE_FILE, pc, party, pokedex, MAX_PARTY_SIZE,
                              registeredSpecies, encounteredSpecies);
            if (loaded)
                cout << "[INFO] Loaded " << SAVE_FILE << "." << endl;
        }

        if (!loaded)
        {
            const SpeciesData *starter = getSpeciesByPokedexNumber(pokedex, 25);
            if (starter)
                party.append(starter, "Pikachu", "Male", 5, "Static", "Hardy", false, "2026-11-10");

            registerListSpecies(registeredSpecies, encounteredSpecies, party);
        }

        int choice = -1;
        while (choice != 0)
        {
            displayMainMenu(party, pc, currentSaveInfo, hasCurrentSaveInfo);
            const int maxMainOption = getGameSettings().developerMode ? 7 : 6;
            choice = readMenuChoice("\nSelect option: ", maxMainOption, true, [&]() {
                displayMainMenu(party, pc, currentSaveInfo, hasCurrentSaveInfo);
            });

            if (choice == 1)
            {
                startExpedition(pokedex, party, pc, registeredSpecies, encounteredSpecies);
            }
            else if (choice == 2)
            {
                managePartyOutsideBox(party, pc);
            }
            else if (choice == 3)
            {
                viewPCMenu(pc, party);
            }
            else if (choice == 4)
            {
                handlePokedexMenu(pokedex, registeredSpecies, encounteredSpecies);
            }
            else if (choice == 5)
            {
                handleSaveMenu(pc, party, pokedex, registeredSpecies, encounteredSpecies);
                hasCurrentSaveInfo = saveFileExists(SAVE_FILE) &&
                                     readSaveFileInfo(SAVE_FILE, pokedex,
                                                      currentSaveInfo, saveInfoError);
            }
            else if (choice == 6)
            {
                bool settingsOpen = true;
                while (settingsOpen)
                {
                    const GameSettings &settings = getGameSettings();
                    displaySettingsMenu(settings);
                    const int setting = readMenuChoice(
                        "\nSelect setting: ", 8, true,
                        [&settings]() { displaySettingsMenu(settings); });
                    GameSettings &mutableSettings = getGameSettings();
                    if (setting == 0)
                    {
                        settingsOpen = false;
                    }
                    else if (setting == 1)
                    {
                        mutableSettings.spritesEnabled = !mutableSettings.spritesEnabled;
                    }
                    else if (setting == 2)
                    {
                        mutableSettings.soundEnabled = !mutableSettings.soundEnabled;
                    }
                    else if (setting == 3)
                    {
                        mutableSettings.typeTagsEnabled = !mutableSettings.typeTagsEnabled;
                    }
                    else if (setting == 4)
                    {
                        mutableSettings.developerMode = !mutableSettings.developerMode;
                    }
                    else if (setting == 5)
                    {
                        mutableSettings.confirmRelease = !mutableSettings.confirmRelease;
                    }
                    else if (setting == 6)
                    {
                        mutableSettings.confirmOverwrite = !mutableSettings.confirmOverwrite;
                    }
                    else if (setting == 7)
                    {
                        mutableSettings.textSpeed =
                            mutableSettings.textSpeed == TextSpeed::Slow
                                ? TextSpeed::Medium
                                : mutableSettings.textSpeed == TextSpeed::Medium
                                      ? TextSpeed::Fast
                                      : TextSpeed::Slow;
                    }
                    else if (setting == 8 &&
                             askYesNo("Reset all settings to defaults?",
                                      []() { displaySettingsMenu(getGameSettings()); }))
                    {
                        mutableSettings = GameSettings{};
                    }
                }
            }
            else if (choice == 7 && getGameSettings().developerMode)
            {
                handleDeveloperTools(pokedex, pc, party,
                                     registeredSpecies, encounteredSpecies);
                hasCurrentSaveInfo = saveFileExists(SAVE_FILE) &&
                                     readSaveFileInfo(SAVE_FILE, pokedex,
                                                      currentSaveInfo, saveInfoError);
            }
        }

        if (askYesNo("Save before exiting?", [&]() {
                displayMainMenu(party, pc, currentSaveInfo, hasCurrentSaveInfo);
            }))
        {
            if (saveGame(SAVE_FILE, pc, party, registeredSpecies, encounteredSpecies))
                ConsoleUtils::printTypewriterLine(
                    "[SUCCESS] Saved to " + SAVE_FILE + ".");
        }
    }

    for (auto *species : pokedex)
        delete species;
    clearScreen();
    return 0;
}
