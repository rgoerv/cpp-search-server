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
#include "generator.h"

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& container) {
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
std::ostream& operator<<(std::ostream& out, const std::set<T>& container) {
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
std::ostream& operator<<(std::ostream& out, const std::map<T1, T2>& container) {
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
void RunTestImpl(_T T, std::string_view T_str) {
    T();
    std::cerr << T_str << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), (#func))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, std::string_view t_str, std::string_view u_str, std::string_view file,
    std::string_view func, unsigned line, std::string_view hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, std::string_view expr_str, std::string_view file, std::string_view func, unsigned line,
    std::string_view hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename ExecutionPolicy>
void Test(std::string_view mark, const SearchServer& search_server, const std::vector<std::string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const std::string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    std::cout << total_relevance << std::endl;
}

#define TEST(policy) Test(#policy, search_server, queries, std::execution::policy)

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
void Benchmark();
// ------------------------- End Benchmarks -------------------------
