#pragma once
#include <algorithm>
#include <stdexcept>
#include <numeric>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <tuple>
#include <deque>
#include <execution>
#include <future>
#include <mutex>
#include <type_traits>

class SearchServer;

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "process_queries.h" // Here used object of class SearchServer, need class declaration
#include "concurrent_map.h"

#include "log_duration.h"

using std::string;
using std::string_view;

using std::vector;
using std::pair;
using std::map;
using std::set;

using namespace std::string_literals;
using namespace std::string_view_literals;

using std::invalid_argument;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(string_view stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {}

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {}

    void AddDocument(int document_id, string_view document, DocumentStatus status,
        const vector<int>& ratings);

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(string_view raw_query, DocumentPredicate document_predicate) const;
    vector<Document> FindTopDocuments(string_view raw_query, DocumentStatus status) const;
    vector<Document> FindTopDocuments(string_view raw_query) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query,
        DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    vector<Document> FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query) const;

    int GetDocumentCount() const;

    // Методы begin() и end() дают возвращают итераторы к контейнеру id документов поисковой системы
    set<int>::const_iterator begin() const;
    set<int>::const_iterator end() const;

    // Получение частот слов по id документа
    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    // Удаление документа из поискового сервера
    void RemoveDocument(int document_id);
    template<class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    using MatchWordsStatus = std::tuple<vector<string_view>, DocumentStatus>;

    MatchWordsStatus MatchDocument(string_view raw_query, int document_id) const;
    MatchWordsStatus MatchDocument(const std::execution::sequenced_policy&, string_view raw_query,
        int document_id) const;
    MatchWordsStatus MatchDocument(const std::execution::parallel_policy&, string_view raw_query,
        int document_id) const;


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    // Contains string for string_view to don`t invalidate
    std::deque<string> heap_strings_;

    const set<string, std::less<>> stop_words_;
    map<string_view, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;
    map<int, map<string_view, double>> id_to_words_freqs_;

    // Save string_view inside the object to they don't invalidate
    // Return new string_view, which deleted together with class object
    string_view ValidateView(string_view word);

    bool IsStopWord(string_view word) const;

    static bool IsValidWord(string_view word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    vector<string_view> SplitIntoWordsNoStop(string_view text) const;

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        // ratings sum / count_ratings
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string_view text) const;

    struct Query {
        vector<string_view> plus_words;
        vector<string_view> minus_words;
    };

    void EraseUnique(Query& result) const;
    Query ParseQuery(string_view text, bool there_duplicates = 0) const;

    double ComputeWordInverseDocumentFreq(string_view word) const;

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate, typename ExecutionPolicy>
    vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
        DocumentPredicate document_predicate) const;
};

void AddDocument(SearchServer& search_server, int document_id,
    string_view document, DocumentStatus status, const vector<int>& ratings);

template <typename DocumentPredicate>
inline vector<Document> SearchServer::FindTopDocuments(string_view raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, 1);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
inline vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query,
    DocumentPredicate document_predicate) const 
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    }

    const auto query = ParseQuery(raw_query, 1);
    auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

    std::sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            const auto EPSILON = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy>
inline vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query, 
    DocumentStatus status) const 
{
    return FindTopDocuments(policy, raw_query, 
        [status](int document_id, DocumentStatus document_status, int rating) { 
            return document_status == status; 
        });
}

template <typename ExecutionPolicy>
inline vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
inline vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const 
{
    map<int, double> document_to_relevance;

    for (const auto& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
inline vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query,
    DocumentPredicate document_predicate) const
{
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    }

    ConcurrentMap<int, double> document_to_relevance(static_cast<size_t>(std::thread::hardware_concurrency()));

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance, document_predicate](const auto& word) {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance](const auto& word) {
            if (word_to_document_freqs_.count(word) != 0) {
                for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        });

    vector<Document> matched_documents;
    for (const auto& container : document_to_relevance) {
        std::for_each(container.map_part.begin(), container.map_part.end(),
            [this, &matched_documents](const auto& id_to_relevance) {
                matched_documents.push_back({ id_to_relevance.first, id_to_relevance.second, documents_.at(id_to_relevance.first).rating });
            });
    }

    return matched_documents;
}

template<class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    // резервируем место для слов документа, полученных в transfom 
    vector<string_view> word_in_erase_document(id_to_words_freqs_.at(document_id).size());

    // вытаскивем указатели на строки из мапы, чтобы в параллельном алгоритме не возникало проблем
    std::transform(policy, id_to_words_freqs_.at(document_id).begin(),
        id_to_words_freqs_.at(document_id).end(), word_in_erase_document.begin(),
        [](const auto& word_to_freqs) {
            return word_to_freqs.first;
        });

    // удаляем id документов из мапы слов
    std::for_each(policy, word_in_erase_document.begin(), word_in_erase_document.end(),
        [document_id, this](const auto& word) {
            word_to_document_freqs_[word].erase(document_id);
        });

    // словарь id -> cловарь слов и ее частоты
    id_to_words_freqs_.erase(document_id);

    // очистка словаря id -> rating и status документа
    if (documents_.count(document_id)) {
        documents_.erase(document_id);
    }
    // удаление id из вектора документов
    document_ids_.erase(document_ids_.find(document_id));
}