#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <execution>
#include <algorithm>
#include <random>

#include "document.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "test_example_functions.h"
#include "log_duration.h"

using std::cerr;
using std::string_view;
using std::string;
using std::vector;
using std::endl;
using std::ostream;
using namespace std::string_view_literals;
using namespace std::string_literals;

#define RUN_TEST(func) RunTestImpl((func), (#func)) // напишите недостающий код

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, string_view expr_str, string_view file, string_view func, unsigned line,
    string_view hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

//#define BECNHMARK(policy) BenchmarkMatchDocumentsTimeCount(#policy, search_server, query, execution::policy)
//
//string GenerateWord(std::mt19937& generator, int max_length) {
//    using namespace std;
//    const int length = uniform_int_distribution(1, max_length)(generator);
//    string word;
//    word.reserve(length);
//    for (int i = 0; i < length; ++i) {
//        word.push_back(uniform_int_distribution(0, 26)(generator) + 'a');
//    }
//    return word;
//}
//
//vector<string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length) {
//    using namespace std;
//    vector<string> words;
//    words.reserve(word_count);
//    for (int i = 0; i < word_count; ++i) {
//        words.push_back(GenerateWord(generator, max_length));
//    }
//    std::sort(words.begin(), words.end());
//    words.erase(unique(words.begin(), words.end()), words.end());
//    return words;
//}
//
//string GenerateQuery(std::mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
//    using namespace std; 
//    string query;
//    for (int i = 0; i < word_count; ++i) {
//        if (!query.empty()) {
//            query.push_back(' ');
//        }
//        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
//            query.push_back('-');
//        }
//        query += dictionary[uniform_int_distribution<int>(0, (int)dictionary.size() - 1)(generator)];
//    }
//    return query;
//}
//
//vector<string> GenerateQueries(std::mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
//    using namespace std; 
//    vector<string> queries;
//    queries.reserve(query_count);
//    for (int i = 0; i < query_count; ++i) {
//        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
//    }
//    return queries;
//}

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string_view content = "cat in the city"sv;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"sv);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"sv);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"sv).empty(),
            "Stop words must be excluded from documents"s);
    }
}

// Добавление документов
void TestAddDocument() {
    const int doc_id = 42;
    const string_view content = "black cat with collar in the city "sv;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server(""s);
    ASSERT_HINT(!server.GetDocumentCount(), "The server is not empty, but it should be empty."s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    vector<Document> find_document = server.FindTopDocuments("cat in the city"sv, DocumentStatus::ACTUAL);

    ASSERT_HINT(!find_document.empty(), "Documents have not were added."s);
    ASSERT_EQUAL(find_document[0].id, doc_id);
    ASSERT_EQUAL(find_document[0].rating, accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
    // it makes no meaning to count the relevance for 1 document here, 
    // ln(1) = 0 [inverse_document_freq = log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size())]
    ASSERT_EQUAL(find_document[0].relevance, 0);
}

//Поддержка минус слов, стоп слов и матчинг
void TestMinusStopWordsSupportAndMatch() {
    const int doc_id = 42;
    const string_view content = "black cat with collar in the city"sv;
    const vector<int> ratings = { 1, 2, 3 };
    const int int_actual_status = static_cast<int>(DocumentStatus::ACTUAL);

    {
        SearchServer server(""sv);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        // collar - minus word test this document
        const auto [query_first, document_status_first] = server.MatchDocument("cat in the city for sale -sale -lang -collar"sv, doc_id);

        vector<string_view> query_true_first = {};
        ASSERT_EQUAL_HINT(query_first, query_true_first, "Document with minus words are not erase."s);
        const int int_first_status = static_cast<int>(document_status_first);
        ASSERT_EQUAL(int_first_status, int_actual_status);
    }

    {
        SearchServer search_server("in the with"sv);
        search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [query_second, document_status_second] = search_server.MatchDocument("cat in the city for sale -sale -lang"sv, doc_id);

        vector<string_view> query_true_second = { "cat"sv, "city"sv };
        ASSERT_EQUAL_HINT(query_second, query_true_second, "Stop word are not erase from the document."s);
        const int int_second_status = static_cast<int>(document_status_second);
        ASSERT_EQUAL(int_second_status, int_actual_status);
    }
}

//Сортировка документов по релевантности и вычисление релевантности и рейтинга
void TestSortAndComputingRelevanceRatingDocuments() {

    SearchServer search_server(""s);
    // Сan be change documents_ and vector ratings - sort, computing relevance, rating will be performed.
    const map<int, string_view> documents_ = { {0, {"белый кот и модный ошейник"sv}}, {1, {"пушистый кот пушистый хвост"sv}},
                                    {2, {"ухоженный пёс выразительные глаза"sv}}, {3, {"ухоженный скворец евгений"sv}} };
    search_server.AddDocument(0, documents_.at(0), DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, documents_.at(1), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, documents_.at(2), DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, documents_.at(3), DocumentStatus::BANNED, { 9 });

    // Relevance computing
    map<string_view, map<int, double>> word_to_document_freqs_;

    for (const auto& [id, document] : documents_) {
        const vector<string_view> words = SplitIntoWords(document);
        const double inv_word_count = 1.0 / words.size();

        for (const string_view& word : words) {
            word_to_document_freqs_[word][id] += inv_word_count;
        }
    }

    map<int, double> document_to_relevance;

    for (const string_view& word : SplitIntoWords("пушистый ухоженный кот"sv)) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = log(search_server.GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    auto ComputeRating = [](const vector<int> ratings)
    { return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()); };

    // Change arg func ComputeRating
    vector<Document> answerT = { {0, document_to_relevance.at(0), ComputeRating({8, -3})},
                                {1, document_to_relevance.at(1), ComputeRating({7, 2, 7})},
                                {2, document_to_relevance.at(2), ComputeRating({5, -12, 2, 1})} };

    std::sort(answerT.begin(), answerT.end(),
        [](const Document& lhs, const Document& rhs) {
            const double EPSILON = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });

    const vector<Document> anserFind = search_server.FindTopDocuments("пушистый ухоженный кот"s);

    ASSERT_EQUAL(answerT.size(), answerT.size());
    const double epsilon = 1e-6;
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_EQUAL_HINT(anserFind[i].id, answerT[i].id, "Sort by relevance perform not correctly."s);
    }

    // Почему нужно проверять вычисление рейтинга и релевантности в отдельной функции, а не сделать это здесь???
    // if sort its ok, then can be check the calculation relevance and rating
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_HINT(std::abs(answerT[i].relevance - anserFind[i].relevance) < epsilon, "Relevance calculated not correctly."s);
        ASSERT_EQUAL_HINT(answerT[i].rating, anserFind[i].rating, "Rating calculated not correctly."s);
    }
}

// Проверка поиска документов, имеющие заданный статус
void TestFindDocumentStatus() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::REMOVED, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    int id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::REMOVED).front().id;
    ASSERT_HINT(id == 0, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::IRRELEVANT).front().id;
    ASSERT_HINT(id == 1, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::ACTUAL).front().id;
    ASSERT_HINT(id == 2, "Find documents by statuss perform wrong."s);

    ASSERT_HINT(search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::BANNED).empty(),
        "Find documents by statuss perform wrong."s);
}

// Тест использования предиката, задаваемого пользователем
void TestPredicateFunc() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::REMOVED, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    int id = search_server.FindTopDocuments("ухоженный кот"s,
        [](int document_id, DocumentStatus status, int rating) { return rating > 8; }).front().id;
    ASSERT_HINT(id == 3, "Function-predicate using not correctly by rating condition."s);

    id = search_server.FindTopDocuments("ухоженный кот"s,
        [](int document_id, DocumentStatus status, int rating) { return document_id == 1; }).front().id;
    ASSERT_HINT(id == 1, "Function-predicate using not correctly by document id condition."s);
}

//void BenchmarkMatchDocuments()
//{
//    using namespace std;
//    mt19937 generator;
//
//    const auto dictionary = GenerateDictionary(generator, 1000, 10);
//    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
//
//    const string query = GenerateQuery(generator, dictionary, 500, 0.1);
//
//    SearchServer search_server(dictionary[0]);
//    for (size_t i = 0; i < documents.size(); ++i) {
//        search_server.AddDocument((int)i, documents[i], DocumentStatus::ACTUAL, { 1, 2, 3 });
//    }
//
//    BECNHMARK(seq);
//    BECNHMARK(par);
//}
//
//void BenchmarkRemoveDocument()
//{
//    using namespace std;
//    SearchServer search_server("and with"s);
//
//    int id = 0;
//    for (
//        const string& text : {
//            "funny pet and nasty rat"s,
//            "funny pet with curly hair"s,
//            "funny pet and not very nasty rat"s,
//            "pet with rat and rat and rat"s,
//            "nasty rat with curly hair"s,
//        }
//        ) {
//        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
//    }
//
//    const string query = "curly and funny"s;
//    auto report = [&search_server, &query] {
//        cout << search_server.GetDocumentCount() << " documents total, "s
//            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
//    };
//
//    report();
//    search_server.RemoveDocument(5);
//    report();
//    search_server.RemoveDocument(execution::seq, 1);
//    report();
//    search_server.RemoveDocument(execution::par, 2);
//    report();
//}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusStopWordsSupportAndMatch);
    RUN_TEST(TestSortAndComputingRelevanceRatingDocuments);
    RUN_TEST(TestFindDocumentStatus);
    RUN_TEST(TestPredicateFunc);
}

//void BenchmarkSearchServer() {
//    BenchmarkMatchDocuments();
//    BenchmarkRemoveDocument();
//}
// --------- Окончание модульных тестов поисковой системы -----------

