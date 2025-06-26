class Console
{

private:
    ScreenInterface SI = ScreenInterface();

public:
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;

    Console() : name(""), currentLine(0), totalLines(0)
    {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    Console(const std::string &n, int total) : name(n), currentLine(0), totalLines(total)
    {
        time_t now = time(nullptr);
        char buf[100];
        strftime(buf, sizeof(buf), "%m/%d/%Y, %I:%M:%S %p", localtime(&now));
        timestamp = buf;
    }

    void display()
    {
        std::string input;
        while (true)
        {
            system("cls"); // clear screen
            SI.printBorder("top");

            SI.printMessage("\033[33mScreen: " + name + "\033[0m");
            SI.printMessage("\033[32mProcess: " + name + "\033[0m");
            SI.printMessage("\033[32mInstruction: Line " + std::to_string(currentLine + 1) + " / " + std::to_string(totalLines) + "\033[0m");
            SI.printMessage("\033[32mCreated at: " + timestamp + "\033[0m");
            SI.printMessage();
            SI.printMessage();
            SI.printMessage();

            SI.printBorder("bottom");

            std::cout << "\nType 'exit' to stop the process.\n";
            std::cout << "\033[33mScreen> \033[0m";

            std::getline(std::cin, input);
            if (input == "exit")
                break;
            if (currentLine < totalLines - 1)
                currentLine++;
        }
    }
};
