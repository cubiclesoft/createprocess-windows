// Ordered hash map with integer and string keys in a memory compact format.  No detachable nodes but is CPU cache-friendly.
// Primarily useful for array style hashes with lots of insertions (up to 2^31 values), frequent key- and index-based lookups, some iteration, and few deletions.
// (C) 2017 CubicleSoft.  All Rights Reserved.

#ifndef CUBICLESOFT_PACKEDORDEREDHASH
#define CUBICLESOFT_PACKEDORDEREDHASH

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <new>

namespace CubicleSoft
{
	template <class T>
	class PackedOrderedHash;
	template <class T>
	class PackedOrderedHashNoCopy;

	template <class T>
	class PackedOrderedHashNode
	{
		friend class PackedOrderedHash<T>;
		friend class PackedOrderedHashNoCopy<T>;

	public:
		inline std::int64_t GetIntKey() { return IntKey; }
		inline char *GetStrKey() { return StrKey; }
		inline size_t GetStrLen() { return *(size_t *)(StrKey - sizeof(size_t)); }

	private:
		std::uint32_t PrevHashIndex;
		std::uint32_t NextHashIndex;

		std::int64_t IntKey;
		char *StrKey;

	public:
		T Value;
	};

	class PackedOrderedHashUtil
	{
	public:
		static size_t GetDJBX33XHashKey(const std::uint8_t *Str, size_t Size, size_t InitVal);
		static std::uint64_t GetSipHashKey(const std::uint8_t *Str, size_t Size, std::uint64_t Key1, std::uint64_t Key2, size_t cRounds, size_t dRounds);
	};

	// PackedOrderedHash.  A packed ordered hash.
	#include "packed_ordered_hash_util.h"

	// PackedOrderedHashNoCopy.  A packed ordered hash with a private copy constructor and assignment operator.
	#define CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
	#include "packed_ordered_hash_util.h"
	#undef CUBICLESOFT_PACKEDORDEREDHASH_NOCOPYASSIGN
}

#endif