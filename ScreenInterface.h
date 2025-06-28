#pragma once
#include "Main.h"

class ScreenInterface
{
    static const int SCREENSIZE = 53;

public:
    void printBorder(std::string pos = "mid")
    {
        std::cout << (pos == "top" ? "╔" : (pos == "bottom" ? "╚" : "╠"));
        for (int i = 0; i < SCREENSIZE; i++)
        {
            std::cout << "═";
        }
        std::cout << (pos == "top" ? "╗" : (pos == "bottom" ? "╝" : "╣")) << std::endl;
    }


    std::string stripAnsiCodes(const std::string &str)
    {
        return std::regex_replace(str, std::regex("\033\\[[0-9;]*m"), "");
    }

    void printMessage(std::string message = "")
    {

        std::string plainText = message;
        message = stripAnsiCodes(message);
        int spaceLen = SCREENSIZE - message.length();

        std::cout << "║ ";
        for (int i = 0; i < spaceLen; i++)
        {

            std::cout << (i == 2 ? plainText : " ");
        }
        std::cout << "║" << std::endl;
    }
};
