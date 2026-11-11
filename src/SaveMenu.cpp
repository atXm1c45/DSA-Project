#include "../include/SaveMenu.h"
#include "../include/CatchMinigame.h"
#include "../include/ConsoleUtils.h"
#include "../include/GameSettings.h"
#include "../include/MenuUtils.h"
#include "../include/SaveSystem.h"
#include <iostream>

using namespace std;

extern const string SAVE_FILE = "pokemon_save.csv";

namespace
{
void displaySaveMenu()
{
    clearScreen();
    printMenuHeader("SAVE / LOAD");
    cout << "1. Save" << endl;
    cout << "2. Load" << endl;
    printMenuSeparator();
    cout << "0. Back" << endl;
}

void displaySaveFileInfo(const SaveFileInfo &info, const string &infoError)
{
    clearScreen();
    printMenuHeader("SAVE FILE INFORMATION");
    if (infoError.empty())
    {
        cout << "Last saved: "
             << (info.hasTimestamp ? info.savedAt : "Not available (older save)") << endl;
        cout << "Party: " << info.partyCount << "/" << MAX_PARTY_SIZE << endl;
        cout << "Pokédex registered: " << info.registeredDexCount << "/1025" << endl;
        cout << "Pokédex encountered: " << info.encounteredDexCount << "/1025" << endl;
    }
    else
    {
        cout << "[WARNING] Could not read save summary: " << infoError << endl;
        cout << "The full save validation will happen during loading." << endl;
    }
    cout << "Backup available: "
         << (saveFileExists(SAVE_FILE + ".bak") ? "Yes" : "No") << endl;
}
}

void handleSaveMenu(PCSystem &pc, DoublyLinkedList &party,
                    const vector<SpeciesData *> &pokedex, set<int> &registeredSpecies,
                    set<int> &encounteredSpecies)
{
    displaySaveMenu();
    int choice = readMenuChoice("\nChoice: ", 2, true, displaySaveMenu);
    if (choice == 0)
        return;

    if (choice == 1)
    {
        clearScreen();
        cout << "Saving game..." << endl;
        timeDelay(500);
        if (saveGame(SAVE_FILE, pc, party, registeredSpecies, encounteredSpecies))
        {
            ConsoleUtils::printTypewriterLine("[SUCCESS] Saved to " + SAVE_FILE + ".");
            waitForContinue();
        }
        else
        {
            // saveGame reports the error and waits for acknowledgment.
        }
    }
    else if (choice == 2)
    {
        if (!saveFileExists(SAVE_FILE))
        {
            reportError("No save file found.");
            return;
        }
        SaveFileInfo info;
        string infoError;
        readSaveFileInfo(SAVE_FILE, pokedex, info, infoError);
        displaySaveFileInfo(info, infoError);
        waitForContinue();
        if (getGameSettings().confirmOverwrite &&
            !askYesNo("Loading replaces your current party and boxes. Continue?",
                      [&info, &infoError]() {
                          displaySaveFileInfo(info, infoError);
                      }))
        {
            cout << "Load cancelled." << endl;
            waitForContinue();
            return;
        }
        clearScreen();
        cout << "Loading game..." << endl;
        timeDelay(500);
        if (loadGame(SAVE_FILE, pc, party, pokedex, MAX_PARTY_SIZE,
                     registeredSpecies, encounteredSpecies))
        {
            ConsoleUtils::printTypewriterLine("[SUCCESS] Loaded " + SAVE_FILE + ".");
            waitForContinue();
        }
        else
        {
            // loadGame reports the error and waits for acknowledgment.
        }
    }
}
