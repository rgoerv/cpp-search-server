#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <set>
#include <map>

#include "document.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"

using std::cerr;
using std::string;
using std::string_view;
using std::vector;
using std::endl;
using std::ostream;
using std::set;
using std::map;

template<typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    bool is_first = true;
    out << "[";
    for (const auto& element : container) {
        if (!is_first) {
            out << ", ";
        }
        is_first = false;
        out << element;
    }
    out << "]";
    return out;
}

template<typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    bool is_first = true;
    out << "{";
    for (const auto& element : container) {
        if (!is_first) {
            out << ", ";
        }
        is_first = false;
        out << element;
    }
    out << "}";
    return out;
}

template<typename T1, typename T2>
ostream& operator<<(ostream& out, const map<T1, T2>& container) {
    bool is_first = true;
    out << "{";
    for (const auto& [key, value] : container) {
        if (!is_first) {
            out << ", ";
        }
        is_first = false;
        out << key << ": " << value;
    }
    out << "}";
    return out;
}

template <typename _T>
void RunTestImpl(_T T, string_view T_str) {
    T();
    cerr << T_str << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), (#func))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, string_view t_str, string_view u_str, string_view file,
    string_view func, unsigned line, string_view hint) {
    if (t != u) {
        cerr << std::boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, string_view expr_str, string_view file, string_view func, unsigned line,
    string_view hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


// -------- Начало модульных тестов поисковой системы ----------
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();
// Добавление документов
void TestAddDocument();
//Поддержка минус слов, стоп слов и матчинг
void TestMinusStopWordsSupportAndMatch();
//Сортировка документов по релевантности и вычисление релевантности и рейтинга
void TestSortAndComputingRelevanceRatingDocuments();
// Проверка поиска документов, имеющие заданный статус
void TestFindDocumentStatus();
// Тест использования предиката, задаваемого пользователем
void TestPredicateFunc();
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();
// --------- Окончание модульных тестов поисковой системы -----------
// --------------------------- Benchmarks ---------------------------
//void BenchmarkMatchDocuments();
//void BenchmarkRemoveDocument();
//void BenchmarkSearchServer();
//
//template <typename ExecutionPolicy>
//void BenchmarkMatchDocumentsTimeCount(string_view mark, SearchServer search_server, const string& query,
//    ExecutionPolicy&& policy) {
//    using namespace std;
//    LOG_DURATION(mark);
//    const int document_count = search_server.GetDocumentCount();
//    int word_count = 0;
//    for (int id = 0; id < document_count; ++id) {
//        const auto [words, status] = search_server.MatchDocument(policy, query, id);
//        word_count += (int)words.size();
//    }
//    cout << word_count << endl;
//}
// ------------------------- End Benchmarks -------------------------
