#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <filesystem>
#include "client.cpp"
using namespace std;
string WHITESPACE = "\n";
    string ltrim(const string &s) {
        size_t start = s.find_first_not_of(WHITESPACE);
        return (start == std::string::npos) ? "" : s.substr(start);
    }
 
    string rtrim(const string &s)
    {
        size_t end = s.find_last_not_of(WHITESPACE);
        return (end == std::string::npos) ? "" : s.substr(0, end + 1);
    }

vector<string> Manager(FILE* file){

    char* c_line = nullptr;
    size_t len = 0;
    vector<string> lines;
    string x;

    while ((getline(&c_line, &len, file)) != -1) {
        //lines.push_back(x.assign(c_line));
        x = x.assign(c_line);
        // if(x.length() != 0 || x[0] != ' '){
        //     x = rtrim(ltrim(x));  
        //     lines.push_back(x);
        // }
        lines.push_back(x);
    }
    cout << "The Lines That Contain the Target Words are " << endl;
    vector<string> res = sendPacketData(lines);
    for(string x: res){
        if(x.length() != 0 || x[0] != ' '){
            x = rtrim(x);
            cout << x << endl;
        }
    }
    return res;
}
struct CommandLineException {
    CommandLineException(int m,int a)
    {   cout << endl;
        cout << "Too many file names on the command line.";
        cout << endl;
        cout << "A maximum of " << m << " file names can appear on the command line.";
        cout << endl;
        cout << a << " file names were entered.";
        cout << endl;
        cout << "p01 (<input file name> (<output file name>))";
    }
};

struct FileException {
    FileException(const char* fn)
    { cout << endl;
    cout << "File " << fn << " cannot be opened.";
    cout << endl;
}
};

int main(int argc, char* argv[])
{   try {
        char ifn[255];
        switch (argc) {
            case 1://no files on the command line
                cout << "Enter the file path";
                cin >> ifn;
                cout << endl;
            break;
            case 2://input file on the command line/prompt for output file
                strcpy(ifn,argv[1]);
            break;
            default:
                throw CommandLineException(2,argc-1);
                break;
        }
        string x;
        //ifstream file;
        std::string filePath= std::string(ifn);
        //std::string const HOME = std::getenv("HOME") ? std::getenv("HOME") : ".";
        //std::ifstream file(HOME + "/vitalis/Documents/op/test.dat");
        cout << filePath << endl;
        //file.open(filePath, ifstream::in);
        FILE* file = fopen(filePath.c_str(), "r");
        if (!file) throw FileException(ifn);
        vector<string> res = Manager(file);
        fclose(file);
        //file.close();
    } catch ( ... ) {
        cout << endl;
        cout << "Program Terminated!";
        cout << endl;
        cout << "I won't be back!";
        cout << endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}
