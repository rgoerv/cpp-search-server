#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

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

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
            for (const string& word : stop_words_) {
                if(!IsValidWord(word)) {
                    throw invalid_argument("Cтоп слово содержит недопустимые символы!"s);
                }
            }            
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {}

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                                    const vector<int>& ratings) {
        try {
            if(GetDocumentId(document_id) == document_id) {
                throw invalid_argument("Документ с таким id уже существует!"s);
            }
        } catch(const out_of_range & error) {
            if((document_id >= 0)) {
                vector<string> words = SplitIntoWordsNoStop(document);            
                const double inv_word_count = 1.0 / words.size();

                for (const string& word : words) {
                    if(!IsValidWord(word)) {
                        throw invalid_argument("Слово документа содержит недопустимые символы!"s);
                    }
                }
                for (const string& word : words) {
                    word_to_document_freqs_[word][document_id] += inv_word_count;
                }
                documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
            } else {
                throw invalid_argument("Попытка добавить документ с отрицательным id!"s);
            }
        }
    }

    template <typename DocumentPredicate>
    [[nodiscard]] vector<Document> FindTopDocuments(const string& raw_query, 
                                            DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    [[nodiscard]] vector<Document> FindTopDocuments(const string& raw_query, 
                                                            DocumentStatus status) const {                                   
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    [[nodiscard]] vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    [[nodiscard]] int GetDocumentId(int index) const {
        if((index >= 0) && (index < GetDocumentCount())) {
            return index;
        } else {
            throw out_of_range("Выход за пределы списка документов!"s);        
        }
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    [[nodiscard]] tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        
        const Query query = ParseQuery(raw_query);
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
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
            if(text.empty()) {
                throw invalid_argument("Пустое минус слово!"s);
            }
        }
        return QueryWord{text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    const QueryWord query_word_last = ParseQueryWord(query_word.data);        
                    if (query_word_last.is_minus) {
                        throw invalid_argument("Минус слово все еще с минусом!"s);
                    }
                    query.minus_words.insert(query_word.data);
                } else {
                    if(!IsValidWord(query_word.data)) {
                        throw invalid_argument("Слово содержит недопустимые символы!"s);
                    }
                    query.plus_words.insert(query_word.data);
                }
            }
        }        
        return query;        
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

template<typename T> 
void PrintError(const T & error) {
    cerr << "Ошибка! "s << error.what() << endl;
}

int main() {

    SearchServer search_server("и в на"s);
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    cout << "SUCESS"s << endl;
    // repeat
    try {
        search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    } catch(const invalid_argument & error) {
        PrintError(error); 
    } catch(const out_of_range & error) {
        PrintError(error);
    }
    // negative id
    try {
        search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    } catch(const invalid_argument & error) {
        PrintError(error); 
    } catch(const out_of_range & error) {
        PrintError(error);
    }
    // banned symbols    
    try {
        search_server.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, {1, 3, 2});
    } catch(const invalid_argument & error) {
        PrintError(error); 
    } catch(const out_of_range & error) {
        PrintError(error);
    }
    // wrong minus word
    try {
        const auto documents = search_server.FindTopDocuments("--пушистый"s);
        for (const Document& document : documents) {
            PrintDocument(document);
        }
    } catch(const invalid_argument & error) {
        PrintError(error); 
    } catch(const out_of_range & error) {
        PrintError(error);
    }
} 