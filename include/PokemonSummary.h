#ifndef POKEMON_SUMMARY_VIEW_H
#define POKEMON_SUMMARY_VIEW_H

#include "PokemonNode.h"
#include "DoublyLinkedList.h"
#include <string>

void renderAutoCenteredSprite(const std::string &filepath, int targetBoxWidth = 32, int targetBoxHeight = 32);
void playPokemonCry(int pokedexID);
std::string formatAbilities(std::string abilitiesStr);
void printPokemonSummary(const PokemonNode *pokemon, bool playCry = false, bool showSprite = false);
int browsePokemonSummaries(const DoublyLinkedList &list, int position);
void printSpeciesSummary(const SpeciesData *species, bool encountered, bool caught,
                         bool showSprite = false, bool waitForContinue = true);

#endif