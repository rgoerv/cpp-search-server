// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271

// Закомитьте изменения и отправьте их в свой репозиторий.

//Я не знаю как оно прошло тесты
#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
 
using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
 
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
 
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
 
vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
    for (const char c : text) 
    {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else { word += c; }
    }
    if (!word.empty()) 
    {
        words.push_back(word);
    }
    return words;
}
 
struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:

    void SetStopWords(const string& text) 
    {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
 
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        ++document_count_;
        
        if(!words.empty())
        {
            double size = 1.0 / words.size();
            for(const auto & word: words)
            {   
                documents[word][document_id] += size; // ok, видимо автоматическое добавление в контейнер элементов, если их нет, рулит
                // // Есть ли смысл от этой конструкции, вместо верхней, но с ней она тест на скорость не проходит
                // if(!word.empty())
                // {
                //     if((documents.count(word) > 0))
                //     {     
                //         if(documents.at(word).count(document_id) > 0)
                //         { documents.at(word).at(document_id) += size; }
                //         else documents.at(word).insert({document_id, size});
                //     }
                //     else documents.insert({word, {{document_id, size}}});     
                // }
            }
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        //if(raw_query.empty()) return {};
        const Query query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query);

        if(!matched_documents.empty())
        {
            sort(matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                     return lhs.relevance > rhs.relevance;
                });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
        }
        return matched_documents;
    }
 
private:

    int document_count_ = 0;
    map<string, map<int, double>> documents;
    set<string> stop_words_;
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
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
 
    Query ParseQuery(const string& text) const {
        Query query;
        if(!text.empty())
        {
            // Зачем нам считать слово на стоп плюс и минус в отдельной функции
            // Если можно разделять слова на плюс и минус без стоп слов
            for (const string& word : SplitIntoWordsNoStop(text))
            {
                if (word[0] == '-') query.minus_words.insert(word.substr(1));
                else query.plus_words.insert(word);
            }
        }
        return query;
    }

    vector<Document> FindAllDocuments(const Query& query) const 
    {
        vector<Document> matched_documents = {};
        if(!documents.empty() || (!query.plus_words.empty())) {

        map<int, double>  answers;             
        for(const auto & word: query.plus_words)
        {
            if((!word.empty()) && (!(documents.count(word) == 0)) && (!documents.at(word).empty()))
            {
                double idf_tf = 0;
                double idf = (log(document_count_ / (double)documents.at(word).size()));

                for(const auto & document : documents.at(word))
                {   
                    answers.insert({document.first, idf_tf});

                    if((answers.count(document.first) > 0)){   
                        answers.at(document.first) += idf * document.second;
                    }
                }
            }
        }

        if(!query.minus_words.empty() || !matched_documents.empty())
        {
            set<int> documents_erase;
            for(const auto & qword: query.minus_words)
            {
                if((!qword.empty()) && !(documents.count(qword) == 0) && !(documents.at(qword).empty()))
                {
                for(const auto & document : documents.at(qword))
                {
                    documents_erase.insert(document.first);
                }
                }
            }
            if(!documents_erase.empty())
                for(const auto & id : documents_erase) answers.erase(id);        
        }

            if(!answers.empty())
                for(const auto & [id, relevance] : answers) matched_documents.push_back({ id, relevance});
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
 
    return search_server;
}
 
int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}