/*
Copyright (C) 2008-2009 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef STRINGUTILITIES_H
#define STRINGUTILITIES_H

#include "Types.h"
#include <boost/tr1/functional.hpp>
#include <string>

using std::string;
using std::tr1::function;

namespace StringUtilities {
	int32_t noCaseCompare(const string &s1, const string &s2);
	void runFlags(const string &flags, function<void (string)> func);
	int64_t atoli(char *str); // ASCII to long int
	bool atob(const char *str); // ASCII to bool
}

inline
bool StringUtilities::atob(const char *str) {
	return atoi(str) != 0;
}

#endif
