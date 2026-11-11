#include "../include/SaveSystem.h"
#include "../include/GameSettings.h"
#include "../include/ConsoleUtils.h"
#include "../include/MenuUtils.h"
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace std;

namespace
{
const int SAVE_VERSION = 9;

struct SavedPokemon
{
    int location;
    const SpeciesData *species;
    string nickname;
    string gender;
    int level;
    string ability;
    string nature;
    bool isShiny;
    string dateCaught;
};

string quote(const string &s)
{
    string out = "\"";
    for (char c : s)
    {
        if (c == '"')
            out += "\"\"";
        else if (c == '\n' || c == '\r')
            out += ' ';
        else
            out += c;
    }
    out += '"';
    return out;
}

bool toInt(const string &s, int &out)
{
    try
    {
        size_t pos = 0;
        out = stoi(s, &pos);
        return pos == s.size();
    }
    catch (const exception &)
    {
        return false;
    }
}

const SpeciesData *findByInternalID(const vector<SpeciesData *> &pokedex, int id)
{
    for (const SpeciesData *s : pokedex)
    {
        if (s->internalID == id)
            return s;
    }
    return nullptr;
}

void writeList(ofstream &out, int location, const DoublyLinkedList &list)
{
    for (const PokemonNode *n = list.getHead(); n != nullptr; n = n->next)
    {
        if (n->species == nullptr)
            continue;

        out << "POKEMON," << location << "," << n->species->internalID << ","
            << quote(n->nickname) << "," << quote(n->gender) << "," << n->level << ","
            << quote(n->ability) << "," << quote(n->nature) << ","
            << (n->isShiny ? 1 : 0) << "," << quote(n->dateCaught) << "\n";
    }
}

void clearList(DoublyLinkedList &list)
{
    while (!list.isEmpty())
        list.releaseByPosition(1);
}

string currentTimestamp()
{
    const time_t now = time(nullptr);
    tm localTime = {};
#ifdef _WIN32
    if (localtime_s(&localTime, &now) != 0)
        return "";
#else
    if (localtime_r(&now, &localTime) == nullptr)
        return "";
#endif

    ostringstream result;
    result << put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return result.str();
}

bool fileExists(const string &filename)
{
    ifstream file(filename, ios::binary);
    return file.good();
}
} // namespace

bool saveFileExists(const string &filename)
{
    ifstream f(filename);
    return f.good();
}

bool readSaveFileInfo(const string &filename,
                      const vector<SpeciesData *> &pokedex,
                      SaveFileInfo &info, string &error)
{
    ifstream in(filename);
    if (!in.is_open())
    {
        error = "Could not open " + filename + ".";
        return false;
    }

    SaveFileInfo parsedInfo;
    set<int> registeredDexNumbers;
    set<int> encounteredDexNumbers;
    bool sawVersion = false;
    bool sawTimestamp = false;
    int version = 0;
    string line;
    int lineNumber = 0;

    while (getline(in, line))
    {
        ++lineNumber;
        if (line.empty() || line == "\r")
            continue;

        const vector<string> tokens = parseCSVLine(line);
        if (tokens.empty())
            continue;

        if (tokens[0] == "VERSION")
        {
            if (sawVersion || tokens.size() != 2 || !toInt(tokens[1], version) ||
                version < 1 || version > SAVE_VERSION)
            {
                error = "Invalid save version on line " + to_string(lineNumber) + ".";
                return false;
            }
            sawVersion = true;
        }
        else if (!sawVersion)
        {
            error = "Missing save version on line " + to_string(lineNumber) + ".";
            return false;
        }
        else if (tokens[0] == "SAVED_AT")
        {
            if (version < 6 || sawTimestamp || tokens.size() != 2 || tokens[1].empty())
            {
                error = "Invalid save timestamp on line " + to_string(lineNumber) + ".";
                return false;
            }
            parsedInfo.savedAt = tokens[1];
            parsedInfo.hasTimestamp = true;
            sawTimestamp = true;
        }
        else if (tokens[0] == "POKEMON")
        {
            int location = 0;
            int speciesID = 0;
            if (tokens.size() != 10 || !toInt(tokens[1], location) ||
                !toInt(tokens[2], speciesID) || location < 0 || location > MAX_BOXES)
            {
                error = "Invalid Pokémon record on line " + to_string(lineNumber) + ".";
                return false;
            }

            const SpeciesData *species =
                version == 1 ? getSpeciesByPokedexNumber(pokedex, speciesID)
                             : findByInternalID(pokedex, speciesID);
            if (species == nullptr)
            {
                error = "Unknown species on line " + to_string(lineNumber) + ".";
                return false;
            }

            if (location == 0)
                ++parsedInfo.partyCount;
            registeredDexNumbers.insert(species->pokedexID);
            encounteredDexNumbers.insert(species->pokedexID);
        }
        else if (tokens[0] == "POKEDEX" || tokens[0] == "ENCOUNTERED")
        {
            int speciesID = 0;
            if (tokens.size() != 2 || !toInt(tokens[1], speciesID))
            {
                error = "Invalid Pokédex record on line " + to_string(lineNumber) + ".";
                return false;
            }
            const SpeciesData *species = findByInternalID(pokedex, speciesID);
            if (species == nullptr)
            {
                error = "Unknown Pokédex species on line " + to_string(lineNumber) + ".";
                return false;
            }
            if (tokens[0] == "POKEDEX")
                registeredDexNumbers.insert(species->pokedexID);
            encounteredDexNumbers.insert(species->pokedexID);
        }
    }

    if (!sawVersion)
    {
        error = "Save file is empty.";
        return false;
    }
    if (version >= 6 && !sawTimestamp)
    {
        error = "Save file is missing its timestamp.";
        return false;
    }

    parsedInfo.registeredDexCount = registeredDexNumbers.size();
    parsedInfo.encounteredDexCount = encounteredDexNumbers.size();
    info = parsedInfo;
    error.clear();
    return true;
}

bool saveGame(const string &filename, PCSystem &pc, DoublyLinkedList &party,
              const set<int> &registeredSpecies, const set<int> &encounteredSpecies,
              bool skipOverwriteConfirmation)
{
    if (!skipOverwriteConfirmation && getGameSettings().confirmOverwrite &&
        fileExists(filename) &&
        !askYesNo("Overwrite the existing save file? A backup will be kept.", []() {
            clearScreen();
            printMenuHeader("OVERWRITE SAVE");
            cout << "The existing save will be backed up before replacement." << endl;
        }))
    {
        cout << "Save cancelled." << endl;
        return false;
    }

    const string tmpName = filename + ".tmp";
    const string savedAt = currentTimestamp();
    if (savedAt.empty())
    {
        reportError("Could not determine the current date and time.");
        return false;
    }

    {
        ofstream out(tmpName);
        if (!out.is_open())
        {
            reportError("Could not write " + tmpName + ".");
            return false;
        }

        out << "VERSION," << SAVE_VERSION << "\n";
        out << "CURRENT_BOX," << pc.getCurrentBoxNumber() << "\n";
        for (int b = 1; b <= MAX_BOXES; ++b)
            out << "BOXNAME," << b << "," << quote(pc.getBoxName(b)) << "\n";

        const GameSettings &settings = getGameSettings();
        out << "SETTINGS," << settings.spritesEnabled << ","
            << settings.soundEnabled << "," << settings.typeTagsEnabled << ","
            << settings.developerMode << "," << settings.confirmRelease << ","
            << settings.confirmOverwrite << ","
            << static_cast<int>(settings.textSpeed) << "\n";
        out << "POKEDEX_CURSOR," << settings.lastPokedexEntry << "\n";
        out << "SAVED_AT," << quote(savedAt) << "\n";

        for (int id : registeredSpecies)
            out << "POKEDEX," << id << "\n";
        for (int id : encounteredSpecies)
            out << "ENCOUNTERED," << id << "\n";

        writeList(out, 0, party);
        for (int b = 1; b <= MAX_BOXES; ++b)
            writeList(out, b, pc.getList(b));

        out.flush();
        if (!out)
        {
            reportError("Failed while writing the save file.");
            return false;
        }
        out.close();
        if (!out)
        {
            reportError("Failed while closing the temporary save file.");
            return false;
        }
    } // file is closed here

    const string backupName = filename + ".bak";
    const bool hadPreviousSave = fileExists(filename);
    if (hadPreviousSave)
    {
        if (fileExists(backupName) && std::remove(backupName.c_str()) != 0)
        {
            reportError("Could not replace the previous backup " + backupName + ".");
            std::remove(tmpName.c_str());
            return false;
        }
        if (std::rename(filename.c_str(), backupName.c_str()) != 0)
        {
            reportError("Could not back up the existing save; it was left unchanged.");
            std::remove(tmpName.c_str());
            return false;
        }
    }

    if (std::rename(tmpName.c_str(), filename.c_str()) != 0)
    {
        reportError("Could not finalize the save file.");
        if (hadPreviousSave && std::rename(backupName.c_str(), filename.c_str()) != 0)
        {
            reportError("The previous save remains at " + backupName +
                        " and could not be restored automatically.");
        }
        std::remove(tmpName.c_str());
        return false;
    }

    if (hadPreviousSave)
        cout << "[INFO] Previous save backed up to " << backupName << "." << endl;
    return true;
}

bool loadGame(const string &filename, PCSystem &pc, DoublyLinkedList &party,
              const vector<SpeciesData *> &pokedex, int maxPartySize,
              set<int> &registeredSpecies, set<int> &encounteredSpecies)
{
    ifstream in(filename);
    if (!in.is_open())
    {
        reportError("No save file found (" + filename + ").");
        return false;
    }

    int currentBox = 1;
    bool sawVersion = false;
    bool sawSettings = false;
    bool sawTimestamp = false;
    bool sawPokedexCursor = false;
    int version = 0;
    GameSettings loadedSettings;
    vector<string> boxNames(MAX_BOXES + 1);
    for (int b = 1; b <= MAX_BOXES; ++b)
        boxNames[b] = "Box " + to_string(b);
    vector<SavedPokemon> saved;
    vector<int> counts(MAX_BOXES + 1, 0); // index 0 = party
    set<int> loadedRegistrations;
    set<int> loadedEncounters;

    string line;
    int lineNo = 0;
    auto fail = [&](const string &why) {
        reportError("Save file is invalid (line " + to_string(lineNo) + ": " +
                    why + "). Nothing was changed.");
        return false;
    };

    while (getline(in, line))
    {
        ++lineNo;
        if (line.empty() || line == "\r")
            continue;

        vector<string> t = parseCSVLine(line);
        const string &tag = t[0];

        if (tag == "VERSION")
        {
            if (t.size() != 2 || !toInt(t[1], version) ||
                (version < 1 || version > SAVE_VERSION))
                return fail("unsupported version");
            sawVersion = true;
        }
        else if (!sawVersion)
        {
            return fail("missing VERSION header");
        }
        else if (tag == "CURRENT_BOX")
        {
            if (t.size() != 2 || !toInt(t[1], currentBox) ||
                currentBox < 1 || currentBox > MAX_BOXES)
                return fail("bad CURRENT_BOX");
        }
        else if (tag == "BOXNAME")
        {
            int b = 0;
            if (t.size() != 3 || !toInt(t[1], b) || b < 1 || b > MAX_BOXES || t[2].empty())
                return fail("bad BOXNAME");
            boxNames[b] = t[2];
        }
        else if (tag == "SETTINGS")
        {
            int sprites = 0, sound = 0, typeTags = 0, developer = 0;
            int confirmRelease = 1, confirmOverwrite = 1;
            const size_t expectedFields = version >= 9 ? 8 : version >= 8 ? 7 : 5;
            int textSpeed = static_cast<int>(TextSpeed::Medium);
            if (version < 5 || sawSettings || t.size() != expectedFields ||
                !toInt(t[1], sprites) || !toInt(t[2], sound) ||
                !toInt(t[3], typeTags) || !toInt(t[4], developer) ||
                (sprites != 0 && sprites != 1) ||
                (sound != 0 && sound != 1) ||
                (typeTags != 0 && typeTags != 1) ||
                (developer != 0 && developer != 1) ||
                (version >= 8 &&
                 (!toInt(t[5], confirmRelease) ||
                  !toInt(t[6], confirmOverwrite) ||
                  (confirmRelease != 0 && confirmRelease != 1) ||
                  (confirmOverwrite != 0 && confirmOverwrite != 1))) ||
                (version >= 9 &&
                 (!toInt(t[7], textSpeed) || textSpeed < 0 || textSpeed > 2)))
                return fail("bad SETTINGS record");
            loadedSettings.spritesEnabled = sprites != 0;
            loadedSettings.soundEnabled = sound != 0;
            loadedSettings.typeTagsEnabled = typeTags != 0;
            loadedSettings.developerMode = developer != 0;
            loadedSettings.confirmRelease = confirmRelease != 0;
            loadedSettings.confirmOverwrite = confirmOverwrite != 0;
            loadedSettings.textSpeed = static_cast<TextSpeed>(textSpeed);
            sawSettings = true;
        }
        else if (tag == "SAVED_AT")
        {
            if (version < 6 || sawTimestamp || t.size() != 2 || t[1].empty())
                return fail("bad SAVED_AT record");
            sawTimestamp = true;
        }
        else if (tag == "POKEDEX_CURSOR")
        {
            int dexNumber = 0;
            if (version < 7 || sawPokedexCursor || t.size() != 2 ||
                !toInt(t[1], dexNumber) || dexNumber < 1 || dexNumber > 1025)
                return fail("bad POKEDEX_CURSOR record");
            loadedSettings.lastPokedexEntry = dexNumber;
            sawPokedexCursor = true;
        }
        else if (tag == "POKEDEX")
        {
            int id = 0;
            if (version < 3 || t.size() != 2 || !toInt(t[1], id) ||
                findByInternalID(pokedex, id) == nullptr)
                return fail("bad POKEDEX record");
            loadedRegistrations.insert(id);
            loadedEncounters.insert(id);
        }
        else if (tag == "ENCOUNTERED")
        {
            int id = 0;
            if (version < 4 || t.size() != 2 || !toInt(t[1], id) ||
                findByInternalID(pokedex, id) == nullptr)
                return fail("bad ENCOUNTERED record");
            loadedEncounters.insert(id);
        }
        else if (tag == "POKEMON")
        {
            int loc = 0, dex = 0, level = 0, shiny = 0;
            if (t.size() != 10 || !toInt(t[1], loc) || !toInt(t[2], dex) ||
                !toInt(t[5], level) || !toInt(t[8], shiny))
                return fail("bad POKEMON record");
            if (loc < 0 || loc > MAX_BOXES)
                return fail("bad location");
            if (shiny != 0 && shiny != 1)
                return fail("bad shiny flag");

            const SpeciesData *species = (version == 1) ? getSpeciesByPokedexNumber(pokedex, dex)
                                                         : findByInternalID(pokedex, dex);
            if (species == nullptr)
                return fail("unknown Dex number " + to_string(dex));

            if (++counts[loc] > (loc == 0 ? maxPartySize : BOX_CAPACITY))
                return fail(loc == 0 ? "too many Pokémon in the party" : "box over capacity");

            saved.push_back({loc, species, t[3], t[4], level, t[6], t[7], shiny == 1, t[9]});
            loadedRegistrations.insert(species->internalID);
            loadedEncounters.insert(species->internalID);
        }
        else
        {
            return fail("unknown record \"" + tag + "\"");
        }
    }

    if (!sawVersion)
        return fail("empty file");
    if (version >= 6 && !sawTimestamp)
        return fail("missing SAVED_AT record");
    if (version >= 7 && !sawPokedexCursor)
        return fail("missing POKEDEX_CURSOR record");

    clearList(party);
    for (int b = 1; b <= MAX_BOXES; ++b)
    {
        clearList(pc.getList(b));
        pc.setBoxName(b, boxNames[b]);
    }

    for (const SavedPokemon &p : saved)
    {
        DoublyLinkedList &target = (p.location == 0) ? party : pc.getList(p.location);
        target.append(p.species, p.nickname, p.gender, p.level, p.ability, p.nature,
                      p.isShiny, p.dateCaught);
    }

    pc.setCurrentBox(currentBox);
    registeredSpecies = std::move(loadedRegistrations);
    encounteredSpecies = std::move(loadedEncounters);
    getGameSettings() = loadedSettings;
    return true;
}