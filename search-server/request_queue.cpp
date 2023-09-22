#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <deque>

#include "request_queue.h"
#include "document.h"

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
    size_t count = count_if(requests_.begin(), requests_.end(), [](const QueryResult& request) {
        if (!request.IsEmpty) {
            return true;
        }
        return false;
    });
    return static_cast<int>(requests_.size() - count);
}