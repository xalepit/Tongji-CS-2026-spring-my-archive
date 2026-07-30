#ifndef ZIPPERHASHTABLE_H
#define ZIPPERHASHTABLE_H

#include "Genome.h"
#include "PositionList.h"

class HashEntry
{
private:
    Genome _key;
    PositionList _positions;
    // 同一桶中的不同 K-mer 通过该指针形成拉链。
    HashEntry *_next;

public:
    friend class ZipperHashTable;

    explicit HashEntry(const Genome &key);
    ~HashEntry();

    HashEntry(const HashEntry &) = delete;
    HashEntry &operator=(const HashEntry &) = delete;
};

class ZipperHashTable
{
private:
    // 动态桶数组由本类手工申请和释放。
    HashEntry **_buckets;
    int _capacity;
    int _keyCount;
    int _positionCount;
    int _collisionCount;

    int Hash(const Genome &key) const;
    HashEntry *FindEntry(const Genome &key);
    void InsertPosition(PositionList &list, int position);

public:
    explicit ZipperHashTable(int capacity);
    ~ZipperHashTable();

    void Clear();
    void Insert(const Genome &key, int position);
    PositionList Find(const Genome &key);

    int GetCapacity() const;
    int GetKeyCount() const;
    int GetPositionCount() const;
    int GetCollisionCount() const;
    double GetLoadFactor() const;

    ZipperHashTable(const ZipperHashTable &) = delete;
    ZipperHashTable &operator=(const ZipperHashTable &) = delete;
};

#endif // ZIPPERHASHTABLE_H
