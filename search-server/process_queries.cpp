#include <vector>
#include <string>
#include <execution>
#include <algorithm>
#include <numeric>
#include <utility>

#include "document.h"
#include "process_queries.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
    [&search_server](const auto& query){
        return search_server.FindTopDocuments(query);
    });
    return result;    
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{   
    std::vector<Document> result_queries;
    for(const auto& documents : ProcessQueries(search_server, queries)) {
        result_queries.insert(result_queries.end(), documents.begin(), documents.end());
    }
    return result_queries;
}