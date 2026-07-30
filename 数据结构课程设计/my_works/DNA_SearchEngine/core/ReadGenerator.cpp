#include "ReadGenerator.h"

#include "DnaLimits.h"

#include <random>
#include <stdexcept>

ReadRecord::ReadRecord()
    : _sequence(), _sourcePosition(-1), _mutationCount(0), _mutationOffsets{-1, -1}
{
}

const Genome &ReadRecord::Sequence() const
{
    return _sequence;
}

int ReadRecord::SourcePosition() const
{
    return _sourcePosition;
}

int ReadRecord::MutationCount() const
{
    return _mutationCount;
}

int ReadRecord::MutationOffset(int index) const
{
    if (index < 0 || index >= _mutationCount) {
        throw std::out_of_range("Mutation index is out of range");
    }
    return _mutationOffsets[index];
}

ReadBatch::ReadBatch()
    : _records(nullptr), _count(0)
{
}

ReadBatch::~ReadBatch()
{
    delete[] _records;
    _records = nullptr;
    _count = 0;
}

ReadBatch::ReadBatch(const ReadBatch &other)
    : _records(nullptr), _count(other._count)
{
    if (_count > 0) {
        _records = new ReadRecord[_count];
        for (int i = 0; i < _count; ++i) {
            _records[i] = other._records[i];
        }
    }
}

ReadBatch &ReadBatch::operator=(const ReadBatch &other)
{
    if (this == &other) {
        return *this;
    }

    ReadRecord *newRecords = nullptr;
    if (other._count > 0) {
        newRecords = new ReadRecord[other._count];
        for (int i = 0; i < other._count; ++i) {
            newRecords[i] = other._records[i];
        }
    }

    delete[] _records;
    _records = newRecords;
    _count = other._count;
    return *this;
}

int ReadBatch::Count() const
{
    return _count;
}

const ReadRecord &ReadBatch::At(int index) const
{
    if (index < 0 || index >= _count) {
        throw std::out_of_range("Read index is out of range");
    }
    return _records[index];
}

ReadBatch ReadGenerator::Generate(const Genome &referenceGenome,
                                  int count,
                                  int readLength,
                                  int kmerLength,
                                  unsigned int seed)
{
    if (referenceGenome.Length() < DnaLimits::MinReferenceLength
        || referenceGenome.Length() > DnaLimits::MaxReferenceLength) {
        throw std::invalid_argument(DnaLimits::ReferenceLengthError);
    }
    if (count < DnaLimits::MinReadCount
        || count > DnaLimits::MaxReadCount) {
        throw std::invalid_argument(DnaLimits::ReadCountError);
    }
    if (readLength < DnaLimits::MinReadLength
        || readLength > DnaLimits::MaxReadLength) {
        throw std::invalid_argument(DnaLimits::ReadLengthError);
    }
    if (kmerLength < DnaLimits::MinKmerLength
        || kmerLength > DnaLimits::MaxKmerLength) {
        throw std::invalid_argument(DnaLimits::KmerLengthError);
    }
    if (kmerLength > readLength) {
        throw std::invalid_argument(
            "Read length must be at least the protected prefix length");
    }

    const int mutableCount = readLength - kmerLength;

    std::mt19937 randomEngine(seed == 0U ? std::random_device{}() : seed);
    std::uniform_int_distribution<int> sourceDistribution(0, referenceGenome.Length() - readLength);
    std::uniform_int_distribution<int> mutationCountDistribution(1, mutableCount >= 2 ? 2 : 1);
    std::uniform_int_distribution<int> offsetDistribution(
        kmerLength,
        mutableCount > 0 ? readLength - 1 : kmerLength);
    std::uniform_int_distribution<int> baseDistribution(0, 3);
    const char bases[4] = {'A', 'C', 'G', 'T'};

    ReadBatch batch;
    batch._records = new ReadRecord[count];
    batch._count = count;

    for (int index = 0; index < count; ++index) {
        ReadRecord &record = batch._records[index];
        record._sourcePosition = sourceDistribution(randomEngine);
        record._mutationCount =
            mutableCount > 0 ? mutationCountDistribution(randomEngine) : 0;

        char *readBases = new char[readLength + 1];
        for (int i = 0; i < readLength; ++i) {
            readBases[i] = referenceGenome.Bases()[record._sourcePosition + i];
        }
        readBases[readLength] = '\0';

        // K 由本次生成时的界面参数传入，突变区间严格为 [K, ReadLength - 1]。
        if (record._mutationCount > 0) {
            record._mutationOffsets[0] = offsetDistribution(randomEngine);
        } else {
            record._mutationOffsets[0] = -1;
        }
        if (record._mutationCount == 2) {
            do {
                record._mutationOffsets[1] = offsetDistribution(randomEngine);
            } while (record._mutationOffsets[1] == record._mutationOffsets[0]);
            if (record._mutationOffsets[1] < record._mutationOffsets[0]) {
                const int temporary = record._mutationOffsets[0];
                record._mutationOffsets[0] = record._mutationOffsets[1];
                record._mutationOffsets[1] = temporary;
            }
        } else {
            record._mutationOffsets[1] = -1;
        }

        for (int mutation = 0; mutation < record._mutationCount; ++mutation) {
            const int offset = record._mutationOffsets[mutation];
            char replacement = readBases[offset];
            while (replacement == readBases[offset]) {
                replacement = bases[baseDistribution(randomEngine)];
            }
            readBases[offset] = replacement;
        }

        record._sequence = Genome(readBases, readLength);
        delete[] readBases;
    }

    return batch;
}

Genome ReadGenerator::GenerateReference(int length, unsigned int seed)
{
    if (length < DnaLimits::MinReferenceLength
        || length > DnaLimits::MaxReferenceLength) {
        throw std::invalid_argument(DnaLimits::ReferenceLengthError);
    }

    std::mt19937 randomEngine(seed == 0U ? std::random_device{}() : seed);
    std::uniform_int_distribution<int> baseDistribution(0, 3);
    const char bases[4] = {'A', 'C', 'G', 'T'};
    char *generated = new char[length + 1];
    for (int i = 0; i < length; ++i) {
        generated[i] = bases[baseDistribution(randomEngine)];
    }
    generated[length] = '\0';

    Genome reference(generated, length);
    delete[] generated;
    return reference;
}
