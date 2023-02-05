// Определение оператора вывода структуры документ
#include <string>
#include <iostream>
#include "document.h"

using std::ostream;

ostream& operator<<(ostream& out, const Document& document) {
    using namespace std;
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s;
    return out;
}
