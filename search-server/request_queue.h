// Обьявление класса RequestQueue, контролирующий статистику запросов 
#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <deque>

#include "search_server.h"
#include "document.h"

using std::vector;
using std::pair;
using std::deque;
using std::string;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : search_server_request(search_server) {
    }

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        if (requests_.size() >= 1440) {
            requests_.pop_front();
        }
        const auto result = search_server_request.FindTopDocuments(raw_query);
        requests_.push_back({ result, result.empty() });
        return result;
    }
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        vector<Document> search_results;
        bool IsEmpty = false;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_request;
};
