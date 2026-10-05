#ifndef SEARCHENGINE_CORE_H
#define SEARCHENGINE_CORE_H

#include "DnaLimits.h"
#include "Genome.h"
#include "SearchResult.h"
#include "ZipperHashTable.h"

class SearchEngine
{
private:
    Genome _referenceGenome;
    ZipperHashTable _hashTable;

    inline static constexpr int DEFAULT_HASH_CAPACITY = 8191;

    int _k;
    int _maxMismatch;
    SearchResult _lastResult;
    long long _indexBuildMicroseconds;

    void IndexConstruct();
    static bool IsDnaSequence(const Genome &sequence);

public:
    SearchEngine();
    explicit SearchEngine(const Genome &referenceGenome);
    SearchEngine(const Genome &referenceGenome,
                 int k,
                 int maxMismatch,
                 int capacity = DEFAULT_HASH_CAPACITY);

    void Search(const Genome &read);
    void SetMaxMismatch(int maxMismatch);

    const Genome &GetReferenceGenome() const;
    const SearchResult &GetLastResult() const;
    int GetK() const;
    int GetMaxMismatch() const;
    int GetIndexedKmerCount() const;
    int GetIndexedPositionCount() const;
    int GetHashCapacity() const;
    int GetCollisionCount() const;
    double GetLoadFactor() const;
    long long GetIndexBuildMicroseconds() const;
};

#endif // SEARCHENGINE_CORE_H
