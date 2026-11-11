#ifndef SAVE_SYSTEM_H
#define SAVE_SYSTEM_H

#include "CSVParser.h"
#include "DoublyLinkedList.h"
#include "PCSystem.h"
#include <cstddef>
#include <set>
#include <string>
#include <vector>

bool saveFileExists(const std::string &filename);
struct SaveFileInfo
{
    std::string savedAt;
    std::size_t partyCount = 0;
    std::size_t registeredDexCount = 0;
    std::size_t encounteredDexCount = 0;
    bool hasTimestamp = false;
};

bool readSaveFileInfo(const std::string &filename,
                      const std::vector<SpeciesData *> &pokedex,
                      SaveFileInfo &info, std::string &error);
bool saveGame(const std::string &filename, PCSystem &pc, DoublyLinkedList &party,
              const std::set<int> &registeredSpecies,
              const std::set<int> &encounteredSpecies,
              bool skipOverwriteConfirmation = false);
bool loadGame(const std::string &filename, PCSystem &pc, DoublyLinkedList &party,
              const std::vector<SpeciesData *> &pokedex, int maxPartySize,
              std::set<int> &registeredSpecies,
              std::set<int> &encounteredSpecies);

#endif