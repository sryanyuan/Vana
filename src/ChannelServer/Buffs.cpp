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
#include "Buffs.hpp"
#include "Common/Algorithm.hpp"
#include "Common/BuffDataProvider.hpp"
#include "Common/GameLogicUtilities.hpp"
#include "Common/SkillConstants.hpp"
#include "Common/SkillDataProvider.hpp"
#include "ChannelServer/BuffsPacket.hpp"
#include "ChannelServer/ChannelServer.hpp"
#include "ChannelServer/Map.hpp"
#include "ChannelServer/Player.hpp"
#include "ChannelServer/SummonHandler.hpp"

namespace Vana {
namespace ChannelServer {

auto Buffs::addBuff(ref_ptr_t<Player> player, skill_id_t skillId, skill_level_t level, int16_t addedInfo, map_object_t mapMobId) -> Result {
	auto source = BuffSource::fromSkill(skillId, level);
	auto &buffProvider = ChannelServer::getInstance().getBuffDataProvider();
	if (!buffProvider.isBuff(source)) {
		return Result::Failure;
	}

	auto skill = source.getSkillData(ChannelServer::getInstance().getSkillDataProvider());
	seconds_t time = skill->buffTime;
	switch (skillId) {
		case Vana::Skills::DragonKnight::DragonRoar:
			time = seconds_t{skill->y};
			break;
		case Vana::Skills::SuperGm::Hide:
			time = Vana::Buffs::MaxBuffTime;
			break;
	}

	auto buffs = preprocessBuff(player, source, time);
	if (!buffs.anyBuffs()) return Result::Failure;

	auto &basics = buffProvider.getBuffsByEffect();
	for (const auto &buffInfo : buffs.getBuffInfo()) {
		if (buffInfo == basics.homingBeacon) {
			if (mapMobId == player->getActiveBuffs()->getHomingBeaconMob()) return Result::Failure;
			player->getActiveBuffs()->resetHomingBeaconMob(mapMobId);
			break;
		}
	}

	return player->getActiveBuffs()->addBuff(source, buffs, time);
}

auto Buffs::addBuff(ref_ptr_t<Player> player, item_id_t itemId, const seconds_t &time) -> Result {
	auto source = BuffSource::fromItem(itemId);
	if (!ChannelServer::getInstance().getBuffDataProvider().isBuff(source)) {
		return Result::Failure;
	}

	auto buffs = preprocessBuff(player, source, time);
	if (!buffs.anyBuffs()) return Result::Failure;

	return player->getActiveBuffs()->addBuff(source, buffs, time);
}

auto Buffs::addBuff(ref_ptr_t<Player> player, mob_skill_id_t skillId, mob_skill_level_t level, milliseconds_t delay) -> Result {
	auto source = BuffSource::fromMobSkill(skillId, level);
	if (!ChannelServer::getInstance().getBuffDataProvider().isDebuff(source)) {
		return Result::Failure;
	}

	if (player->getStats()->isDead() || player->getActiveBuffs()->hasHolyShield() || player->hasGmBenefits()) {
		return Result::Failure;
	}

	auto skill = source.getMobSkillData(ChannelServer::getInstance().getSkillDataProvider());
	seconds_t time{skill->time};

	auto buffs = preprocessBuff(player, source, time);
	if (!buffs.anyBuffs()) return Result::Failure;

	return player->getActiveBuffs()->addBuff(source, buffs.withDelay(delay), time);
}

auto Buffs::endBuff(ref_ptr_t<Player> player, const BuffSource &source, bool fromTimer) -> void {
	player->getActiveBuffs()->removeBuff(source, preprocessBuff(player, source, seconds_t{0}), fromTimer);
}

auto Buffs::buffMayApply(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, const BuffInfo &buff) -> bool {
	switch (source.getType()) {
		case BuffSourceType::Skill: {
			skill_id_t skillId = source.getSkillId();
			skill_level_t skillLevel = source.getSkillLevel();
			if (GameLogicUtilities::isDarkSight(skillId)) {
				return
					buff.getValue() != BuffSkillValue::Speed ||
					skillLevel != ChannelServer::getInstance().getSkillDataProvider().getMaxLevel(skillId);
			}
			return true;
		}
		case BuffSourceType::MobSkill: {
			mob_skill_id_t skillId = source.getMobSkillId();
			return player->getActiveBuffs()->getBuffSource(buff).is_initialized() ?
				false :
				true;
		}
		case BuffSourceType::Item: {
			item_id_t itemId = source.getItemId();
			return true;
		}
	}
	throw NotImplementedException{"BuffSourceType"};
}

auto Buffs::preprocessBuff(ref_ptr_t<Player> player, skill_id_t skillId, skill_level_t level, const seconds_t &time) -> Buff {
	auto source = BuffSource::fromSkill(skillId, level);
	return preprocessBuff(player, source, time);
}

auto Buffs::preprocessBuff(ref_ptr_t<Player> player, item_id_t itemId, const seconds_t &time) -> Buff {
	auto source = BuffSource::fromItem(itemId);
	return preprocessBuff(player, source, time);
}

auto Buffs::preprocessBuff(ref_ptr_t<Player> player, mob_skill_id_t skillId, mob_skill_level_t level, const seconds_t &time) -> Buff {
	auto source = BuffSource::fromMobSkill(skillId, level);
	return preprocessBuff(player, source, time);
}

auto Buffs::preprocessBuff(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time) -> Buff {
	auto &buffProvider = ChannelServer::getInstance().getBuffDataProvider();

	if (source.getType() == BuffSourceType::Item) {
		item_id_t itemId = source.getItemId();
		switch (itemId) {
			case Items::BeholderHexWdef:
			case Items::BeholderHexMdef:
			case Items::BeholderHexAcc:
			case Items::BeholderHexAvo:
			case Items::BeholderHexWatk: {
				BuffInfo data = SummonHandler::makeBuff(player, itemId);
				Buff parsed{data};
				return preprocessBuff(player, source, time, parsed);
			}
		}
	}
	auto &skillsInfo = buffProvider.getInfo(source);
	return preprocessBuff(player, source, time, skillsInfo);
}

auto Buffs::preprocessBuff(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, const Buff &buff) -> Buff {
	vector_t<BuffInfo> applicableBuffs;
	vector_t<BuffInfo> existingBuffs;
	if (buff.isSelectionBuff()) {
		int16_t chance = Randomizer::percentage<int16_t>();
		for (const auto &buffInfo : buff.getBuffInfo()) {
			if (chance < buffInfo.getChance()) {
				existingBuffs.push_back(buffInfo);
				break;
			}
			else chance -= buffInfo.getChance();
		}
	}
	else {
		existingBuffs = buff.getBuffInfo();
	}

	for (auto &info : existingBuffs) {
		if (!buffMayApply(player, source, time, info)) continue;
		applicableBuffs.push_back(info);
	}

	return buff.withBuffs(applicableBuffs);
}

auto Buffs::preprocessBuff(const Buff &buff, const vector_t<uint8_t> &bitPositionsToInclude) -> Buff {
	vector_t<BuffInfo> applicable;
	for (const auto &info : buff.getBuffInfo()) {
		bool found = false;
		for (const auto &bitPosition : bitPositionsToInclude) {
			if (bitPosition == info) {
				found = true;
				break;
			}
		}
		if (found) {
			applicable.push_back(info);
		}
	}
	return buff.withBuffs(applicable);
}

auto Buffs::convertToPacketTypes(const Buff &buff) -> BuffPacketValues {
	BuffPacketValues ret;
	for (const auto &buffInfo : buff.getBuffInfo()) {
		ret.player.types[buffInfo.getBuffByte()] |= buffInfo.getBuffType();
		if (buffInfo.isMovementAffecting()) ret.player.anyMovementBuffs = true;
		if (buffInfo.hasMapInfo()) {
			if (!ret.map.is_initialized()) {
				ret.map = BuffPacketStructure{};
			}
			ret.map.get().types[buffInfo.getBuffByte()] |= buffInfo.getBuffType();
		}
	}
	return ret;
}

auto Buffs::convertToPacket(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, const Buff &buff) -> BuffPacketValues {
	BuffPacketValues ret;
	bool anyMap = false;
	for (const auto &buffInfo : buff.getBuffInfo()) {
		ret.player.types[buffInfo.getBuffByte()] |= buffInfo.getBuffType();
		if (buffInfo.isMovementAffecting()) ret.player.anyMovementBuffs = true;
		if (buffInfo.hasMapInfo()) {
			auto mapValue = buffInfo.getMapInfo();
			if (!anyMap) {
				ret.map = BuffPacketStructure{};
				anyMap = true;
			}

			auto &map = ret.map.get();
			map.types[buffInfo.getBuffByte()] |= buffInfo.getBuffType();
			map.values.push_back(getValue(player, source, time, buffInfo.getBitPosition(), mapValue));
		}

		ret.player.values.push_back(getValue(player, source, time, buffInfo));
	}
	ret.delay = buff.getDelay();
	return ret;
}

auto Buffs::getValue(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, const BuffInfo &buff) -> BuffPacketValue {
	if (buff.getValue() == BuffSkillValue::Predefined) {
		return BuffPacketValue::fromValue(2, buff.getPredefinedValue());
	}
	return getValue(player, source, time, buff.getBitPosition(), buff.getValue(), 2);
}

auto Buffs::getValue(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, uint8_t bitPosition, const BuffMapInfo &buff) -> BuffPacketValue {
	if (buff.getValue() == BuffSkillValue::Predefined) {
		return BuffPacketValue::fromValue(buff.getSize(), buff.getPredefinedValue());
	}
	return getValue(player, source, time, bitPosition, buff.getValue(), buff.getSize());
}

auto Buffs::getValue(ref_ptr_t<Player> player, const BuffSource &source, const seconds_t &time, uint8_t bitPosition, BuffSkillValue value, uint8_t buffValueSize) -> BuffPacketValue {
	switch (source.getType()) {
		case BuffSourceType::Item: {
			throw NotImplementedException{"Item BuffSkillValue type"};
		}
		case BuffSourceType::MobSkill: {
			mob_skill_id_t skillId = source.getMobSkillId();
			mob_skill_level_t skillLevel = source.getMobSkillLevel();
			auto skill = source.getMobSkillData(ChannelServer::getInstance().getSkillDataProvider());
			switch (value) {
				case BuffSkillValue::X: return BuffPacketValue::fromValue(buffValueSize, skill->x);
				case BuffSkillValue::Y: return BuffPacketValue::fromValue(buffValueSize, skill->y);
				case BuffSkillValue::Prop: return BuffPacketValue::fromValue(buffValueSize, skill->prop);
				case BuffSkillValue::Level: return BuffPacketValue::fromValue(buffValueSize, skillLevel);
				case BuffSkillValue::SkillId: return BuffPacketValue::fromValue(buffValueSize, skillId);
				case BuffSkillValue::BitpackedSkillAndLevel32: return BuffPacketValue::fromValue(buffValueSize, (static_cast<int32_t>(skillLevel) << 16) | skillId);
				case BuffSkillValue::SpecialProcessing: {
					switch (skillId) {
						// This unusual skill has two "values"
						case MobSkills::Poison: {
							PacketBuilder builder;
							builder.add<int16_t>(static_cast<int16_t>(skill->x));
							builder.add<int16_t>(skillId);
							builder.add<int16_t>(skillLevel);
							return BuffPacketValue::fromPacket(builder);
						}
					}
					throw NotImplementedException{"SpecialProcessing mob skill"};
				}
			}
			throw NotImplementedException{"Mob BuffSkillValue type"};
		}
		case BuffSourceType::Skill: {
			skill_id_t skillId = source.getSkillId();
			skill_level_t skillLevel = source.getSkillLevel();
			auto skill = source.getSkillData(ChannelServer::getInstance().getSkillDataProvider());

			switch (value) {
				case BuffSkillValue::X: return BuffPacketValue::fromValue(buffValueSize, skill->x);
				case BuffSkillValue::Y: return BuffPacketValue::fromValue(buffValueSize, skill->y);
				case BuffSkillValue::Speed: return BuffPacketValue::fromValue(buffValueSize, skill->speed);
				case BuffSkillValue::Jump: return BuffPacketValue::fromValue(buffValueSize, skill->jump);
				case BuffSkillValue::Watk: return BuffPacketValue::fromValue(buffValueSize, skill->wAtk);
				case BuffSkillValue::Wdef: return BuffPacketValue::fromValue(buffValueSize, skill->wDef);
				case BuffSkillValue::Matk: return BuffPacketValue::fromValue(buffValueSize, skill->mAtk);
				case BuffSkillValue::Mdef: return BuffPacketValue::fromValue(buffValueSize, skill->mDef);
				case BuffSkillValue::Accuracy: return BuffPacketValue::fromValue(buffValueSize, skill->acc);
				case BuffSkillValue::Avoid: return BuffPacketValue::fromValue(buffValueSize, skill->avo);
				case BuffSkillValue::Prop: return BuffPacketValue::fromValue(buffValueSize, skill->prop);
				case BuffSkillValue::Morph: return BuffPacketValue::fromValue(buffValueSize, skill->morph);
				case BuffSkillValue::Level: return BuffPacketValue::fromValue(buffValueSize, skillLevel);
				case BuffSkillValue::SkillId: return BuffPacketValue::fromValue(buffValueSize, skillId);
				case BuffSkillValue::MobCount: return BuffPacketValue::fromValue(buffValueSize, skill->mobCount);
				case BuffSkillValue::Range: return BuffPacketValue::fromValue(buffValueSize, skill->range);
				case BuffSkillValue::Damage: return BuffPacketValue::fromValue(buffValueSize, skill->damage);
				case BuffSkillValue::BitpackedXy16: return BuffPacketValue::fromValue(buffValueSize, (skill->x << 8) | skill->y);

				case BuffSkillValue::SpecialProcessing:
					switch (skillId) {
						case Vana::Skills::Crusader::ComboAttack:
						case Vana::Skills::DawnWarrior::ComboAttack:
							return BuffPacketValue::fromValue(
								buffValueSize,
								player->getActiveBuffs()->getCombo() + 1
							);
						case Vana::Skills::NightLord::ShadowStars:
							return BuffPacketValue::fromValue(
								buffValueSize,
								(player->getInventory()->doShadowStars() % 10000) + 1
							);
						case Vana::Skills::SuperGm::Hide:
							// TODO FIXME BUFFS
							return BuffPacketValue::fromValue(buffValueSize, 1);
					}

					throw NotImplementedException{"SpecialProcessing skill"};

				case BuffSkillValue::SpecialPacket: {
					auto &basics = ChannelServer::getInstance().getBuffDataProvider().getBuffsByEffect();

					PacketBuilder builder;
					if (bitPosition == basics.energyCharge) {
						builder.add<int32_t>(player->getActiveBuffs()->getEnergyChargeLevel());
					}
					else if (bitPosition == basics.dashSpeed) {
						builder.add<int32_t>(skill->x);
					}
					else if (bitPosition == basics.dashJump) {
						builder.add<int32_t>(skill->y);
					}
					else if (bitPosition == basics.mount) {
						builder.add<item_id_t>(player->getActiveBuffs()->getMountItemId());
					}
					else if (bitPosition == basics.speedInfusion) {
						builder.add<int32_t>(skill->x);
					}
					else if (bitPosition == basics.homingBeacon) {
						builder.unk<int32_t>(1);
					}
					else throw NotImplementedException{"SpecialPacket skill"};

					builder.add<int32_t>(
						bitPosition == basics.energyCharge ?
							0 :
							skillId);
					builder.unk<int32_t>();
					builder.unk<int8_t>();

					if (bitPosition == basics.energyCharge) {
						builder.add<int16_t>(static_cast<int16_t>(time.count()));
					}
					else if (bitPosition == basics.dashSpeed) {
						builder.add<int16_t>(static_cast<int16_t>(time.count()));
					}
					else if (bitPosition == basics.dashJump) {
						builder.add<int16_t>(static_cast<int16_t>(time.count()));
					}
					else if (bitPosition == basics.mount) {
						// Intentionally blank
					}
					else if (bitPosition == basics.speedInfusion) {
						builder.unk<int32_t>();
						builder.unk<int8_t>();
						builder.add<int16_t>(static_cast<int16_t>(time.count()));
					}
					else if (bitPosition == basics.homingBeacon) {
						builder.add<map_object_t>(player->getActiveBuffs()->getHomingBeaconMob());
					}

					return BuffPacketValue::fromSpecialPacket(builder);
				}

				case BuffSkillValue::GenderSpecificMorph:
					return BuffPacketValue::fromValue(
						buffValueSize,
						skill->morph +
						(player->getGender() == Gender::Male ? 0 : 100)
					);
			}

			throw NotImplementedException{"Skill BuffSkillValue type"};
		}
	}

	throw NotImplementedException{"BuffSourceType"};
}

}
}