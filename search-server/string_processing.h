#pragma once
#include <vector>
#include <string>
#include <set>

using std::string;

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(static_cast<string>(str));
        }
    }
    return non_empty_strings;
}