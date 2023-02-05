#include <iostream>
#include <string>
#include <vector>
#include <numeric>

#include "document.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "TestSearchServer.h"

using std::cerr;
using std::string;
using std::vector;
using std::endl;
using std::ostream;


#define RUN_TEST(func) RunTestImpl((func), (#func)) // �������� ����������� ���

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
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

// -------- ������ ��������� ������ ��������� ������� ----------

// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

// ���������� ����������
void TestAddDocument() {
    const int doc_id = 42;
    const string content = "black cat with collar in the city "s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server(""s);
    ASSERT_HINT(!server.GetDocumentCount(), "The server is not empty, but it should be empty.");
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    vector<Document> find_document = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);

    ASSERT_HINT(!find_document.empty(), "Documents have not were added.");
    ASSERT_EQUAL(find_document[0].id, doc_id);
    ASSERT_EQUAL(find_document[0].rating, accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
    // it makes no meaning to count the relevance for 1 document here, 
    // ln(1) = 0 [inverse_document_freq = log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size())]
    ASSERT_EQUAL(find_document[0].relevance, 0);
}

//��������� ����� ����, ���� ���� � �������
void TestMinusStopWordsSupportAndMatch() {
    const int doc_id = 42;
    const string content = "black cat with collar in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    const int int_actual_status = static_cast<int>(DocumentStatus::ACTUAL);

    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        // collar - minus word test this document
        const auto [query_first, document_status_first] = server.MatchDocument("cat in the city for sale -sale -lang -collar"s, doc_id);

        vector<string> query_true_first = {};
        ASSERT_EQUAL_HINT(query_first, query_true_first, "Document with minus words are not erase."s);
        const int int_first_status = static_cast<int>(document_status_first);
        ASSERT_EQUAL(int_first_status, int_actual_status);
    }

    {
        SearchServer search_server("in the with"s);
        search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [query_second, document_status_second] = search_server.MatchDocument("cat in the city for sale -sale -lang"s, doc_id);

        vector<string> query_true_second = { "cat"s, "city"s };
        ASSERT_EQUAL_HINT(query_second, query_true_second, "Stop word are not erase from the document.");
        const int int_second_status = static_cast<int>(document_status_second);
        ASSERT_EQUAL(int_second_status, int_actual_status);
    }
}

//���������� ���������� �� ������������� � ���������� ������������� � ��������
void TestSortAndComputingRelevanceRatingDocuments() {

    SearchServer search_server(""s);
    // �an be change documents_ and vector ratings - sort, computing relevance, rating will be performed.
    const map<int, string> documents_ = { {0, {"����� ��� � ������ �������"s}}, {1, {"�������� ��� �������� �����"s}},
                                    {2, {"��������� �� ������������� �����"s}}, {3, {"��������� ������� �������"s}} };
    search_server.AddDocument(0, documents_.at(0), DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, documents_.at(1), DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, documents_.at(2), DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, documents_.at(3), DocumentStatus::BANNED, { 9 });

    // Relevance computing
    map<string, map<int, double>> word_to_document_freqs_;

    for (const auto& [id, document] : documents_) {
        const vector<string> words = SplitIntoWords(document);
        const double inv_word_count = 1.0 / words.size();

        for (const string& word : words) {
            word_to_document_freqs_[word][id] += inv_word_count;
        }
    }

    map<int, double> document_to_relevance;

    for (const string& word : SplitIntoWords("�������� ��������� ���"s)) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = log(search_server.GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    auto ComputeRating = [](const vector<int> ratings)
    { return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()); };

    // Change arg func ComputeRating
    vector<Document> answerT = { {0, document_to_relevance.at(0), ComputeRating({8, -3})},
                                {1, document_to_relevance.at(1), ComputeRating({7, 2, 7})},
                                {2, document_to_relevance.at(2), ComputeRating({5, -12, 2, 1})} };

    sort(answerT.begin(), answerT.end(),
        [](const Document& lhs, const Document& rhs) {
            const double EPSILON = 1e-6;
    if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
        return lhs.rating > rhs.rating;
    }
    else {
        return lhs.relevance > rhs.relevance;
    }
        });

    const vector<Document> anserFind = search_server.FindTopDocuments("�������� ��������� ���"s);

    ASSERT_EQUAL(answerT.size(), answerT.size());
    const double epsilon = 1e-6;
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_EQUAL_HINT(anserFind[i].id, answerT[i].id, "Sort by relevance perform not correctly."s);
    }

    // ������ ����� ��������� ���������� �������� � ������������� � ��������� �������, � �� ������� ��� �����???
    // if sort its ok, then can be check the calculation relevance and rating
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_HINT(std::abs(answerT[i].relevance - anserFind[i].relevance) < epsilon, "Relevance calculated not correctly."s);
        ASSERT_EQUAL_HINT(answerT[i].rating, anserFind[i].rating, "Rating calculated not correctly."s);
    }
}

// �������� ������ ����������, ������� �������� ������
void TestFindDocumentStatus() {
    SearchServer search_server("� � ��"s);
    search_server.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::REMOVED, { 8, -3 });
    search_server.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    search_server.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    int id = search_server.FindTopDocuments("��������� ���"s, DocumentStatus::REMOVED).front().id;
    ASSERT_HINT(id == 0, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("��������� ���"s, DocumentStatus::IRRELEVANT).front().id;
    ASSERT_HINT(id == 1, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("��������� ���"s, DocumentStatus::ACTUAL).front().id;
    ASSERT_HINT(id == 2, "Find documents by statuss perform wrong."s);

    ASSERT_HINT(search_server.FindTopDocuments("��������� ���"s, DocumentStatus::BANNED).empty(),
        "Find documents by statuss perform wrong."s);
}

// ���� ������������� ���������, ����������� �������������
void TestPredicateFunc() {
    SearchServer search_server("� � ��"s);
    search_server.AddDocument(0, "����� ��� � ������ �������"s, DocumentStatus::REMOVED, { 8, -3 });
    search_server.AddDocument(1, "�������� ��� �������� �����"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    search_server.AddDocument(2, "��������� �� ������������� �����"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "��������� ������� �������"s, DocumentStatus::BANNED, { 9 });

    int id = search_server.FindTopDocuments("��������� ���"s,
        [](int document_id, DocumentStatus status, int rating) { return rating > 8; }).front().id;
    ASSERT_HINT(id == 3, "Function-predicate using not correctly by rating condition."s);

    id = search_server.FindTopDocuments("��������� ���"s,
        [](int document_id, DocumentStatus status, int rating) { return document_id == 1; }).front().id;
    ASSERT_HINT(id == 1, "Function-predicate using not correctly by document id condition."s);
}


// ������� TestSearchServer �������� ������ ����� ��� ������� ������
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // �� �������� �������� ��������� ����� �����
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusStopWordsSupportAndMatch);
    RUN_TEST(TestSortAndComputingRelevanceRatingDocuments);
    RUN_TEST(TestFindDocumentStatus);
    RUN_TEST(TestPredicateFunc);
}
// --------- ��������� ��������� ������ ��������� ������� -----------

