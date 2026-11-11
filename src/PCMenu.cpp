#include "../include/PCMenu.h"
#include "../include/CatchMinigame.h"
#include "../include/ConsoleUtils.h"
#include "../include/GameSettings.h"
#include "../include/MenuUtils.h"
#include "../include/PokemonSummary.h"
#include "../include/TypeTags.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const PokemonNode *nodeAt(const DoublyLinkedList &list, int position)
{
    const PokemonNode *cur = list.getHead();
    for (int i = 1; cur != nullptr && i < position; ++i)
        cur = cur->next;
    return cur;
}

PokemonNode *getPokemonAtPosition(const DoublyLinkedList &list, int position)
{
    if (position < 1 || position > list.getCount())
        return nullptr;

    PokemonNode *pokemon = list.getHead();
    for (int currentPosition = 1; currentPosition < position; currentPosition++)
        pokemon = pokemon->next;

    return pokemon;
}

void depositPartyPokemonIntoFirstEmptySlot(DoublyLinkedList &party, PCSystem &pc)
{
    const int firstEmptySlot = pc.getCurrentBoxCount() + 1;
    if (firstEmptySlot > BOX_CAPACITY)
    {
        reportError("The active box is full.");
        return;
    }
    if (party.getCount() <= 1)
    {
        reportError("At least one Pokémon must remain in the party.");
        return;
    }

    displayParty(party);
    cout << "First empty slot in [" << pc.getCurrentBoxName() << "]: "
         << firstEmptySlot << "/" << BOX_CAPACITY << endl;
    const int partySlot = readMenuChoice(
        "Choose a party Pokémon to deposit (0 to cancel): ", party.getCount(), true,
        [&party, &pc, firstEmptySlot]() {
            displayParty(party);
            cout << "First empty slot in [" << pc.getCurrentBoxName() << "]: "
                 << firstEmptySlot << "/" << BOX_CAPACITY << endl;
        });
    if (partySlot == 0)
        return;

    PokemonNode *moved = party.removeByPosition(partySlot);
    if (moved == nullptr)
    {
        reportError("Could not remove the selected Pokémon from the party.");
        return;
    }
    pc.getCurrentList().insertNode(moved);
    cout << "[SUCCESS] Deposited " << moved->nickname << " into slot "
         << firstEmptySlot << " of [" << pc.getCurrentBoxName() << "]." << endl;
    waitForContinue();
}

void printPokemonSlotContext(const PokemonNode *pokemon, const string &location,
                             int slot)
{
    if (pokemon == nullptr)
        return;

    printMenuSeparator();
    cout << "Selected: " << pokemon->nickname;
    if (pokemon->species != nullptr)
    {
        cout << " (" << pokemon->species->name << ", #"
             << pokemon->species->pokedexID << ")";
    }
    cout << "\nLocation: " << location << " | Slot " << slot << endl;
    printMenuSeparator();
}

bool changePokemonNickname(PokemonNode *pokemon)
{
    if (pokemon == nullptr)
    {
        reportError("Could not find that Pokémon.");
        return false;
    }

    cout << "Current nickname: " << pokemon->nickname << endl;
    cout << "Enter a new nickname (blank to cancel): ";
    string nickname;
    if (!getline(cin, nickname))
    {
        reportError("Could not read the nickname; no changes were made.");
        return false;
    }

    if (nickname.find_first_not_of(" \t") == string::npos)
    {
        cout << "Nickname change cancelled." << endl;
        return true;
    }

    const string oldNickname = pokemon->nickname;
    pokemon->nickname = nickname;
    cout << "[SUCCESS] Renamed " << oldNickname << " to " << pokemon->nickname << "." << endl;
    return true;
}

void showPartySlotActions(const DoublyLinkedList &party, const PCSystem &pc,
                          int slot)
{
    clearScreen();
    printMenuHeader("PARTY SLOT ACTIONS");
    printPokemonSlotContext(getPokemonAtPosition(party, slot), "Party", slot);
    cout << "1. Deposit to Active Box [" << pc.getCurrentBoxName() << "]" << endl;
    cout << "2. Swap Position in Party" << endl;
    cout << "3. Change Nickname" << endl;
    cout << "4. View Pokédex Entry" << endl;
    cout << "5. Release Pokémon" << endl;
    printMenuSeparator();
    cout << "0. Cancel" << endl;
}

void showPartySlotActionsOutsideBox(const DoublyLinkedList &party, int slot)
{
    clearScreen();
    printMenuHeader("PARTY SLOT ACTIONS");
    printPokemonSlotContext(getPokemonAtPosition(party, slot), "Party", slot);
    cout << "1. Swap Position in Party" << endl;
    cout << "2. Change Nickname" << endl;
    cout << "3. View Pokédex Entry" << endl;
    printMenuSeparator();
    cout << "0. Cancel" << endl;
}

void showBoxSlotActions(PCSystem &pc, int slot)
{
    clearScreen();
    printMenuHeader("BOX SLOT ACTIONS");
    printPokemonSlotContext(
        getPokemonAtPosition(pc.getCurrentList(), slot),
        "Box " + to_string(pc.getCurrentBoxNumber()) + " (" +
            pc.getCurrentBoxName() + ")",
        slot);
    cout << "1. Withdraw to Party" << endl;
    cout << "2. Swap Position in Box" << endl;
    cout << "3. Move to another Box" << endl;
    cout << "4. Change Nickname" << endl;
    cout << "5. View Pokédex Entry" << endl;
    cout << "6. Release Pokémon" << endl;
    printMenuSeparator();
    cout << "0. Cancel" << endl;
}

void handlePartySubMenu(DoublyLinkedList &party, PCSystem &pc, int slot)
{
    if (slot < 1 || slot > party.getCount())
    {
        reportInvalidInput("Invalid party slot position.");
        return;
    }

    while (true)
    {
        showPartySlotActions(party, pc, slot);
        int action = readMenuChoice(
            "\nChoice: ", 5, true,
            [&party, &pc, slot]() { showPartySlotActions(party, pc, slot); });
        if (action == 0)
            break;

        bool shouldReturn = false;
        if (action == 1)
        {
            if (party.getCount() <= 1)
            {
                reportError("Minimum 1 Pokémon required in party!");
                continue;
            }
            if (pc.getCurrentBoxCount() >= BOX_CAPACITY)
            {
                reportError("Active box is full!");
                continue;
            }

            PokemonNode *moved = party.removeByPosition(slot);
            if (moved)
            {
                pc.getCurrentList().insertNode(moved);
                cout << "[SUCCESS] Deposited " << moved->nickname << " into [" << pc.getCurrentBoxName() << "]." << endl;
                shouldReturn = true;
            }
        }
        else if (action == 2)
        {
            if (party.getCount() < 2)
            {
                reportError("Need at least 2 Pokémon in party to swap!");
                continue;
            }
            int targetPos = readMenuChoice("\nSwap slot " + to_string(slot) + " with slot # (0 to cancel): ",
                                           party.getCount(), true,
                                           [&party, &pc, slot]() {
                                               showPartySlotActions(party, pc, slot);
                                           });
            if (targetPos == 0)
                continue;
            if (party.swapNodes(slot, targetPos))
            {
                cout << "[SUCCESS] Swapped slot " << slot << " and " << targetPos << "." << endl;
                shouldReturn = true;
            }
            else
            {
                reportInvalidInput("Invalid target position.");
                continue;
            }
        }
        else if (action == 3)
        {
            if (!changePokemonNickname(getPokemonAtPosition(party, slot)))
                continue;
        }
        else if (action == 4)
        {
            slot = browsePokemonSummaries(party, slot);
            continue;
        }
        else if (action == 5)
        {
            if (party.getCount() <= 1)
            {
                reportError("Cannot release your last Pokémon!");
                continue;
            }

            const PokemonNode *p = nodeAt(party, slot);
            if (getGameSettings().confirmRelease &&
                !askYesNo("Release " + p->nickname + "? This can't be undone.",
                          [&party, &pc, slot]() {
                              showPartySlotActions(party, pc, slot);
                          }))
            {
                cout << "Release cancelled." << endl;
                waitForContinue();
                continue;
            }

            if (party.releaseByPosition(slot))
            {
                cout << "[SUCCESS] Released Pokémon at slot " << slot << "." << endl;
            }
            else
            {
                reportError("Failed to release Pokémon.");
                continue;
            }
        }
        else
        {
            reportInvalidInput("Invalid choice.");
            continue;
        }

        if (shouldReturn)
            break;
        waitForContinue();
    }
}

void managePartyOutsideBox(DoublyLinkedList &party, PCSystem &)
{
    while (true)
    {
        displayParty(party);
        if (party.isEmpty())
        {
            waitForContinue();
            return;
        }

        int slot = readMenuChoice(
            "Select Slot # to manage (0 to go back): ", party.getCount(), true,
            [&party]() { displayParty(party); });
        if (slot == 0)
            return;

        while (true)
        {
            showPartySlotActionsOutsideBox(party, slot);
            int action = readMenuChoice(
                "\nChoice: ", 3, true,
                [&party, slot]() { showPartySlotActionsOutsideBox(party, slot); });
            if (action == 0)
                break;

            bool shouldReturnToColumns = false;
            if (action == 1)
            {
                if (party.getCount() < 2)
                {
                    reportError("Need at least 2 Pokémon in party to swap!");
                    continue;
                }
                int targetPos = readMenuChoice("\nSwap slot " + to_string(slot) + " with slot # (0 to cancel): ",
                                               party.getCount(), true,
                                               [&party, slot]() {
                                                   showPartySlotActionsOutsideBox(party, slot);
                                               });
                if (targetPos == 0)
                    continue;
                if (party.swapNodes(slot, targetPos))
                {
                    cout << "[SUCCESS] Swapped slot " << slot << " and " << targetPos << "." << endl;
                    shouldReturnToColumns = true;
                }
                else
                {
                    reportInvalidInput("Invalid target position.");
                    continue;
                }
            }
            else if (action == 2)
            {
                if (!changePokemonNickname(getPokemonAtPosition(party, slot)))
                    continue;
            }
            else if (action == 3)
            {
                slot = browsePokemonSummaries(party, slot);
                continue;
            }
            else
            {
                reportInvalidInput("Invalid choice.");
                continue;
            }

            if (shouldReturnToColumns)
                break;
            waitForContinue();
        }
    }
}

void handleBoxSubMenu(PCSystem &pc, DoublyLinkedList &party, int slot)
{
    DoublyLinkedList &boxList = pc.getCurrentList();
    if (slot < 1 || slot > boxList.getCount())
    {
        reportInvalidInput("Invalid box slot position.");
        return;
    }

    while (true)
    {
        showBoxSlotActions(pc, slot);
        int action = readMenuChoice(
            "\nChoice: ", 6, true,
            [&pc, slot]() { showBoxSlotActions(pc, slot); });
        if (action == 0)
            break;

        bool shouldReturn = false;
        if (action == 1)
        {
            if (party.getCount() >= MAX_PARTY_SIZE)
            {
                reportError("Party is full!");
                continue;
            }

            PokemonNode *moved = boxList.removeByPosition(slot);
            if (moved)
            {
                party.insertNode(moved);
                cout << "[SUCCESS] Withdrew " << moved->nickname << " to party." << endl;
                shouldReturn = true;
            }
        }
        else if (action == 2)
        {
            if (boxList.getCount() < 2)
            {
                reportError("Need at least 2 Pokémon in box to swap!");
                continue;
            }
            int targetPos = readMenuChoice("\nSwap slot " + to_string(slot) + " with slot # (0 to cancel): ",
                                           boxList.getCount(), true,
                                           [&pc, slot]() {
                                               showBoxSlotActions(pc, slot);
                                           });
            if (targetPos == 0)
                continue;
            if (boxList.swapNodes(slot, targetPos))
            {
                cout << "[SUCCESS] Swapped slot " << slot << " and " << targetPos << "." << endl;
                shouldReturn = true;
            }
            else
            {
                reportInvalidInput("Invalid target position.");
                continue;
            }
        }
        else if (action == 3)
        {
            int targetBox = readMenuChoice(
                "\nEnter Target Box (1-" + to_string(MAX_BOXES) + "): ",
                MAX_BOXES, false,
                [&pc, slot]() {
                    showBoxSlotActions(pc, slot);
                    cout << "Enter target box (1-" << MAX_BOXES << ")." << endl;
                });
            int srcBox = pc.getCurrentBoxNumber();
            if (targetBox == srcBox)
            {
                reportError("That is already the active box.");
                continue;
            }

            if (pc.moveBetweenBoxesByPosition(srcBox, targetBox, slot))
            {
                cout << "[SUCCESS] Moved slot " << slot << " to Box " << targetBox << "." << endl;
                shouldReturn = true;
            }
            else
            {
                reportError("Transfer failed (check box number or target capacity).");
                continue;
            }
        }
        else if (action == 4)
        {
            if (!changePokemonNickname(getPokemonAtPosition(boxList, slot)))
                continue;
        }
        else if (action == 5)
        {
            slot = browsePokemonSummaries(boxList, slot);
            continue;
        }
        else if (action == 6)
        {
            const PokemonNode *p = nodeAt(boxList, slot);
            if (getGameSettings().confirmRelease &&
                !askYesNo("Release " + p->nickname + "? This can't be undone.",
                          [&pc, slot]() {
                              showBoxSlotActions(pc, slot);
                          }))
            {
                cout << "Release cancelled." << endl;
                waitForContinue();
                continue;
            }

            if (boxList.releaseByPosition(slot))
            {
                cout << "[SUCCESS] Released Pokémon at slot " << slot << "." << endl;
            }
            else
            {
                reportError("Failed to release Pokémon.");
                continue;
            }
        }
        else
        {
            reportInvalidInput("Invalid choice.");
            continue;
        }

        if (shouldReturn)
            break;
        waitForContinue();
    }
}

void printSearchResults(PCSystem &pc, const string &title,
                        const vector<SearchResult> &results)
{
    if (results.empty())
    {
        clearScreen();
        printMenuHeader(title);
        cout << "(No matches)" << endl;
        waitForContinue();
        return;
    }

    constexpr size_t PAGE_SIZE = 15;
    size_t page = 0;
    while (true)
    {
        const size_t pageStart = page * PAGE_SIZE;
        const size_t pageEnd = min(results.size(), pageStart + PAGE_SIZE);
        clearScreen();
        printMenuHeader(title);
        cout << "Page " << (page + 1) << "/"
             << ((results.size() + PAGE_SIZE - 1) / PAGE_SIZE)
             << " | Select a result to view its summary." << endl;
        for (size_t i = pageStart; i < pageEnd; ++i)
        {
            const auto &r = results[i];
            cout << setw(2) << (i - pageStart + 1) << ". Box " << r.boxNumber
                 << " (" << r.boxName << ") Slot " << r.slot
                 << ": " << r.nickname << " (#" << right << setfill('0')
                 << setw(3) << r.dexNumber << setfill(' ') << " "
                 << r.speciesName << ") " << colorTypeTag(r.type1)
                 << (r.type2.empty() ? "" : " " + colorTypeTag(r.type2))
                 << " Gen " << r.generation << endl;
        }
        cout << results.size() << " found." << endl;

        const int selection = readSlotOrNav(
            "Result # to view | N = Next page | P = Previous page | 0 = Back: ",
            static_cast<int>(pageEnd - pageStart));
        if (selection == NAV_INVALID)
            continue;
        if (selection == 0)
            return;
        if (selection == NAV_NEXT)
        {
            if (pageEnd < results.size())
                ++page;
            else
                reportError("Already on the last page.");
            continue;
        }
        if (selection == NAV_PREV)
        {
            if (page > 0)
                --page;
            else
                reportError("Already on the first page.");
            continue;
        }
        if (selection < 1 || selection > static_cast<int>(pageEnd - pageStart))
        {
            reportInvalidInput("Invalid search result selection.");
            continue;
        }

        const SearchResult &result = results[pageStart + selection - 1];
        browsePokemonSummaries(pc.getList(result.boxNumber), result.slot);
    }
}

bool nameLess(const PokemonNode &a, const PokemonNode &b)
{
    string na = toLowerStr(a.species ? a.species->name : "");
    string nb = toLowerStr(b.species ? b.species->name : "");
    if (na != nb)
        return na < nb;
    return toLowerStr(a.nickname) < toLowerStr(b.nickname);
}

bool dexLess(const PokemonNode &a, const PokemonNode &b)
{
    int da = a.species ? a.species->pokedexID : 0;
    int db = b.species ? b.species->pokedexID : 0;
    if (da != db)
        return da < db;
    return toLowerStr(a.nickname) < toLowerStr(b.nickname);
}

int typeRank(const string &type)
{
    const string t = toLowerStr(type);
    for (size_t i = 0; i < POKEMON_TYPES.size(); ++i)
    {
        if (toLowerStr(POKEMON_TYPES[i]) == t)
            return static_cast<int>(i);
    }
    return static_cast<int>(POKEMON_TYPES.size());
}

NodeLess typeOrder(bool descending)
{
    return [descending](const PokemonNode &a, const PokemonNode &b)
    {
        int ra = typeRank(a.species ? a.species->type1 : "");
        int rb = typeRank(b.species ? b.species->type1 : "");
        if (ra != rb)
            return descending ? (ra > rb) : (ra < rb);
        return dexLess(a, b);
    };
}

void sortAllBoxesInGlobalOrder(PCSystem &pc, const NodeLess &less)
{
    vector<PokemonNode *> pokemon;
    for (int boxNumber = 1; boxNumber <= MAX_BOXES; ++boxNumber)
    {
        DoublyLinkedList &box = pc.getList(boxNumber);
        while (!box.isEmpty())
            pokemon.push_back(box.removeByPosition(1));
    }

    stable_sort(pokemon.begin(), pokemon.end(),
                [&less](const PokemonNode *left, const PokemonNode *right) {
                    return less(*left, *right);
                });

    for (size_t i = 0; i < pokemon.size(); ++i)
    {
        const int boxNumber = static_cast<int>(i / BOX_CAPACITY) + 1;
        pc.getList(boxNumber).insertNode(pokemon[i]);
    }
}

void handleSortMenu(PCSystem &pc, DoublyLinkedList &party)
{
    clearScreen();
    printMenuHeader("SORT POKÉMON");
    cout << "1. Active Box [" << pc.getCurrentBoxName() << "]" << endl;
    cout << "2. Party" << endl;
    cout << "3. All PC Boxes (sort each box independently)" << endl;
    cout << "4. All PC Boxes (global order; repack from Box 1)" << endl;
    printMenuSeparator();
    cout << "0. Back" << endl;
    int target = readMenuChoice("\nChoice: ", 4, true, [&pc]() {
        clearScreen();
        printMenuHeader("SORT POKÉMON");
        cout << "1. Active Box [" << pc.getCurrentBoxName() << "]" << endl;
        cout << "2. Party" << endl;
        cout << "3. All PC Boxes (sort each box independently)" << endl;
        cout << "4. All PC Boxes (global order; repack from Box 1)" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;
    });
    if (target == 0)
        return;

    vector<DoublyLinkedList *> lists;
    if (target == 1)
        lists.push_back(&pc.getCurrentList());
    else if (target == 2)
        lists.push_back(&party);
    else
    {
        for (int boxNumber = 1; boxNumber <= MAX_BOXES; ++boxNumber)
            lists.push_back(&pc.getList(boxNumber));
    }

    const bool hasSortablePokemon = any_of(
        lists.begin(), lists.end(),
        [](const DoublyLinkedList *list) { return list->getCount() >= 2; });
    if (!hasSortablePokemon)
    {
        reportError("Need at least 2 Pokémon to sort.");
        return;
    }

    clearScreen();
    printMenuHeader("SORT BY");
    cout << "1. Name" << endl;
    cout << "2. Type" << endl;
    cout << "3. Dex Number" << endl;
    printMenuSeparator();
    cout << "0. Back" << endl;
    int field = readMenuChoice("\nChoice: ", 3, true, []() {
        clearScreen();
        printMenuHeader("SORT BY");
        cout << "1. Name" << endl;
        cout << "2. Type" << endl;
        cout << "3. Dex Number" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;
    });
    if (field == 0)
        return;

    auto applySort = [&lists, &pc, target](const NodeLess &less) {
        if (target == 4)
        {
            sortAllBoxesInGlobalOrder(pc, less);
            return;
        }
        for (DoublyLinkedList *list : lists)
            list->sort(less);
    };
    auto showSortResult = [target, &pc](const string &description,
                                        const DoublyLinkedList *list) {
        clearScreen();
        if (target == 3)
        {
            cout << "[SUCCESS] Sorted Pokémon within each PC box "
                 << description << ". Box assignments were unchanged." << endl;
        }
        else if (target == 4)
        {
            size_t total = 0;
            for (int boxNumber = 1; boxNumber <= MAX_BOXES; ++boxNumber)
                total += static_cast<size_t>(pc.getList(boxNumber).getCount());
            cout << "[SUCCESS] Globally sorted " << total << " Pokémon across PC boxes "
                 << description << ", then repacked from Box 1 ("
                 << BOX_CAPACITY << " per box)." << endl;
        }
        else
        {
            cout << "[SUCCESS] Sorted " << description << "." << endl;
            list->displayBox();
        }
        waitForContinue();
    };

    if (field == 1)
    {
        clearScreen();
        printMenuHeader("SORT BY NAME");
        cout << "\n1. Ascending (A-Z)" << endl;
        cout << "2. Descending (Z-A)" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;
        int dir = readMenuChoice("\nChoice: ", 2, true, []() {
            clearScreen();
            printMenuHeader("SORT BY NAME");
            cout << "\n1. Ascending (A-Z)" << endl;
            cout << "2. Descending (Z-A)" << endl;
            printMenuSeparator();
            cout << "0. Back" << endl;
        });
        if (dir == 0)
            return;

        const NodeLess less = dir == 1
                                  ? NodeLess(nameLess)
                                  : NodeLess([](const PokemonNode &a,
                                                const PokemonNode &b) {
                                        return nameLess(b, a);
                                    });
        applySort(less);
        showSortResult("by name", lists.front());
    }
    else if (field == 2)
    {
        clearScreen();
        printMenuHeader("SORT BY TYPE");
        cout << "\n1. Ascending (Normal -> Fairy)" << endl;
        cout << "2. Descending (Fairy -> Normal)" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;
        int dir = readMenuChoice("\nChoice: ", 2, true, []() {
            clearScreen();
            printMenuHeader("SORT BY TYPE");
            cout << "\n1. Ascending (Normal -> Fairy)" << endl;
            cout << "2. Descending (Fairy -> Normal)" << endl;
            printMenuSeparator();
            cout << "0. Back" << endl;
        });
        if (dir == 0)
            return;

        applySort(typeOrder(dir == 2));
        showSortResult("by type (" +
                           string(dir == 1 ? "ascending" : "descending") + ")",
                       lists.front());
    }
    else if (field == 3)
    {
        clearScreen();
        printMenuHeader("SORT BY DEX NUMBER");
        cout << "\n1. Ascending (low -> high)" << endl;
        cout << "2. Descending (high -> low)" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;
        int dir = readMenuChoice("\nChoice: ", 2, true, []() {
            clearScreen();
            printMenuHeader("SORT BY DEX NUMBER");
            cout << "\n1. Ascending (low -> high)" << endl;
            cout << "2. Descending (high -> low)" << endl;
            printMenuSeparator();
            cout << "0. Back" << endl;
        });
        if (dir == 0)
            return;

        const NodeLess less = dir == 1
                                  ? NodeLess(dexLess)
                                  : NodeLess([](const PokemonNode &a,
                                                const PokemonNode &b) {
                                        return dexLess(b, a);
                                    });
        applySort(less);
        showSortResult("by Dex number (" +
                           string(dir == 1 ? "ascending" : "descending") + ")",
                       lists.front());
    }
}

bool boxGridView = true;

// Visible characters: skips colour codes and counts each UTF-8 character once
int visibleWidth(const string &s)
{
    int w = 0;
    bool inEscape = false;
    for (unsigned char c : s)
    {
        if (inEscape)
        {
            if (c == 'm')
                inEscape = false;
            continue;
        }
        if (c == 0x1B)
        {
            inEscape = true;
            continue;
        }
        if ((c & 0xC0) != 0x80)
            ++w;
    }
    return w;
}

string padTo(const string &s, int width)
{
    return s + string(max(0, width - visibleWidth(s)), ' ');
}

string fitText(const string &s, int width)
{
    if (visibleWidth(s) <= width)
        return s;

    string out;
    int n = 0;
    size_t i = 0;
    while (i < s.size() && n < width - 1)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3
                                   : (c >= 0xC0)   ? 2
                                                   : 1;
        out += s.substr(i, len);
        i += len;
        ++n;
    }
    return out + ".";
}

void printPokemonGrid(const DoublyLinkedList &list, int capacity, int cols)
{
    const int CELL_W = 23;
    const int rows = (capacity + cols - 1) / cols;

    vector<const PokemonNode *> mons;
    for (const PokemonNode *p = list.getHead(); p != nullptr; p = p->next)
        mons.push_back(p);

    for (int r = 0; r < rows; ++r)
    {
        string line1, line2, line3;
        for (int c = 0; c < cols; ++c)
        {
            const int idx = r * cols + c;
            if (idx >= capacity)
                break;

            ostringstream num;
            num << setw(2) << setfill('0') << (idx + 1);

            string top, middle, bottom;
            if (idx < static_cast<int>(mons.size()))
            {
                const PokemonNode *p = mons[idx];
                const SpeciesData *s = p->species;
                top = num.str() + (p->isShiny ? "\033[93m*\033[0m" : " ") +
                      fitText(p->nickname, CELL_W - 4);
                middle = " " + fitText(s ? s->name : "?", 12) + " L" + to_string(p->level);
                if (s)
                    bottom = " " + colorTypeTag(s->type1) +
                             (s->type2.empty() ? "" : " " + colorTypeTag(s->type2));
            }
            else
            {
                top = "\033[90m" + num.str() + " --\033[0m";
            }

            line1 += padTo(top, CELL_W);
            line2 += padTo(middle, CELL_W);
            line3 += padTo(bottom, CELL_W);
        }
        cout << line1 << "\n"
             << line2 << "\n"
             << line3 << "\n\n";
    }
}

void displayParty(const DoublyLinkedList &party)
{
    clearScreen();
    cout << "\n=== CURRENT PARTY (" << party.getCount() << "/" << MAX_PARTY_SIZE << ") "
         << fillBar(party.getCount(), MAX_PARTY_SIZE, 6) << " ===" << endl;
    printPokemonGrid(party, MAX_PARTY_SIZE, 3);
}

void showActiveBox(PCSystem &pc)
{
    clearScreen();
    const int count = pc.getCurrentBoxCount();
    cout << "\n=== BOX " << pc.getCurrentBoxNumber() << "/" << MAX_BOXES << ": "
         << pc.getCurrentBoxName() << " (" << count << "/" << BOX_CAPACITY << ") "
         << fillBar(count, BOX_CAPACITY, 15) << " ===" << endl;

    if (boxGridView)
    {
        printPokemonGrid(pc.getCurrentList(), BOX_CAPACITY, 5);
        return;
    }
    if (count == 0)
    {
        cout << "(Empty)" << endl;
        return;
    }
    pc.getCurrentList().displayBox();
}

void printBoxOverview(PCSystem &pc)
{
    clearScreen();
    printMenuHeader("ALL BOXES (> = active)");
    const int active = pc.getCurrentBoxNumber();

    for (int row = 0; row < 10; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            const int b = col * 10 + row + 1;
            string name = pc.getBoxName(b);
            if (name.size() > 10)
                name = name.substr(0, 9) + ".";

            ostringstream cell;
            cell << (b == active ? '>' : ' ') << setw(2) << b << " "
                 << left << setw(10) << name << right
                 << " " << setw(2) << pc.getList(b).getCount() << "/" << BOX_CAPACITY;
            cout << cell.str() << "   ";
        }
        cout << endl;
    }
    waitForContinue();
}

void displayBoxSettingsMenu()
{
    clearScreen();
    printMenuHeader("BOX SETTINGS");
    cout << "1. Switch Active Box" << endl;
    cout << "2. Rename Active Box" << endl;
    cout << "3. View All Boxes Overview" << endl;
    printMenuSeparator();
    cout << "0. Back" << endl;
}

void viewPCMenu(PCSystem &pc, DoublyLinkedList &party)
{
    int choice = -1;
    while (choice != 0)
    {
        clearScreen();
        printMenuHeader("POKÉMON STORAGE SYSTEM");
        cout << "Active Box: [" << pc.getCurrentBoxName() << "]" << endl;
        cout << "Party: " << party.getCount() << "/" << MAX_PARTY_SIZE << endl;
        cout << "1. Manage Party (" << party.getCount() << "/" << MAX_PARTY_SIZE << ")" << endl;
        cout << "2. Manage Active Box (" << pc.getCurrentBoxCount() << "/" << BOX_CAPACITY << ")" << endl;
        cout << "3. Box Settings (Switch / Rename / Overview)" << endl;
        cout << "4. Search Pokémon in PC" << endl;
        cout << "5. Sort Pokémon" << endl;
        printMenuSeparator();
        cout << "0. Back" << endl;

        choice = readMenuChoice("\nSelect option: ", 5, true, [&pc, &party]() {
            clearScreen();
            printMenuHeader("POKÉMON STORAGE SYSTEM");
            cout << "Active Box: [" << pc.getCurrentBoxName() << "]" << endl;
            cout << "Party: " << party.getCount() << "/" << MAX_PARTY_SIZE << endl;
            cout << "1. Manage Party (" << party.getCount() << "/" << MAX_PARTY_SIZE << ")" << endl;
            cout << "2. Manage Active Box (" << pc.getCurrentBoxCount() << "/" << BOX_CAPACITY << ")" << endl;
            cout << "3. Box Settings (Switch / Rename / Overview)" << endl;
            cout << "4. Search Pokémon in PC" << endl;
            cout << "5. Sort Pokémon" << endl;
            printMenuSeparator();
            cout << "0. Back" << endl;
        });
        if (choice != -1)
        {
            if (choice == 1)
            {
                while (true)
                {
                    displayParty(party);
                    if (party.isEmpty())
                    {
                        waitForContinue();
                        break;
                    }

                    int slot = readMenuChoice(
                        "Select Slot # to manage (0 to go back): ",
                        party.getCount(), true,
                        [&party]() { displayParty(party); });
                    if (slot == 0)
                        break;
                    handlePartySubMenu(party, pc, slot);
                }
            }
            else if (choice == 2)
            {
                while (true)
                {
                    showActiveBox(pc);
                    int slot = readSlotOrNav("Slot # to manage | N = Next | P = Previous | G = Grid/List | B = Jump box | E = Deposit to first empty slot | 0 = Back: ",
                                             pc.getCurrentList().getCount());
                    if (slot == NAV_INVALID)
                        continue;
                    if (slot == 0)
                        break;
                    if (slot == NAV_NEXT)
                    {
                        pc.setCurrentBox(pc.getCurrentBoxNumber() >= MAX_BOXES
                                             ? 1
                                             : pc.getCurrentBoxNumber() + 1);
                    }
                    else if (slot == NAV_PREV)
                    {
                        pc.setCurrentBox(pc.getCurrentBoxNumber() == 1
                                             ? MAX_BOXES
                                             : pc.getCurrentBoxNumber() - 1);
                    }
                    else if (slot == NAV_GRID)
                    {
                        boxGridView = !boxGridView;
                    }
                    else if (slot == NAV_BOX)
                    {
                        const int boxNumber = readMenuChoice(
                            "Jump to box (1-" + to_string(MAX_BOXES) +
                                ", 0 to cancel): ",
                            MAX_BOXES, true,
                            [&pc]() { showActiveBox(pc); });
                        if (boxNumber != 0)
                            pc.setCurrentBox(boxNumber);
                    }
                    else if (slot == NAV_EMPTY)
                    {
                        depositPartyPokemonIntoFirstEmptySlot(party, pc);
                    }
                    else
                    {
                        handleBoxSubMenu(pc, party, slot);
                    }
                }
            }
            else if (choice == 3)
            {
                displayBoxSettingsMenu();
                int sub = readMenuChoice("\nChoice: ", 3, true,
                                         displayBoxSettingsMenu);
                if (sub == 0)
                    continue;

                if (sub == 1)
                {
                    int boxNum = readMenuChoice(
                        "\nEnter Box (1-" + to_string(MAX_BOXES) + "): ",
                        MAX_BOXES, false,
                        displayBoxSettingsMenu);
                    if (pc.switchBox(boxNum))
                        waitForContinue();
                }
                else if (sub == 2)
                {
                    string name;
                    do
                    {
                        cout << "New name: ";
                        if (!getline(cin, name))
                            return;
                        if (name.empty())
                        {
                            reportInvalidInput("Box name cannot be empty.");
                            displayBoxSettingsMenu();
                        }
                    } while (name.empty());
                    pc.renameCurrentBox(name);
                    waitForContinue();
                }
                else if (sub == 3)
                {
                    printBoxOverview(pc);
                }
            }
            else if (choice == 4)
            {
                clearScreen();
                printMenuHeader("SEARCH POKÉMON IN PC");
                cout << "1. By nickname / species" << endl;
                cout << "2. By type" << endl;
                cout << "3. By generation" << endl;
                cout << "4. By Dex number" << endl;
                printMenuSeparator();
                cout << "0. Back" << endl;
                int sub = readMenuChoice("\nChoice: ", 4, true, []() {
                    clearScreen();
                    printMenuHeader("SEARCH POKÉMON IN PC");
                    cout << "1. By nickname / species" << endl;
                    cout << "2. By type" << endl;
                    cout << "3. By generation" << endl;
                    cout << "4. By Dex number" << endl;
                    printMenuSeparator();
                    cout << "0. Back" << endl;
                });
                if (sub == 0)
                    continue;

                string query;
                if (sub == 1)
                {
                    cout << "Enter nickname or species: ";
                    getline(cin, query);
                    printSearchResults(pc, "SEARCH: \"" + query + "\"", pc.search(query));
                }
                else if (sub == 2)
                {
                    string type1 = chooseType();
                    if (!type1.empty())
                    {
                        int second = chooseSecondType(type1);
                        if (second >= 1 && second <= 18)
                        {
                            string type2 = POKEMON_TYPES[second - 1];
                            printSearchResults(pc, "TYPE: " + colorTypeTag(type1) + " / " +
                                                   colorTypeTag(type2) + " (either order)",
                                               pc.searchByTypePair(type1, type2));
                        }
                        else if (second == 19)
                        {
                            printSearchResults(pc, "TYPE: " + colorTypeTag(type1) + " (any second type)",
                                               pc.searchByType(type1));
                        }
                        else if (second == 20)
                        {
                            printSearchResults(pc, "TYPE: " + colorTypeTag(type1) + " (single-type only)",
                                               pc.searchByMonoType(type1));
                        }
                    }
                }
                else if (sub == 3)
                {
                    int gen = chooseGeneration();
                    if (gen != 0)
                    {
                        printSearchResults(pc, "GENERATION: " + GENERATION_LABELS[gen - 1],
                                           pc.searchByGeneration(gen));
                    }
                }
                else if (sub == 4)
                {
                    int dex = promptDexNumber([]() {
                        clearScreen();
                        printMenuHeader("SEARCH POKÉMON IN PC");
                        cout << "1. By nickname / species" << endl;
                        cout << "2. By type" << endl;
                        cout << "3. By generation" << endl;
                        cout << "4. By Dex number" << endl;
                        printMenuSeparator();
                        cout << "0. Back" << endl;
                    });
                    if (dex != 0)
                    {
                        printSearchResults(pc, "DEX #" + to_string(dex), pc.searchByDexNumber(dex));
                    }
                }
            }
            else if (choice == 5)
            {
                handleSortMenu(pc, party);
            }
            else if (choice != 0)
            {
                reportInvalidInput("Invalid choice.");
            }
        }
        else
        {
            reportInvalidInput("Please enter a number.");
        }
    }
}
