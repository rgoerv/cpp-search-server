#include <vector>
#include <string_view>

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;

    size_t space = str.find_first_not_of(' ');
    size_t after_space = str.find_first_of(' ', space);

    while (space != str.npos) {
        result.push_back(str.substr(space, after_space - space));
        str.remove_prefix(std::min(str.size(), after_space));
        space = str.find_first_not_of(' ');
        after_space = str.find_first_of(' ', space);
    }
    return result;
}