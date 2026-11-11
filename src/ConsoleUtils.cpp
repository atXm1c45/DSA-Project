#include "../include/ConsoleUtils.h"
#include "../include/GameSettings.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

namespace
{
int typewriterDelayMilliseconds()
{
    switch (getGameSettings().textSpeed)
    {
    case TextSpeed::Slow:
        return 55;
    case TextSpeed::Fast:
        return 8;
    case TextSpeed::Medium:
    default:
        return 25;
    }
}

size_t utf8CharacterLength(unsigned char firstByte)
{
    if ((firstByte & 0x80) == 0)
        return 1;
    if ((firstByte & 0xE0) == 0xC0)
        return 2;
    if ((firstByte & 0xF0) == 0xE0)
        return 3;
    if ((firstByte & 0xF8) == 0xF0)
        return 4;
    return 1;
}

bool isAnsiSequenceFinal(unsigned char character)
{
    return character >= 0x40 && character <= 0x7E;
}
}

namespace ConsoleUtils
{
void printTypewriter(const string &text)
{
    const int delayMs = typewriterDelayMilliseconds();
    for (size_t position = 0; position < text.size();)
    {
        if (static_cast<unsigned char>(text[position]) == 0x1B &&
            position + 1 < text.size() && text[position + 1] == '[')
        {
            size_t end = position + 2;
            while (end < text.size() &&
                   !isAnsiSequenceFinal(static_cast<unsigned char>(text[end])))
                ++end;
            if (end < text.size())
            {
                cout.write(text.data() + position,
                           static_cast<streamsize>(end - position + 1));
                position = end + 1;
                continue;
            }
        }

        const unsigned char firstByte =
            static_cast<unsigned char>(text[position]);
        const size_t characterLength =
            std::min(utf8CharacterLength(firstByte), text.size() - position);
        cout.write(text.data() + position,
                   static_cast<streamsize>(characterLength))
            << flush;
        position += characterLength;
        if (firstByte != '\n' && firstByte != '\r')
            this_thread::sleep_for(chrono::milliseconds(delayMs));
    }
    this_thread::sleep_for(chrono::seconds(1));
}

void printTypewriterLine(const string &text)
{
    printTypewriter(text);
    cout << endl;
}
}
