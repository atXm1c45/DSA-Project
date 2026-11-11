#include "../include/TitleScreen.h"
#include "../include/ConsoleUtils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#ifdef _WIN32
#include <conio.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

using namespace std;

namespace
{
    constexpr int REVEAL_DELAY_MS = 100;

    const string FALLBACK_ART = R"ART(
ooooooooo.     .oooooo.   oooo    oooo             .oooooo.   ooooo        ooooo 
`888   `Y88.  d8P'  `Y8b  `888   .8P'             d8P'  `Y8b  `888'        `888' 
 888   .d88' 888      888  888  d8'     .ooooo.  888           888          888  
 888ooo88P'  888      888  88888[      d88' `88b 888           888          888  
 888         888      888  888`88b.    888ooo888 888           888          888  
 888         `88b    d88'  888  `88b.  888    .o `88b    ooo   888       o  888  
o888o         `Y8bood8P'  o888o  o888o `Y8bod8P'  `Y8bood8P'  o888ooooood8 o888o 
)ART";

    const string FALLBACK_COLOR = "\033[93m";

    void revealRow(const string &row, bool &skip)
    {
        cout << row << '\n'
             << flush;
#ifdef _WIN32
        if (!skip && _kbhit())
        {
            _getch();
            skip = true;
        }
#endif
        if (!skip && REVEAL_DELAY_MS > 0)
            this_thread::sleep_for(chrono::milliseconds(REVEAL_DELAY_MS));
    }

    void revealFallback(bool &skip)
    {
        istringstream stream(FALLBACK_ART);
        string line;
        while (getline(stream, line))
            revealRow(FALLBACK_COLOR + line + "\033[0m", skip);
    }
}

void displayTitleScreen()
{
    clearScreen();

    constexpr int targetWidth = 120;
    const string ramp = " .,:;i1tfLCG08@";
    bool skip = false;

    int width = 0, height = 0, channels = 0;
    unsigned char *image =
        stbi_load("assets/logo.png", &width, &height, &channels, 4);

    if (image == nullptr || width <= 0 || height <= 0)
    {
        if (image != nullptr)
            stbi_image_free(image);
        revealFallback(skip);
    }
    else
    {
        const double cellW = static_cast<double>(width) / targetWidth;
        const double cellH = cellW * 2.0;
        const int rows = max(1, static_cast<int>(height / cellH));

        for (int row = 0; row < rows; ++row)
        {
            const int y0 = static_cast<int>(row * cellH);
            const int y1 = min(height, max(y0 + 1, static_cast<int>((row + 1) * cellH)));

            ostringstream line;

            for (int col = 0; col < targetWidth; ++col)
            {
                const int x0 = static_cast<int>(col * cellW);
                const int x1 = min(width, max(x0 + 1, static_cast<int>((col + 1) * cellW)));

                double sumR = 0, sumG = 0, sumB = 0, sumA = 0;
                int count = 0;
                for (int y = y0; y < y1; ++y)
                {
                    for (int x = x0; x < x1; ++x)
                    {
                        const unsigned char *p =
                            image + (static_cast<size_t>(y) * width + x) * 4;
                        sumR += p[0] * p[3];
                        sumG += p[1] * p[3];
                        sumB += p[2] * p[3];
                        sumA += p[3];
                        ++count;
                    }
                }

                const double avgA = count ? sumA / count : 0;
                if (avgA < 60 || sumA == 0)
                {
                    line << ' ';
                    continue;
                }

                const int r = min(255, static_cast<int>(sumR / sumA * 1.2));
                const int g = min(255, static_cast<int>(sumG / sumA * 1.2));
                const int b = min(255, static_cast<int>(sumB / sumA * 1.2));

                const double lum = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;
                const double density = pow(lum, 0.8) * (avgA / 255.0);
                const int idx = min(static_cast<int>(ramp.size()) - 1,
                                    static_cast<int>(density * ramp.size()));

                line << "\033[38;2;" << r << ';' << g << ';' << b << 'm'
                     << ramp[idx];
            }
            line << "\033[0m";
            revealRow(line.str(), skip);
        }
        stbi_image_free(image);
    }

    const string pad(targetWidth / 2 - 6, ' ');
    cout << "\n"
         << pad << "1. Play\n";
    cout << pad << "0. Exit\n";
}