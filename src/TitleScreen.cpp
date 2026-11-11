#include "../include/TitleScreen.h"
#include "../include/ConsoleUtils.h"
#include <algorithm>
#include <iostream>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

using namespace std;

namespace
{
void printPixel(const unsigned char *top, const unsigned char *bottom)
{
    const bool topVisible = top[3] >= 128;
    const bool bottomVisible = bottom[3] >= 128;
    if (!topVisible && !bottomVisible)
    {
        cout << ' ';
        return;
    }

    if (topVisible)
    {
        cout << "\033[38;2;" << static_cast<int>(top[0]) << ';'
             << static_cast<int>(top[1]) << ';' << static_cast<int>(top[2])
             << 'm';
    }
    if (bottomVisible)
    {
        cout << "\033[48;2;" << static_cast<int>(bottom[0]) << ';'
             << static_cast<int>(bottom[1]) << ';' << static_cast<int>(bottom[2])
             << 'm';
    }
    else
    {
        cout << "\033[49m";
    }

    cout << (topVisible ? "▀" : "▄") << "\033[0m";
}
}

void displayTitleScreen()
{
    clearScreen();

    constexpr int targetWidth = 76;
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *image =
        stbi_load("assets/logo.png", &width, &height, &channels, 4);
    if (image == nullptr || width <= 0 || height <= 0)
    {
        if (image != nullptr)
            stbi_image_free(image);
        cout << "\033[94m"
             << "  ____   ___  _  _______ ____ _     ___ \n"
             << " |  _ \\ / _ \\| |/ / ____/ ___| |   |_ _|\n"
             << " | |_) | | | | ' /|  _|| |   | |    | | \n"
             << " |  __/| |_| | . \\| |__| |___| |___ | | \n"
             << " |_|    \\___/|_|\\_\\_____\\____|_____|___|\n"
             << "\033[0m";
    }
    else
    {
        const int targetPixelHeight = max(
            1, static_cast<int>(targetWidth * height /
                                static_cast<double>(width) + 0.5));
        const int targetRowCount = (targetPixelHeight + 1) / 2;
        const int leftPadding = max(0, (80 - targetWidth) / 2);

        for (int row = 0; row < targetRowCount; ++row)
        {
            cout << string(static_cast<size_t>(leftPadding), ' ');
            for (int column = 0; column < targetWidth; ++column)
            {
                const int sourceX = min(
                    width - 1, (column * width + width / 2) / targetWidth);
                const int targetTopY = row * 2;
                const int targetBottomY = targetTopY + 1;
                const int topY = min(
                    height - 1,
                    (targetTopY * height + targetPixelHeight / 2) /
                        targetPixelHeight);
                const int bottomY = min(
                    height - 1,
                    (targetBottomY * height + targetPixelHeight / 2) /
                        targetPixelHeight);
                const size_t topIndex =
                    (static_cast<size_t>(topY) * width + sourceX) * 4;
                const size_t bottomIndex =
                    (static_cast<size_t>(bottomY) * width + sourceX) * 4;
                printPixel(image + topIndex, image + bottomIndex);
            }
            cout << "\033[0m\n";
        }
        stbi_image_free(image);
    }

    cout << "\n                         1. Play\n";
    cout << "                         0. Exit\n";
}
