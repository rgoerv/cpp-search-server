#include "search_server.h"
#include "log_duration.h"
#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace Generator {

std::string GenerateWord(std::mt19937& generator, int max_length);

std::vector<std::string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length);

std::string GenerateQuery(std::mt19937& generator, const std::vector<std::string>& dictionary, 
                                                                int word_count, double minus_prob = 0);

std::vector<std::string> GenerateQueries(std::mt19937& generator, const std::vector<std::string>& dictionary, 
                                                                int query_count, int max_word_count);
} // namespace Generator

