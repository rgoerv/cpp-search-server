// Обьявления функций обработки строк вне класса SearchServer
#pragma once
#include <vector>
#include <string>
#include <set>

using std::vector;
using std::string;
using std::set;

vector<string> SplitIntoWords(const string& text);

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}