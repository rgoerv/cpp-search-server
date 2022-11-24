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
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    vector<Document> docT = {{42, 0, 2}};
    vector<Document> docFind = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);

    ASSERT_EQUAL(docFind.front().id, docT.front().id);
    ASSERT_EQUAL(docFind.front().rating, docT.front().rating);
    ASSERT_EQUAL(docFind.front().relevance, docT.front().relevance);
}

//Поддержка стоп слов и матчинг
void TestMinusWordsSupportAndMatch() {
    const int doc_id = 42;
    const string content = "black cat with collar in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int intStatus_actual = static_cast<int>(DocumentStatus::ACTUAL);

    SearchServer server;
    server.SetStopWords("in the for"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    const auto [queryfirst, documentStatusfirst] = server.MatchDocument("cat in the city for sale -sale -lang -collar"s, doc_id);
    vector<string> queryTfirst = {};
    ASSERT_EQUAL(queryfirst, queryTfirst);
    const int intStatus_firts = static_cast<int>(documentStatusfirst);
    ASSERT_EQUAL(intStatus_firts, intStatus_actual);

    const auto [querysecond, documentStatussecond] = server.MatchDocument("cat in the city for sale -sale -lang"s, doc_id);
    vector<string> queryTsecond = { "cat"s, "city"s};
    const int intStatus_second = static_cast<int>(documentStatussecond);
    ASSERT_EQUAL(querysecond, queryTsecond);
    ASSERT_EQUAL(intStatus_second, intStatus_actual);
}

//Сортировка документов по релевантности и вычисление релевантности и рейтинга
void TestSortRelevanceDocuments(){

    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    const vector<Document> answerT = {{1, 0.866434, 5}, {0, 0.173287, 2}, {2, 0.173287, -1}};
    const vector<Document> anserFind = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    
    ASSERT_EQUAL(answerT.size(), anserFind.size());
    const double EPSILON = 1e-6;
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_EQUAL_HINT(answerT[i].id, anserFind[i].id, "Sort by relevance perform not correctly."s);
    }

    // if sort its ok, then can be check the calculation relevance and rating
    for (size_t i = 0; i < answerT.size(); ++i) {
        ASSERT_HINT(abs(answerT[i].relevance - anserFind[i].relevance) < EPSILON, "Relevance calculated not correctly."s);
        ASSERT_EQUAL_HINT(answerT[i].rating, anserFind[i].rating, "Rating calculated not correctly."s);
    }
}

// Проверка поиска документов, имеющие заданный статус
void TestFindDocumentStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::REMOVED, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    int id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::REMOVED).front().id;
    ASSERT_HINT(id == 0, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::IRRELEVANT).front().id;
    ASSERT_HINT(id == 1, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::ACTUAL).front().id;
    ASSERT_HINT(id == 2, "Find documents by statuss perform wrong."s);

    id = search_server.FindTopDocuments("ухоженный кот"s, DocumentStatus::BANNED).front().id;
    ASSERT_HINT(id == 3, "Find documents by statuss perform wrong."s);
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
    RUN_TEST(TestMinusWordsSupportAndMatch);
    RUN_TEST(TestSortRelevanceDocuments);
    RUN_TEST(TestFindDocumentStatus);
    RUN_TEST(TestPredicateFunc);
}
// --------- Окончание модульных тестов поисковой системы -----------