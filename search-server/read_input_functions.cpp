#include <string>
#include <iostream>

#include "read_input_functions.h"

using std::string;
using std::cin;
using std::ostream;
using std::istream;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}