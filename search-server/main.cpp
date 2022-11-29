// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

// Добавление документов
void TestAddDocument() {
    const int doc_id = 42;
    const string content = "black cat with collar in the city "s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server;
    ASSERT_HINT(!server.GetDocumentCount(), "The server is not empty, but it should be empty.");
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.GetDocumentCount() == 1, "Document not added || Document count not up.");

    vector<Document> find_document = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);

    ASSERT_HINT(!find_document.empty(), "Documents have not were added.");
    ASSERT_EQUAL(find_document[0].id, doc_id);
    ASSERT_EQUAL(find_document[0].rating, accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
    // it makes no meaning to count the relevance for 1 document here, 
    // ln(1) = 0 [inverse_document_freq = log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size())]
    const double epsilon = 1e-6;
    ASSERT(abs(find_document[0].relevance - 0.0) < epsilon);
}

//Поддержка минус слов, стоп слов и матчинг
void TestMinusStopWordsSupportAndMatch() {
    const int doc_id = 42;
    const string content = "black cat with collar in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int int_actual_status = static_cast<int>(DocumentStatus::ACTUAL);

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        // collar - minus word test this document
        const auto [query_first, document_status_first] = server.MatchDocument("cat in the city for sale -sale -lang -collar"s, doc_id);

        vector<string> query_true_first = {};
        ASSERT_EQUAL_HINT(query_first, query_true_first, "Document with minus words are not erase."s);
        const int int_first_status = static_cast<int>(document_status_first);
        ASSERT_EQUAL(int_first_status, int_actual_status);
    }

    {
        SearchServer search_server;
        search_server.SetStopWords("in the with"s);
        search_server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [query_second, document_status_second] = search_server.MatchDocument("cat in the city for sale"s, doc_id);

        vector<string> query_true_second = { "cat"s, "city"s};
        ASSERT_EQUAL_HINT(query_second, query_true_second, "Stop word are not erase from the document.");
        const int int_second_status = static_cast<int>(document_status_second);
        ASSERT_EQUAL(int_second_status, int_actual_status);
    }
}

//Сортировка документов по релевантности и вычисление релевантности и рейтинга
void TestSortDocuments(){

    SearchServer search_server;
    // Сan be change documents_ and vector ratings - sort, computing relevance, rating will be performed.
    const map<int, string> documents_ = {{0, {"белый кот и модный ошейник"s}}, {1, {"пушистый кот пушистый хвост"s}},
                                        {2, {"ухоженный пёс выразительные глаза"s}}, {3, {"ухоженный скворец евгений"s}}};
    search_server.AddDocument(0, documents_.at(0), DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, documents_.at(1), DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, documents_.at(2), DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, documents_.at(3), DocumentStatus::BANNED, {9});

    const vector<Document> anser_find = search_server.FindTopDocuments("пушистый ухоженный кот"s);

    ASSERT(!anser_find.empty());
    
    double last_relevance = anser_find[0].relevance;
    for (size_t i = 1; i < anser_find.size(); ++i) {
        ASSERT_HINT(last_relevance > anser_find[i].relevance, "Sort by relevance perform not correctly."s);
        last_relevance = anser_find[i].relevance;
    }
}

void TestComputingRelevance() {

    SearchServer search_server;
    // Сan be change documents_ and vector ratings - sort, computing relevance, rating will be performed.
    const map<int, string> documents_ = {{0, {"белый кот и модный ошейник"s}}, {1, {"пушистый кот пушистый хвост"s}},
                                    {2, {"ухоженный пёс выразительные глаза"s}}, {3, {"ухоженный скворец евгений"s}}};
    search_server.AddDocument(0, documents_.at(0), DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, documents_.at(1), DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, documents_.at(2), DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, documents_.at(3), DocumentStatus::BANNED, {9});

    // Relevance computing
    map<string, map<int, double>> word_to_document_freqs_;

    for(const auto& [id, document] : documents_) {
        const vector<string> words = SplitIntoWords(document);
        const double inv_word_count = 1.0 / words.size();
        
        for (const string& word : words) {
            word_to_document_freqs_[word][id] += inv_word_count;
        }
    }

    map<int, double> document_to_relevance;

    // Change query
    for (const string& word : SplitIntoWords("пушистый ухоженный кот"s)) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = log(search_server.GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    // RATING VALUE(NULL) IS NOT USING !!!ALARM!!! THIS IS ЗАГЛУШКА!!!
    vector<Document> answer_true = {{1, document_to_relevance.at(1), 0},
                                    {2, document_to_relevance.at(2), 0},
                                    {0, document_to_relevance.at(0), 0}};

    const vector<Document> anser_find = search_server.FindTopDocuments("пушистый ухоженный кот"s);
  
    ASSERT_EQUAL(answer_true.size(), anser_find.size());
    const double epsilon = 1e-6;
    for (size_t i = 0; i < answer_true.size(); ++i) {
        ASSERT_HINT(abs(answer_true[i].relevance - anser_find[i].relevance) < epsilon, "Relevance calculated not correctly."s);
    }
}

void TestComputingRating() {

    SearchServer search_server;
    // Сan be change documents_ and vector ratings - sort, computing relevance, rating will be performed.
    const map<int, string> documents_ = {{0, {"белый кот и модный ошейник"s}}, {1, {"пушистый кот пушистый хвост"s}},
                                    {2, {"ухоженный пёс выразительные глаза"s}}};
    search_server.AddDocument(0, documents_.at(0), DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, documents_.at(1), DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, documents_.at(2), DocumentStatus::ACTUAL, {5, -12, 2, 1});

    auto ComputeRating = [](const vector<int> ratings)
    { return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()); };

    // Change arg func ComputeRating
    // RELEVANCE VALUE(NULL) IS NOT USING !!!ALARM!!! THIS IS ЗАГЛУШКА!!!
    vector<Document> answer_true = {{1, 0, ComputeRating({7, 2, 7})},
                                    {2, 0, ComputeRating({5, -12, 2, 1})},
                                    {0, 0, ComputeRating({8, -3})}};

    const vector<Document> anser_find = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    
    ASSERT_EQUAL(answer_true.size(), anser_find.size());

    for (size_t i = 0; i < answer_true.size(); ++i) {
        ASSERT_EQUAL_HINT(answer_true[i].rating, anser_find[i].rating, "Rating calculated not correctly."s);
    }
}


// Проверка поиска документов, имеющие заданный статус
void TestFindDocumentStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::REMOVED, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});

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
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::REMOVED, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    int id = search_server.FindTopDocuments("ухоженный кот"s, 
                [](int document_id, DocumentStatus status, int rating) { return rating > 8; }).front().id;
    ASSERT_HINT(id == 3, "Function-predicate using not correctly by rating condition."s);
    
    id = search_server.FindTopDocuments("ухоженный кот"s, 
                [](int document_id, DocumentStatus status, int rating) { return document_id == 1; }).front().id;
    ASSERT_HINT(id == 1, "Function-predicate using not correctly by document id condition."s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusStopWordsSupportAndMatch);
    RUN_TEST(TestSortDocuments);
    RUN_TEST(TestComputingRelevance);
    RUN_TEST(TestComputingRating);
    RUN_TEST(TestFindDocumentStatus);
    RUN_TEST(TestPredicateFunc);
}
// --------- Окончание модульных тестов поисковой системы -----------