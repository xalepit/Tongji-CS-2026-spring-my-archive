#ifndef GENOME_H
#define GENOME_H

class Genome
{
private:
    // 核心序列始终存放在连续、以 '\0' 结尾的自有字符数组中。
    int _length;
    char *_bases;

public:
    Genome();
    Genome(const char *bases, int length);
    ~Genome();
    Genome(const Genome &other);
    Genome &operator=(const Genome &other);
    bool operator==(const Genome &other) const;

    int Length() const;
    const char *Bases() const;
};

#endif // GENOME_H
