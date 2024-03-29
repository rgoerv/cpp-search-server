#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <tuple>
#include <deque>
#include <execution>
#include <numeric>
#include <mutex>
#include <future>
#include <iterator>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "process_queries.h"
#include "concurrent_map.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

void AddDocument(SearchServer& search_server, int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        const auto view_word = ValidateView(word);
        word_to_document_freqs_[view_word][document_id] += inv_word_count;
        id_to_words_freqs_[document_id][view_word] = 0;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const static std::map<std::string_view, double> empty;
    if (id_to_words_freqs_.count(document_id)) {
        return id_to_words_freqs_.at(document_id);
    }
    return empty;
}

void SearchServer::RemoveDocument(int document_id) {
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query, 1);

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { std::vector<std::string_view>{}, documents_.at(document_id).status };
        }
    }

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
    std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
    std::string_view raw_query, int document_id) const {

    const auto query = ParseQuery(raw_query);

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](const std::string_view& word) {
            return (word_to_document_freqs_.count(word) != 0) &&
                (word_to_document_freqs_.at(word).count(document_id) > 0);
        })) {
        return { std::vector<std::string_view>{}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());

    auto last_copy = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        [this, document_id](const std::string_view& word) {
            return (word_to_document_freqs_.count(word) != 0) &&
                (word_to_document_freqs_.at(word).count(document_id) > 0);
        });

    std::sort(matched_words.begin(), last_copy, [](const auto& lhs, const auto& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        });
    auto last = std::unique(matched_words.begin(), last_copy);
    matched_words.erase(last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}


std::string_view SearchServer::ValidateView(std::string_view word) {
    heap_strings_.push_back(static_cast<std::string>(word));
    return heap_strings_.back();
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + static_cast<std::string>(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + (std::string)text + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

void SearchServer::EraseUnique(SearchServer::Query& query) const {
    std::sort(query.minus_words.begin(), query.minus_words.end(), [](const auto& lhs, const auto& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        });
    auto last_minus = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(last_minus, query.minus_words.end());

    std::sort(query.plus_words.begin(), query.plus_words.end(), [](const auto& lhs, const auto& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        });
    auto last_plus = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(last_plus, query.plus_words.end());
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool there_duplicates) const {
    Query result;
    for (const auto& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (there_duplicates) {
        EraseUnique(result);
    }
    return result;
}