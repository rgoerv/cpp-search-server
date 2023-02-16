#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <iterator>

using std::ostream;

template <class Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) 
        : begin_(begin), end_(end), range_size_(distance(begin_, end_)) {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return range_size_;
    }

private:
    Iterator begin_, end_;
    size_t range_size_;
};

template <class Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        auto pages_begin = range_begin;
        while (pages_begin != range_end) {
            if (static_cast<size_t>(std::distance(pages_begin, range_end)) < page_size) {
                pages_.push_back({ pages_begin, range_end });
                break;
            }
            else {
                pages_.push_back({ pages_begin, pages_begin + page_size });
                pages_begin += page_size;
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    std::vector<IteratorRange<Iterator>> pages_;
};

template <class Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator>& ItRange) {
    for (Iterator it = ItRange.begin(); it != ItRange.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}