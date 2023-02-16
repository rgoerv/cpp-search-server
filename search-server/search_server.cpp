#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"

using std::vector;
using std::pair;
using std::map;
using std::set;
using namespace std::string_literals;
using std::invalid_argument;

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_to_words_freqs_[document_id][word] = 0;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.push_back(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

vector<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

vector<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const map<string, double> & SearchServer::GetWordFrequencies(int document_id) const {
    map<string, double> word_freqs_ = {};
    if (id_to_words_freqs_.count(document_id)) {
        return id_to_words_freqs_.at(document_id); 
    }
    return empty;
}

void SearchServer::RemoveDocument(int document_id) {

    // словарь id -> cловарь слов и ее частоты
    id_to_words_freqs_.erase(document_id);

    // очистка словаря слово -> id и его частота 
    vector<string> word_to_erase_ids = {};
    
    for (auto& [word, id_freqs] : word_to_document_freqs_) {
        
        id_freqs.erase(document_id);

        if (id_freqs.empty()) {
            word_to_erase_ids.push_back(word);
        }
    }
    
    for (const auto& word : word_to_erase_ids) {
        word_to_document_freqs_.erase(word);
    }

    // очистка словаря id -> rating и status документа
    if (documents_.count(document_id)) {
        documents_.erase(document_id);
    }
    // удаление id из вектора документов
    document_ids_.erase(remove(document_ids_.begin(), document_ids_.end(), document_id));
}


std::tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query);

    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}


SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query result;
    for (const string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
