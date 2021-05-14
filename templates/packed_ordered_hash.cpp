// Packed Ordered Hash Utilities.
// (C) 2014 CubicleSoft.  All Rights Reserved.

#include "packed_ordered_hash.h"

#if (defined(__GNUC__) && __GNUC__ >= 7) || (defined(__clang__) && __clang_major__ >= 12)
	#define FALL_THROUGH __attribute__ ((fallthrough))
#else
	#define FALL_THROUGH ((void)0)
#endif

namespace CubicleSoft
{
	size_t PackedOrderedHashUtil::GetDJBX33XHashKey(const std::uint8_t *Str, size_t Size, size_t InitVal)
	{
		std::uint32_t Result = (std::uint32_t)InitVal;
		std::uint32_t y;
		const size_t NumLeft = Size & 3;
		const std::uint8_t *StrEnd = Str + Size - NumLeft;

		while (Str != StrEnd)
		{
			// Changes the official implementation.
			y = *((const std::uint32_t *)Str);

			Result = ((Result << 5) + Result) ^ y;

			Str += 4;
		}

		switch (NumLeft)
		{
			case 3:  Result = ((Result << 5) + Result) ^ ((std::uint32_t)Str[2]);  FALL_THROUGH;
			case 2:  Result = ((Result << 5) + Result) ^ ((std::uint32_t)Str[1]);  FALL_THROUGH;
			case 1:  Result = ((Result << 5) + Result) ^ ((std::uint32_t)Str[0]);  break;
			case 0:  break;
		}

		return (size_t)Result;
	}

	#define ROTL(x, b) (std::uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

	#define SIPROUND \
		do { \
			v0 += v1; v1 = ROTL(v1, 13); v1 ^= v0; v0 = ROTL(v0, 32); \
			v2 += v3; v3 = ROTL(v3, 16); v3 ^= v2; \
			v0 += v3; v3 = ROTL(v3, 21); v3 ^= v0; \
			v2 += v1; v1 = ROTL(v1, 17); v1 ^= v2; v2 = ROTL(v2, 32); \
		} while(0)

	std::uint64_t PackedOrderedHashUtil::GetSipHashKey(const std::uint8_t *Str, size_t Size, std::uint64_t Key1, std::uint64_t Key2, size_t cRounds, size_t dRounds)
	{
		// "somepseudorandomlygeneratedbytes"
		std::uint64_t v0 = 0x736f6d6570736575ULL;
		std::uint64_t v1 = 0x646f72616e646f6dULL;
		std::uint64_t v2 = 0x6c7967656e657261ULL;
		std::uint64_t v3 = 0x7465646279746573ULL;
		std::uint64_t Result;
		std::uint64_t y;
		size_t x;
		const size_t NumLeft = Size & 7;
		const std::uint8_t *StrEnd = Str + Size - NumLeft;

		Result = ((std::uint64_t)Size) << 56;
		v3 ^= Key2;
		v2 ^= Key1;
		v1 ^= Key2;
		v0 ^= Key1;
		while (Str != StrEnd)
		{
			// Minor change to the official implementation.  (Does endianness actually matter?)
			y = *((const std::uint64_t *)Str);

			v3 ^= y;

			for (x = 0; x < cRounds; ++x)  SIPROUND;

			v0 ^= y;

			Str += 8;
		}

		switch (NumLeft)
		{
			case 7:  Result |= ((std::uint64_t)Str[6]) << 48;  FALL_THROUGH;
			case 6:  Result |= ((std::uint64_t)Str[5]) << 40;  FALL_THROUGH;
			case 5:  Result |= ((std::uint64_t)Str[4]) << 32;  FALL_THROUGH;
			case 4:  Result |= ((std::uint64_t)Str[3]) << 24;  FALL_THROUGH;
			case 3:  Result |= ((std::uint64_t)Str[2]) << 16;  FALL_THROUGH;
			case 2:  Result |= ((std::uint64_t)Str[1]) << 8;  FALL_THROUGH;
			case 1:  Result |= ((std::uint64_t)Str[0]);  break;
			case 0:  break;
		}

		v3 ^= Result;

		for (x = 0; x < cRounds; ++x)  SIPROUND;

		v0 ^= Result;
		v2 ^= 0xff;

		for (x = 0; x < dRounds; ++x)  SIPROUND;

		Result = v0 ^ v1 ^ v2 ^ v3;

		return Result;
	}
}
