#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include "Genome.h"

class SearchEngine;

class SearchMatch
{
private:
    int _position;
    int _mismatchCount;
    int *_mismatchOffsets;

    void Configure(int position, int mismatchCount, const int *mismatchOffsets);
    friend class SearchResult;

public:
    SearchMatch();
    ~SearchMatch();
    SearchMatch(const SearchMatch &other);
    SearchMatch &operator=(const SearchMatch &other);

    int Position() const;
    int MismatchCount() const;
    int MismatchOffset(int index) const;
};

class SearchResult
{
private:
    Genome _read;
    SearchMatch *_matches;
    int _matchCount;
    int _capacity;
    int _candidateCount;
    double _elapsedMicroseconds;

    void Reset(const Genome &read);
    void AddCandidate();
    void AppendMatch(int position, int mismatchCount, const int *mismatchOffsets);
    void SetElapsedMicroseconds(double elapsedMicroseconds);
    friend class SearchEngine;

public:
    SearchResult();
    ~SearchResult();
    SearchResult(const SearchResult &other);
    SearchResult &operator=(const SearchResult &other);

    int CandidateCount() const;
    int MatchCount() const;
    double ElapsedMicroseconds() const;
    const SearchMatch &MatchAt(int index) const;
    const Genome &Read() const;
};

#endif // SEARCHRESULT_H
