// Здесь определены и (обьявлены) классы Paginator и IteratorRange, делящие вывод результатов поиска по страницам
#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using std::vector;
using std::pair;
using std::ostream;

template <class Iterator>
ostream& operator<<(ostream& out, const pair<Iterator, Iterator>& ItRange) {
    for (auto it = ItRange.first; it != ItRange.second; ++it) {
        out << *it;
    }
    return out;
}


template <class Iterator, class Container>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) {
        Page.first = begin;
        Page.second = end;
    }
    Iterator begin(const Container& container) const {
        return container.begin();
    }

    Iterator end(const Container& container) const {
        return container.end();
    }

    size_t size(const Container& container) const {
        return distance(begin(container), end(container));
    }
    pair<Iterator, Iterator> Page;
};

template <class Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        auto pages = range_begin;
        while (pages != range_end) {
            if (pages + page_size > range_end) {
                Pages.push_back({ pages, range_end });
                break;
            }
            else {
                Pages.push_back({ pages, pages + page_size });
                pages += page_size;
            }
        }
    }

    auto begin() const {
        return Pages.begin();
    }

    auto end() const {
        return Pages.end();
    }

    vector<pair<Iterator, Iterator>> Pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}