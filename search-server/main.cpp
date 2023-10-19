#include "process_queries.h"
#include "search_server.h"
#include "document.h"

#include "tests.h"

#include <execution>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
    if(argc < 2) {
        /* Создается экзмепляр класса с конструктором, принимающий строку стоп слов. */
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const std::string& text : {
                "white cat and yellow hat"s,
                "curly cat curly tail"s,
                "nasty dog with big eyes"s,
                "nasty pigeon john"s,
            }
        ) {
            /* Добавляем документы в базу. */
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }
        std::cout << "ACTUAL by default:"s << std::endl;
        /* Последовательная версия.
        /* Выводим только актуальные документы. */
        for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
            std::cout << document << std::endl;
        }
        std::cout << "BANNED:"s << std::endl;
        /* Последовательная версия.
        /* Только заблокированные документы. */
        for (const Document& document : search_server.FindTopDocuments(std::execution::seq, "curly nasty cat"s, 
            DocumentStatus::BANNED)) {
            std::cout << document << std::endl;
        }
        std::cout << "Even ids:"s << std::endl;
        /* Параллельная версия.
        /* Документы с четным id. */
        for (const Document& document : search_server.FindTopDocuments(std::execution::par, "curly nasty cat"s, 
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
            std::cout << document << std::endl;
        }
    } else {
        TestSearchServer();
        Benchmark();
    }
}