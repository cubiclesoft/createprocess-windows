// A mixed, flexible variable data type (plain ol' data - POD) with all-public data access.
// (C) 2021 CubicleSoft.  All Rights Reserved.

#ifndef CUBICLESOFT_STATIC_MIXED_VAR
#define CUBICLESOFT_STATIC_MIXED_VAR

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

namespace CubicleSoft
{
	enum StaticMixedVarModes
	{
		MV_None,
		MV_Bool,
		MV_Int,
		MV_UInt,
		MV_Double,
		MV_Str
	};

	// Must be used like:  StaticMixedVar<char[8192]>
	// Designed to be extended but not overridden.
	template <class T>
	class StaticMixedVar
	{
	public:
		StaticMixedVarModes MxMode;
		std::int64_t MxInt;
		double MxDouble;
		T MxStr;
		size_t MxStrPos;

		StaticMixedVar() : MxMode(MV_None), MxInt(0), MxDouble(0.0), MxStrPos(0)
		{
		}

		// Some functions for those who prefer member functions over directly accessing raw class data.
		inline bool IsNone()
		{
			return (MxMode == MV_None);
		}

		inline bool IsBool()
		{
			return (MxMode == MV_Bool);
		}

		inline bool IsInt()
		{
			return (MxMode == MV_Int);
		}

		inline bool IsUInt()
		{
			return (MxMode == MV_UInt);
		}

		inline bool IsDouble()
		{
			return (MxMode == MV_Double);
		}

		inline bool IsStr()
		{
			return (MxMode == MV_Str);
		}

		inline bool GetBool()
		{
			return (MxInt != 0);
		}

		inline std::int64_t GetInt()
		{
			return MxInt;
		}

		inline std::uint64_t GetUInt()
		{
			return (std::uint64_t)MxInt;
		}

		inline double GetDouble()
		{
			return MxDouble;
		}

		inline char *GetStr()
		{
			return MxStr;
		}

		inline size_t GetSize()
		{
			return MxStrPos;
		}

		inline size_t GetMaxSize()
		{
			return sizeof(MxStr);
		}

		inline void SetBool(const bool newbool)
		{
			MxMode = MV_Bool;
			MxInt = (int)newbool;
		}

		inline void SetInt(const std::int64_t newint)
		{
			MxMode = MV_Int;
			MxInt = newint;
		}

		inline void SetUInt(const std::uint64_t newint)
		{
			MxMode = MV_UInt;
			MxInt = (std::int64_t)newint;
		}

		inline void SetDouble(const double newdouble)
		{
			MxMode = MV_Double;
			MxDouble = newdouble;
		}

		void SetStr(const char *str)
		{
			MxMode = MV_Str;
			MxStrPos = 0;
			while (MxStrPos < sizeof(MxStr) - 1 && *str)
			{
				MxStr[MxStrPos++] = *str++;
			}
			MxStr[MxStrPos] = '\0';
		}

		void SetData(const char *str, size_t size)
		{
			MxMode = MV_Str;
			MxStrPos = 0;
			while (MxStrPos < sizeof(MxStr) - 1 && size)
			{
				MxStr[MxStrPos++] = *str++;
				size--;
			}
			MxStr[MxStrPos] = '\0';
		}

		template<class... Args>
		void SetFormattedStr(const char *format, Args... args)
		{
			MxMode = MV_Str;

#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf_s(MxStr, sizeof(MxStr), _TRUNCATE, format, args...);
			MxStr[sizeof(MxStr) - 1] = '\0';
#else
			if (snprintf(MxStr, sizeof(MxStr), format, args...) < 0)  MxStr[0] = '\0';
#endif

			MxStrPos = strlen(MxStr);
		}

		// Prepend functions only prepend if there is enough space.
		void PrependStr(const char *str)
		{
			size_t y = strlen(str);
			if (MxStrPos + y < sizeof(MxStr) - 1)
			{
				memmove(MxStr + y, MxStr, MxStrPos + 1);
				memcpy(MxStr, str, y);
				MxStrPos += y;
			}
		}

		void PrependData(const char *str, size_t size)
		{
			if (MxStrPos + size < sizeof(MxStr) - 1)
			{
				memmove(MxStr + size, MxStr, MxStrPos + 1);
				memcpy(MxStr, str, size);
				MxStrPos += size;
			}
		}

		template<class... Args>
		void PrependFormattedStr(const char *format, Args... args)
		{
			T tempbuffer;
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf_s(tempbuffer, sizeof(tempbuffer), _TRUNCATE, format, args...);
			tempbuffer[sizeof(tempbuffer) - 1] = '\0';
#else
			snprintf(tempbuffer, sizeof(tempbuffer), format, args...);
#endif

			PrependStr(tempbuffer);
		}

		void PrependInt(const std::int64_t val, size_t radix = 10)
		{
			char tempbuffer[44];
			if (IntToString(tempbuffer, sizeof(tempbuffer), val, radix))  PrependStr(tempbuffer);
		}

		void PrependUInt(const std::uint64_t val, size_t radix = 10)
		{
			char tempbuffer[44];
			if (IntToString(tempbuffer, sizeof(tempbuffer), val, radix))  PrependStr(tempbuffer);
		}

		void PrependDouble(const double val, const size_t precision = 16)
		{
			char tempbuffer[100];
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf_s(tempbuffer, sizeof(tempbuffer), _TRUNCATE, "%1.*g", precision, val);
			tempbuffer[sizeof(tempbuffer) - 1] = '\0';
#else
			if (snprintf(tempbuffer, sizeof(tempbuffer), "%1.*g", (int)precision, val) < 0)  tempbuffer[0] = '\0';
#endif

			PrependStr(tempbuffer);
		}

		void AppendStr(const char *str)
		{
			while (MxStrPos < sizeof(MxStr) - 1 && *str)
			{
				MxStr[MxStrPos++] = *str++;
			}
			MxStr[MxStrPos] = '\0';
		}

		void AppendData(const char *str, size_t size)
		{
			while (MxStrPos < sizeof(MxStr) - 1 && size)
			{
				MxStr[MxStrPos++] = *str++;
				size--;
			}
			MxStr[MxStrPos] = '\0';
		}

		template<class... Args>
		void AppendFormattedStr(const char *format, Args... args)
		{
			T tempbuffer;
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf_s(tempbuffer, sizeof(tempbuffer), _TRUNCATE, format, args...);
			tempbuffer[sizeof(tempbuffer) - 1] = '\0';
#else
			if (snprintf(tempbuffer, sizeof(tempbuffer), format, args...) < 0)  tempbuffer[0] = '\0';
#endif

			AppendStr(tempbuffer);
		}

		void AppendInt(const std::int64_t val, size_t radix = 10)
		{
			char tempbuffer[44];
			if (IntToString(tempbuffer, sizeof(tempbuffer), val, radix))  AppendStr(tempbuffer);
		}

		void AppendUInt(const std::uint64_t val, size_t radix = 10)
		{
			char tempbuffer[44];
			if (IntToString(tempbuffer, sizeof(tempbuffer), val, radix))  AppendStr(tempbuffer);
		}

		void AppendDouble(const double val, const size_t precision = 16)
		{
			char tempbuffer[100];
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf_s(tempbuffer, sizeof(tempbuffer), _TRUNCATE, "%1.*g", precision, val);
			tempbuffer[sizeof(tempbuffer) - 1] = '\0';
#else
			if (snprintf(tempbuffer, sizeof(tempbuffer), "%1.*g", (int)precision, val) < 0)  tempbuffer[0] = '\0';
#endif

			AppendStr(tempbuffer);
		}

		inline void AppendChar(const char chr)
		{
			if (MxStrPos < sizeof(MxStr) - 1)
			{
				MxStr[MxStrPos++] = chr;
				MxStr[MxStrPos] = '\0';
			}
		}

		inline void AppendMissingChar(const char chr)
		{
			if ((!MxStrPos || MxStr[MxStrPos - 1] != chr) && MxStrPos < sizeof(MxStr) - 1)
			{
				MxStr[MxStrPos++] = chr;
				MxStr[MxStrPos] = '\0';
			}
		}

		inline bool RemoveTrailingChar(const char chr)
		{
			if (!MxStrPos || MxStr[MxStrPos - 1] != chr)  return false;

			MxStr[--MxStrPos] = '\0';

			return true;
		}

		inline void SetSize(const size_t size)
		{
			if (size < sizeof(MxStr))
			{
				MxStrPos = size;
				MxStr[MxStrPos] = '\0';
			}
		}

		// Swiped and slightly modified from Int::ToString().
		static bool IntToString(char *Result, size_t Size, std::uint64_t Num, size_t Radix = 10)
		{
			if (Size < 2)  return false;

			size_t x = Size, z;

			Result[--x] = '\0';
			if (!Num)  Result[--x] = '0';
			else
			{
				while (Num && x)
				{
					z = Num % Radix;
					Result[--x] = (char)(z > 9 ? z - 10 + 'A' : z + '0');
					Num /= Radix;
				}

				if (Num)  return false;
			}

			memmove(Result, Result + x, Size - x);

			return true;
		}

		static bool IntToString(char *Result, size_t Size, std::int64_t Num, size_t Radix = 10)
		{
			if (Num >= 0)  return IntToString(Result, Size, (std::uint64_t)Num, Radix);

			if (Size < 2)  return false;
			Result[0] = '-';

			return IntToString(Result + 1, Size - 1, (std::uint64_t)-Num, Radix);
		}
	};
}

#endif
