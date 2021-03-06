/*
Copyright (C) 2008-2016 Vana Development Team

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
#pragma once

#include "Common/Types.hpp"

namespace Vana {
	namespace ChannelServer {
		struct StatusInfo {
			// TODO FIXME it appears that a review of reflection/skill ID are needed - some had 0, some had -1 for both
			StatusInfo() = default;
			StatusInfo(int32_t status, int32_t val, int32_t skillId, seconds_t time);
			StatusInfo(int32_t status, int32_t val, int16_t mobSkill, int16_t level, seconds_t time);
			StatusInfo(int32_t status, int32_t val, int16_t mobSkill, int16_t level, int32_t reflect, seconds_t time);

			int16_t mobSkill = 0;
			int16_t level = 0;
			int32_t status = 0;
			int32_t skillId = 0;
			int32_t reflection = -1;
			int32_t val = 0;
			seconds_t time;
		};
	}
}