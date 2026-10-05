#ifndef DNALIMITS_H
#define DNALIMITS_H

namespace DnaLimits {

inline constexpr int MinReferenceLength = 2000;
inline constexpr int MaxReferenceLength = 5000;
inline constexpr int MinReadLength = 20;
inline constexpr int MaxReadLength = 150;
inline constexpr int MinKmerLength = 2;
inline constexpr int MaxKmerLength = 10;
inline constexpr int MinMismatchCount = 0;
inline constexpr int MaxMismatchCount = 2;
inline constexpr int MinReadCount = 1;
inline constexpr int MaxReadCount = 50;

inline constexpr int DefaultReferenceLength = 3000;
inline constexpr int DefaultReadLength = 30;
inline constexpr int DefaultKmerLength = 6;
inline constexpr int DefaultMismatchCount = 2;
inline constexpr int DefaultReadCount = 50;

inline constexpr const char *ReferenceLengthError =
    "Reference genome length must be between 2000 and 5000";
inline constexpr const char *ReadLengthError =
    "Read length must be between 20 and 150";
inline constexpr const char *KmerLengthError =
    "K must be between 2 and 10";
inline constexpr const char *MismatchCountError =
    "Maximum mismatch count must be between 0 and 2";
inline constexpr const char *ReadCountError =
    "Read count must be between 1 and 50";

} // namespace DnaLimits

#endif // DNALIMITS_H
