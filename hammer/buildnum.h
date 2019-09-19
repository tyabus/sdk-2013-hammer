//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BUILDNUM_H
#define BUILDNUM_H
#pragma once

namespace detail
{
	constexpr int C_strnicmp(const char *s1, const char *s2, int n)
	{
		for (; n > 0 && *s1; --n, ++s1, ++s2)
		{
			if (*s1 != *s2)
			{
				// in ascii char set, lowercase = uppercase | 0x20
				unsigned char c1 = *s1 | 0x20;
				unsigned char c2 = *s2 | 0x20;
				if (c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a'))
				{
					// ascii mismatch. only use the | 0x20 value if alphabetic.
					if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
					if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
					return c1 > c2 ? 1 : -1;
				}
			}
		}
		return (n > 0 && *s2) ? -1 : 0;
	}

	constexpr int C_atoi(const char* s, int N)
	{
		int ret = 0;
		int len = N - 1;
		int mul = 1;
		while (len >= 0)
		{
			ret += (s[len] - '0') * mul;
			mul *= 10;
			--len;
		}
		return ret;
	}

	static constexpr const char date[] = __DATE__;

	static constexpr const char *mon[12] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	static constexpr const char monl[12] =
	{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

}

constexpr int build_number(void)
{
	int m = 0;
	int d = 0;

	for (m = 0; m < 11; m++)
	{
		if (detail::C_strnicmp(&detail::date[0], detail::mon[m], 3) == 0)
			break;
		d += detail::monl[m];
	}

	d += detail::C_atoi(&detail::date[4], 2) - 1;

	int y = detail::C_atoi( &detail::date[7], 4 ) - 1900;

	int b = d + (int)((y - 1) * 365.25);

	if (((y % 4) == 0) && m > 1)
	{
		b += 1;
	}

	b -= 34995; // Oct 24 1996

	return b;
}

#endif // BUILDNUM_H
