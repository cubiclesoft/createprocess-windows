// A mixed, flexible variable data type (plain ol' data - POD) with all-public data access.
// (C) 2021 CubicleSoft.  All Rights Reserved.

#ifndef CUBICLESOFT_STATIC_WC_MIXED_VAR
#define CUBICLESOFT_STATIC_WC_MIXED_VAR

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
	#include <windows.h>
#else
	#include <wchar.h>

	#ifndef WCHAR
		#define WCHAR wchar_t
	#endif
#endif

namespace CubicleSoft
{
	enum StaticWCMixedVarModes
	{
		WCMV_None,
		WCMV_Bool,
		WCMV_Int,
		WCMV_UInt,
		WCMV_Double,
		WCMV_Str
	};

	// Must be used like:  StaticWCMixedVar<WCHAR[8192]>
	// Designed to be extended but not overridden.
	template <class T>
	class StaticWCMixedVar
	{
	public:
		StaticWCMixedVarModes MxMode;
		std::int64_t MxInt;
		double MxDouble;
		T MxStr;
		size_t MxStrPos;

		StaticWCMixedVar() : MxMode(WCMV_None), MxInt(0), MxDouble(0.0), MxStrPos(0)
		{
		}

		// Some functions for those who prefer member functions over directly accessing raw class data.
		inline bool IsNone()
		{
			return (MxMode == WCMV_None);
		}

		inline bool IsBool()
		{
			return (MxMode == WCMV_Bool);
		}

		inline bool IsInt()
		{
			return (MxMode == WCMV_Int);
		}

		inline bool IsUInt()
		{
			return (MxMode == WCMV_UInt);
		}

		inline bool IsDouble()
		{
			return (MxMode == WCMV_Double);
		}

		inline bool IsStr()
		{
			return (MxMode == WCMV_Str);
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

		inline WCHAR *GetStr()
		{
			return MxStr;
		}

		inline size_t GetSize()
		{
			return MxStrPos;
		}

		inline size_t GetMaxSize()
		{
			return sizeof(MxStr) / sizeof(WCHAR);
		}

		inline void SetBool(const bool newbool)
		{
			MxMode = WCMV_Bool;
			MxInt = (int)newbool;
		}

		inline void SetInt(const std::int64_t newint)
		{
			MxMode = WCMV_Int;
			MxInt = newint;
		}

		inline void SetUInt(const std::uint64_t newint)
		{
			MxMode = WCMV_UInt;
			MxInt = (std::int64_t)newint;
		}

		inline void SetDouble(const double newdouble)
		{
			MxMode = WCMV_Double;
			MxDouble = newdouble;
		}

		void SetStr(const WCHAR *str)
		{
			MxMode = WCMV_Str;
			MxStrPos = 0;
			while (MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1 && *str)
			{
				MxStr[MxStrPos++] = *str++;
			}
			MxStr[MxStrPos] = L'\0';
		}

		// Doesn't do anything fancy beyond expanding characters to fill the space of a wide character.
		void SetStr(const char *str)
		{
			MxMode = WCMV_Str;
			MxStrPos = 0;
			while (MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1 && *str)
			{
				MxStr[MxStrPos++] = (WCHAR)*str++;
			}
			MxStr[MxStrPos] = L'\0';
		}

		template<class... Args>
		void SetFormattedStr(const WCHAR *format, Args... args)
		{
			MxMode = WCMV_Str;

#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			swprintf_s(MxStr, sizeof(MxStr) / sizeof(WCHAR), format, args...);
			MxStr[sizeof(MxStr) / sizeof(WCHAR) - 1] = '\0';
#else
			if (swprintf(MxStr, sizeof(MxStr) / sizeof(WCHAR), format, args...) < 0)  MxStr[0] = '\0';
#endif

			MxStrPos = wcslen(MxStr);
		}

		// Prepend functions only prepend if there is enough space.
		void PrependStr(const WCHAR *str)
		{
			size_t y = wcslen(str);
			size_t y2 = y * sizeof(WCHAR);
			if (MxStrPos + y < (sizeof(MxStr) / sizeof(WCHAR)) - 1)
			{
				memmove(MxStr + y, MxStr, (MxStrPos + 1) * sizeof(WCHAR));
				memcpy(MxStr, str, y2);
				MxStrPos += y;
			}
		}

		// Doesn't do anything fancy beyond expanding characters to fill the space of a wide character.
		void PrependStr(const char *str)
		{
			size_t y = strlen(str);
			if (MxStrPos + y < (sizeof(MxStr) / sizeof(WCHAR)) - 1)
			{
				memmove(MxStr + y, MxStr, (MxStrPos + 1) * sizeof(WCHAR));
				MxStrPos += y;

				for (size_t x = 0; x < y; x++)
				{
					MxStr[x] = (WCHAR)*str++;
				}
			}
		}

		template<class... Args>
		void PrependFormattedStr(const WCHAR *format, Args... args)
		{
			T tempbuffer;
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			swprintf_s(tempbuffer, sizeof(tempbuffer) / sizeof(WCHAR), format, args...);
			tempbuffer[sizeof(tempbuffer) / sizeof(WCHAR) - 1] = '\0';
#else
			if (swprintf(tempbuffer, sizeof(tempbuffer) / sizeof(WCHAR), format, args...) < 0)  tempbuffer[0] = '\0';
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
			snprintf(tempbuffer, sizeof(tempbuffer), "%1.*g", (int)precision, val);
#endif

			PrependStr(tempbuffer);
		}

		void AppendStr(const WCHAR *str)
		{
			while (MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1 && *str)
			{
				MxStr[MxStrPos++] = *str++;
			}
			MxStr[MxStrPos] = L'\0';
		}

		// Doesn't do anything fancy beyond expanding characters to fill the space of a wide character.
		void AppendStr(const char *str)
		{
			while (MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1 && *str)
			{
				MxStr[MxStrPos++] = (WCHAR)*str++;
			}
			MxStr[MxStrPos] = L'\0';
		}

		template<class... Args>
		void AppendFormattedStr(const WCHAR *format, Args... args)
		{
			T tempbuffer;
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && defined(_MSC_VER) && _MSC_VER < 1900
			swprintf_s(tempbuffer, sizeof(tempbuffer) / sizeof(WCHAR), format, args...);
			tempbuffer[sizeof(tempbuffer) / sizeof(WCHAR) - 1] = '\0';
#else
			if (swprintf(tempbuffer, sizeof(tempbuffer) / sizeof(WCHAR), format, args...) < 0)  tempbuffer[0] = '\0';
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
			snprintf(tempbuffer, sizeof(tempbuffer), "%1.*g", (int)precision, val);
#endif

			AppendStr(tempbuffer);
		}

		inline void AppendChar(const WCHAR chr)
		{
			if (MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1)
			{
				MxStr[MxStrPos++] = chr;
				MxStr[MxStrPos] = L'\0';
			}
		}

		inline void AppendMissingChar(const WCHAR chr)
		{
			if ((!MxStrPos || MxStr[MxStrPos - 1] != chr) && MxStrPos < (sizeof(MxStr) / sizeof(WCHAR)) - 1)
			{
				MxStr[MxStrPos++] = chr;
				MxStr[MxStrPos] = L'\0';
			}
		}

		inline bool RemoveTrailingChar(const WCHAR chr)
		{
			if (!MxStrPos || MxStr[MxStrPos - 1] != chr)  return false;

			MxStr[--MxStrPos] = L'\0';

			return true;
		}

		inline void SetSize(const size_t size)
		{
			if (size < (sizeof(MxStr) / sizeof(WCHAR)))
			{
				MxStrPos = size;
				MxStr[MxStrPos] = L'\0';
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
