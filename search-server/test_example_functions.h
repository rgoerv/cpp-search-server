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
void RunTestImpl(_T T, const string& T_str) {
    /* Напишите недостающий код */
    T();
    cerr << T_str << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func), (#func)) // напишите недостающий код

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
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

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint);

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

