#include "SearchResult.h"

#include <stdexcept>

SearchMatch::SearchMatch()
    : _position(-1), _mismatchCount(0), _mismatchOffsets(nullptr)
{
}

SearchMatch::~SearchMatch()
{
    delete[] _mismatchOffsets;
    _mismatchOffsets = nullptr;
    _mismatchCount = 0;
}

SearchMatch::SearchMatch(const SearchMatch &other)
    : _position(other._position),
      _mismatchCount(other._mismatchCount),
      _mismatchOffsets(nullptr)
{
    if (_mismatchCount > 0) {
        _mismatchOffsets = new int[_mismatchCount];
        for (int i = 0; i < _mismatchCount; ++i) {
            _mismatchOffsets[i] = other._mismatchOffsets[i];
        }
    }
}

SearchMatch &SearchMatch::operator=(const SearchMatch &other)
{
    if (this == &other) {
        return *this;
    }
    Configure(other._position, other._mismatchCount, other._mismatchOffsets);
    return *this;
}

void SearchMatch::Configure(int position, int mismatchCount, const int *mismatchOffsets)
{
    int *newOffsets = nullptr;
    if (mismatchCount > 0) {
        newOffsets = new int[mismatchCount];
        for (int i = 0; i < mismatchCount; ++i) {
            newOffsets[i] = mismatchOffsets[i];
        }
    }

    delete[] _mismatchOffsets;
    _mismatchOffsets = newOffsets;
    _position = position;
    _mismatchCount = mismatchCount;
}

int SearchMatch::Position() const
{
    return _position;
}

int SearchMatch::MismatchCount() const
{
    return _mismatchCount;
}

int SearchMatch::MismatchOffset(int index) const
{
    if (index < 0 || index >= _mismatchCount) {
        throw std::out_of_range("Mismatch index is out of range");
    }
    return _mismatchOffsets[index];
}

SearchResult::SearchResult()
    : _read(),
      _matches(nullptr),
      _matchCount(0),
      _capacity(0),
      _candidateCount(0),
      _elapsedMicroseconds(0.0)
{
}

SearchResult::~SearchResult()
{
    delete[] _matches;
    _matches = nullptr;
    _matchCount = 0;
    _capacity = 0;
}

SearchResult::SearchResult(const SearchResult &other)
    : _read(other._read),
      _matches(nullptr),
      _matchCount(other._matchCount),
      _capacity(other._matchCount),
      _candidateCount(other._candidateCount),
      _elapsedMicroseconds(other._elapsedMicroseconds)
{
    if (_capacity > 0) {
        _matches = new SearchMatch[_capacity];
        for (int i = 0; i < _matchCount; ++i) {
            _matches[i] = other._matches[i];
        }
    }
}

SearchResult &SearchResult::operator=(const SearchResult &other)
{
    if (this == &other) {
        return *this;
    }

    SearchMatch *newMatches = nullptr;
    if (other._matchCount > 0) {
        newMatches = new SearchMatch[other._matchCount];
        for (int i = 0; i < other._matchCount; ++i) {
            newMatches[i] = other._matches[i];
        }
    }

    delete[] _matches;
    _matches = newMatches;
    _matchCount = other._matchCount;
    _capacity = other._matchCount;
    _candidateCount = other._candidateCount;
    _elapsedMicroseconds = other._elapsedMicroseconds;
    _read = other._read;
    return *this;
}

void SearchResult::Reset(const Genome &read)
{
    delete[] _matches;
    _matches = nullptr;
    _matchCount = 0;
    _capacity = 0;
    _candidateCount = 0;
    _elapsedMicroseconds = 0.0;
    _read = read;
}

void SearchResult::AddCandidate()
{
    ++_candidateCount;
}

void SearchResult::AppendMatch(int position, int mismatchCount, const int *mismatchOffsets)
{
    if (_matchCount == _capacity) {
        const int newCapacity = _capacity == 0 ? 4 : _capacity * 2;
        SearchMatch *newMatches = new SearchMatch[newCapacity];
        for (int i = 0; i < _matchCount; ++i) {
            newMatches[i] = _matches[i];
        }
        delete[] _matches;
        _matches = newMatches;
        _capacity = newCapacity;
    }

    _matches[_matchCount].Configure(position, mismatchCount, mismatchOffsets);
    ++_matchCount;
}

void SearchResult::SetElapsedMicroseconds(double elapsedMicroseconds)
{
    _elapsedMicroseconds = elapsedMicroseconds;
}

int SearchResult::CandidateCount() const
{
    return _candidateCount;
}

int SearchResult::MatchCount() const
{
    return _matchCount;
}

double SearchResult::ElapsedMicroseconds() const
{
    return _elapsedMicroseconds;
}

const SearchMatch &SearchResult::MatchAt(int index) const
{
    if (index < 0 || index >= _matchCount) {
        throw std::out_of_range("Match index is out of range");
    }
    return _matches[index];
}

const Genome &SearchResult::Read() const
{
    return _read;
}
