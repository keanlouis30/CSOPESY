#include <iostream>
#include <string>
#include <vector>

using namespace std;

vector<string> createBanner(const string& text) {
    vector<string> banner;
    string line1, line2, line3;

    line1 += " /";
    line2 += "| ";
    line3 += " \\";
    for (char c : text) {
        line1 += "-";
        line2 += c;
        line3 += "-";
    }
    line1 += "\\";
    line2 += "|";
    line3 += "/";

    banner.push_back(line1);
    banner.push_back(line2);
    banner.push_back(line3);  // Fixed typo here

    return banner;
}

int main() {
    string text;
    cout << "Enter text for the banner: ";
    getline(cin, text);

    vector<string> banner = createBanner(text);

    for (const string& line : banner) {
        cout << line << endl;
    }

    return 0;
}
