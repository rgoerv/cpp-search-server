#include <iostream>
#include <algorithm>
#include <vector>
#include <string_view>
#include <map>

#include "remove_duplicates.h"
#include "search_server.h"

/* Можно итерироваться по поисковому серверу с помощью написаных итераторов begin и end
   Методом .GetWordFrequencies(document_id) получать map с словами документа и ее частоту
   Частота нам не важна, дубликатом считается документ с одинаковым набором уникальных слов,
   что обеспечивают уникальные ключи контейнера map
   создать вектор map-ов из слов и его частоты
   методом .count()*/

   
   void RemoveDuplicates(SearchServer& search_server)
   {
	   using namespace std;

	   set<map<string_view, double>> words_words_freqs = {};
	   vector<int> ids_to_delete_;

	   for (const int & id : search_server) {	   
		   const auto & word_to_freq = search_server.GetWordFrequencies(id);
		   if (words_words_freqs.count(word_to_freq) > 0) {
			   cout << "Found duplicate document id "s << id << endl;
			   ids_to_delete_.push_back(id);
			   continue;
		   }
		   words_words_freqs.insert(word_to_freq);
	   }

	   for (const int id : ids_to_delete_) {
		   search_server.RemoveDocument(id);
	   }
   }