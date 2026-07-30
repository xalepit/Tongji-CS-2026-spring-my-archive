#ifndef READGENERATOR_H
#define READGENERATOR_H

#include "Genome.h"

class ReadGenerator;

class ReadRecord
{
private:
    Genome _sequence;
    int _sourcePosition;
    int _mutationCount;
    int _mutationOffsets[2];
    friend class ReadGenerator;

public:
    ReadRecord();

    const Genome &Sequence() const;
    int SourcePosition() const;
    int MutationCount() const;
    int MutationOffset(int index) const;
};

class ReadBatch
{
private:
    ReadRecord *_records;
    int _count;
    friend class ReadGenerator;

public:
    ReadBatch();
    ~ReadBatch();
    ReadBatch(const ReadBatch &other);
    ReadBatch &operator=(const ReadBatch &other);

    int Count() const;
    const ReadRecord &At(int index) const;
};

class ReadGenerator
{
public:
    static Genome GenerateReference(int length, unsigned int seed);
    static ReadBatch Generate(const Genome &referenceGenome,
                              int count,
                              int readLength,
                              int kmerLength,
                              unsigned int seed);
};

#endif // READGENERATOR_H
