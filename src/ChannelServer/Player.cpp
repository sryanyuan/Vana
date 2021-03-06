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
#include "Player.hpp"
#include "Common/CommonHeader.hpp"
#include "Common/Database.hpp"
#include "Common/EnumUtilities.hpp"
#include "Common/GameConstants.hpp"
#include "Common/GameLogicUtilities.hpp"
#include "Common/ItemDataProvider.hpp"
#include "Common/PacketBuilder.hpp"
#include "Common/PacketReader.hpp"
#include "Common/PacketWrapper.hpp"
#include "Common/Randomizer.hpp"
#include "Common/Session.hpp"
#include "Common/SplitPacketBuilder.hpp"
#include "Common/StringUtilities.hpp"
#include "Common/TimeUtilities.hpp"
#include "ChannelServer/BuddyListHandler.hpp"
#include "ChannelServer/BuddyListPacket.hpp"
#include "ChannelServer/ChannelServer.hpp"
#include "ChannelServer/ChatHandler.hpp"
#include "ChannelServer/CmsgHeader.hpp"
#include "ChannelServer/CommandHandler.hpp"
#include "ChannelServer/DropHandler.hpp"
#include "ChannelServer/Fame.hpp"
#include "ChannelServer/Instance.hpp"
#include "ChannelServer/Inventory.hpp"
#include "ChannelServer/InventoryHandler.hpp"
#include "ChannelServer/InventoryPacket.hpp"
#include "ChannelServer/KeyMaps.hpp"
#include "ChannelServer/LevelsPacket.hpp"
#include "ChannelServer/Map.hpp"
#include "ChannelServer/MapPacket.hpp"
#include "ChannelServer/Maps.hpp"
#include "ChannelServer/MobHandler.hpp"
#include "ChannelServer/MonsterBookPacket.hpp"
#include "ChannelServer/MysticDoor.hpp"
#include "ChannelServer/Npc.hpp"
#include "ChannelServer/NpcHandler.hpp"
#include "ChannelServer/Party.hpp"
#include "ChannelServer/PartyHandler.hpp"
#include "ChannelServer/Pet.hpp"
#include "ChannelServer/PetHandler.hpp"
#include "ChannelServer/PlayerDataProvider.hpp"
#include "ChannelServer/PlayerHandler.hpp"
#include "ChannelServer/PlayerPacket.hpp"
#include "ChannelServer/Quests.hpp"
#include "ChannelServer/ReactorHandler.hpp"
#include "ChannelServer/ServerPacket.hpp"
#include "ChannelServer/SkillMacros.hpp"
#include "ChannelServer/Skills.hpp"
#include "ChannelServer/Summon.hpp"
#include "ChannelServer/SummonHandler.hpp"
#include "ChannelServer/SyncPacket.hpp"
#include "ChannelServer/TradeHandler.hpp"
#include "ChannelServer/WorldServerSession.hpp"
#include <array>
#include <stdexcept>

namespace Vana {
namespace ChannelServer {

Player::Player() :
	MovableLife{0, Point{}, 0}
{
}

auto Player::handle(PacketReader &reader) -> Result {
	try {
		header_t header = reader.get<header_t>();
		if (!m_isConnect) {
			// We don't want to accept any other packet than the one for loading the character
			if (header == CMSG_PLAYER_LOAD) {
				playerConnect(reader);
			}
		}
		else {
			switch (header) {
				case CMSG_ADMIN_COMMAND: CommandHandler::handleAdminCommand(shared_from_this(), reader); break;
				case CMSG_ADMIN_MESSENGER: PlayerHandler::handleAdminMessenger(shared_from_this(), reader); break;
				case CMSG_ATTACK_ENERGY_CHARGE: PlayerHandler::useEnergyChargeAttack(shared_from_this(), reader); break;
				case CMSG_ATTACK_MAGIC: PlayerHandler::useSpellAttack(shared_from_this(), reader); break;
				case CMSG_ATTACK_MELEE: PlayerHandler::useMeleeAttack(shared_from_this(), reader); break;
				case CMSG_ATTACK_RANGED: PlayerHandler::useRangedAttack(shared_from_this(), reader); break;
				case CMSG_BOMB_SKILL_USE: PlayerHandler::useBombSkill(shared_from_this(), reader); break;
				case CMSG_BUDDY: BuddyListHandler::handleBuddyList(shared_from_this(), reader); break;
				case CMSG_BUFF_ITEM_USE: InventoryHandler::useBuffItem(shared_from_this(), reader); break;
				case CMSG_CASH_ITEM_USE: InventoryHandler::useCashItem(shared_from_this(), reader); break;
				case CMSG_CASH_SHOP: send(Packets::Player::sendBlockedMessage(Packets::Player::BlockMessages::NoCashShop)); break;
				case CMSG_CHAIR: InventoryHandler::handleChair(shared_from_this(), reader); break;
				case CMSG_CHALKBOARD: sendMap(Packets::Inventory::sendChalkboardUpdate(m_id, "")); setChalkboard(""); break;
				case CMSG_CHANNEL_CHANGE: changeChannel(reader.get<int8_t>()); break;
				case CMSG_COMMAND: CommandHandler::handleCommand(shared_from_this(), reader); break;
				case CMSG_DROP_MESOS: DropHandler::dropMesos(shared_from_this(), reader); break;
				case CMSG_EMOTE: PlayerHandler::handleFacialExpression(shared_from_this(), reader); break;
				case CMSG_FAME: Fame::handleFame(shared_from_this(), reader); break;
				case CMSG_FRIENDLY_MOB_DAMAGE: MobHandler::friendlyDamaged(shared_from_this(), reader); break;
				case CMSG_HAMMER: InventoryHandler::handleHammerTime(shared_from_this()); break;
				case CMSG_ITEM_CANCEL: InventoryHandler::cancelItem(shared_from_this(), reader); break;
				case CMSG_ITEM_EFFECT: InventoryHandler::useItemEffect(shared_from_this(), reader); break;
				case CMSG_ITEM_LOOT: DropHandler::lootItem(shared_from_this(), reader); break;
				case CMSG_ITEM_MOVE: InventoryHandler::moveItem(shared_from_this(), reader); break;
				case CMSG_ITEM_USE: InventoryHandler::useItem(shared_from_this(), reader); break;
				case CMSG_KEYMAP: changeKey(reader); break;
				case CMSG_MACRO_LIST: changeSkillMacros(reader); break;
				case CMSG_MAP_CHANGE: Maps::usePortal(shared_from_this(), reader); break;
				case CMSG_MAP_CHANGE_SPECIAL: Maps::useScriptedPortal(shared_from_this(), reader); break;
				case CMSG_MESSAGE_GROUP: ChatHandler::handleGroupChat(shared_from_this(), reader); break;
				case CMSG_MOB_CONTROL: MobHandler::monsterControl(shared_from_this(), reader); break;
				case CMSG_MOB_EXPLOSION: MobHandler::handleBomb(shared_from_this(), reader); break;
				case CMSG_MOB_TURNCOAT_DAMAGE: MobHandler::handleTurncoats(shared_from_this(), reader); break;
				case CMSG_MONSTER_BOOK: PlayerHandler::handleMonsterBook(shared_from_this(), reader); break;
				case CMSG_MTS: send(Packets::Player::sendBlockedMessage(Packets::Player::BlockMessages::MtsUnavailable)); break;
				case CMSG_MULTI_STAT_ADDITION: getStats()->addStatMulti(reader); break;
				case CMSG_MYSTIC_DOOR_ENTRY: PlayerHandler::handleDoorUse(shared_from_this(), reader); break;
				case CMSG_NPC_ANIMATE: NpcHandler::handleNpcAnimation(shared_from_this(), reader); break;
				case CMSG_NPC_TALK: NpcHandler::handleNpc(shared_from_this(), reader); break;
				case CMSG_NPC_TALK_CONT: NpcHandler::handleNpcIn(shared_from_this(), reader); break;
				case CMSG_PARTY: PartyHandler::handleRequest(shared_from_this(), reader); break;
				case CMSG_PET_CHAT: PetHandler::handleChat(shared_from_this(), reader); break;
				case CMSG_PET_COMMAND: PetHandler::handleCommand(shared_from_this(), reader); break;
				case CMSG_PET_CONSUME_POTION: PetHandler::handleConsumePotion(shared_from_this(), reader); break;
				case CMSG_PET_FOOD_USE: PetHandler::handleFeed(shared_from_this(), reader); break;
				case CMSG_PET_LOOT: DropHandler::petLoot(shared_from_this(), reader); break;
				case CMSG_PET_MOVEMENT: PetHandler::handleMovement(shared_from_this(), reader); break;
				case CMSG_PET_SUMMON: PetHandler::handleSummon(shared_from_this(), reader); break;
				case CMSG_PLAYER_CHAT: ChatHandler::handleChat(shared_from_this(), reader); break;
				case CMSG_PLAYER_DAMAGE: PlayerHandler::handleDamage(shared_from_this(), reader); break;
				case CMSG_PLAYER_HEAL: PlayerHandler::handleHeal(shared_from_this(), reader); break;
				case CMSG_PLAYER_INFO: PlayerHandler::handleGetInfo(shared_from_this(), reader); break;
				case CMSG_PLAYER_MOVE: PlayerHandler::handleMoving(shared_from_this(), reader); break;
				case CMSG_PLAYER_ROOM: TradeHandler::tradeHandler(shared_from_this(), reader); break;
				case CMSG_QUEST_OBTAIN: Quests::getQuest(shared_from_this(), reader); break;
				case CMSG_REACTOR_HIT: ReactorHandler::hitReactor(shared_from_this(), reader); break;
				case CMSG_REACTOR_TOUCH: ReactorHandler::touchReactor(shared_from_this(), reader); break;
				case CMSG_REVIVE_EFFECT: InventoryHandler::useItemEffect(shared_from_this(), reader); break;
				case CMSG_SCROLL_USE: InventoryHandler::useScroll(shared_from_this(), reader); break;
				case CMSG_SHOP: NpcHandler::useShop(shared_from_this(), reader); break;
				case CMSG_SKILL_ADD: Skills::addSkill(shared_from_this(), reader); break;
				case CMSG_SKILL_CANCEL: Skills::cancelSkill(shared_from_this(), reader); break;
				case CMSG_SKILL_USE: Skills::useSkill(shared_from_this(), reader); break;
				case CMSG_SKILLBOOK_USE: InventoryHandler::useSkillbook(shared_from_this(), reader); break;
				case CMSG_SPECIAL_SKILL: PlayerHandler::handleSpecialSkills(shared_from_this(), reader); break;
				case CMSG_STAT_ADDITION: getStats()->addStat(reader); break;
				case CMSG_STORAGE: NpcHandler::useStorage(shared_from_this(), reader); break;
				case CMSG_SUMMON_ATTACK: PlayerHandler::useSummonAttack(shared_from_this(), reader); break;
				case CMSG_SUMMON_BAG_USE: InventoryHandler::useSummonBag(shared_from_this(), reader); break;
				case CMSG_SUMMON_DAMAGE: SummonHandler::damageSummon(shared_from_this(), reader); break;
				case CMSG_SUMMON_MOVEMENT: SummonHandler::moveSummon(shared_from_this(), reader); break;
				case CMSG_SUMMON_SKILL: SummonHandler::summonSkill(shared_from_this(), reader); break;
				case CMSG_TELEPORT_ROCK: InventoryHandler::handleRockFunctions(shared_from_this(), reader); break;
				case CMSG_TELEPORT_ROCK_USE: InventoryHandler::handleRockTeleport(shared_from_this(), Items::SpecialTeleportRock, reader); break;
				case CMSG_TOWN_SCROLL_USE: InventoryHandler::useReturnScroll(shared_from_this(), reader); break;
				case CMSG_USE_CHAIR: InventoryHandler::useChair(shared_from_this(), reader); break;
				case CMSG_USE_REWARD_ITEM: InventoryHandler::handleRewardItem(shared_from_this(), reader); break;
				case CMSG_USE_SCRIPT_ITEM: InventoryHandler::handleScriptItem(shared_from_this(), reader); break;
#ifdef DEBUG
				case CMSG_PONG:
				case CMSG_PLAYER_MOB_DISTANCE:
				case CMSG_PLAYER_UNK_MAP:
				case CMSG_PLAYER_BOAT_MAP:
				case CMSG_PLAYER_UNK_MAP2:
				case CMSG_ADMIN_COMMAND_LOG:
				case CMSG_BUFF_ACKNOWLEDGE:
				case CMSG_MAP_TELEPORT:
					// Do nothing
					break;
				default:
					std::cout << "Unhandled 0x" << std::hex << std::setw(4) << header << ": " << reader << std::endl;
#endif
			}
		}
	}
	catch (const PacketContentException &e) {
		// Packet data didn't match the packet length somewhere
		// This isn't always evidence of tampering with packets
		// We may not process the structure properly

		reader.reset();
		ChannelServer::getInstance().log(LogType::MalformedPacket, [&](out_stream_t &log) {
			log << "Player ID: " << getId()
				<< "; Packet: " << reader
				<< "; Error: " << e.what();
		});
		disconnect();
		return Result::Failure;
	}

	return Result::Successful;
}

auto Player::onDisconnect() -> void {
	m_disconnecting = true;

	Map *curMap = Maps::getMap(m_map);
	if (getMapChair() != 0) {
		curMap->playerSeated(getMapChair(), nullptr);
	}

	curMap->removePlayer(shared_from_this());
	m_isConnect = false;

	if (isTrading()) {
		TradeHandler::cancelTrade(shared_from_this());
	}

	bool isLeader = false;
	if (Party *party = getParty()) {
		isLeader = party->isLeader(getId());
	}

	getSkills()->onDisconnect();

	if (Instance *instance = getInstance()) {
		instance->removePlayer(getId());
		instance->playerDisconnect(getId(), isLeader);
	}
	//if (this->getStats()->isDead()) {
	//	this->acceptDeath();
	//}
	// "Bug" in global, would be fixed here:
	// When disconnecting and dead, you actually go back to forced return map before the death return map
	// (that means that it's parsed while logging in, not while logging out)
	if (const PortalInfo * const closest = curMap->getNearestSpawnPoint(getPos())) {
		m_mapPos = closest->id;
	}

	if (m_saveOnDc) {
		saveAll(true);
		setOnline(false);
	}

	if (ChannelServer::getInstance().isConnected()) {
		// Do not connect to worldserver if the worldserver has disconnected
		ChannelServer::getInstance().sendWorld(Packets::Interserver::Player::disconnect(getId()));
	}

	ChannelServer::getInstance().getPlayerDataProvider().removePlayer(shared_from_this());

	// Important: Clean up all external pointers that may be pointing to this at the class level before finalizing
	m_npc.reset();
	ChannelServer::getInstance().finalizePlayer(shared_from_this());
}

auto Player::playerConnect(PacketReader &reader) -> void {
	player_id_t id = reader.get<player_id_t>();
	bool hasTransferPacket = false;
	auto &channel = ChannelServer::getInstance();
	auto &provider = channel.getPlayerDataProvider();
	if (provider.checkPlayer(id, getIp().get(), hasTransferPacket) == Result::Failure) {
		// Hacking
		disconnect();
		return;
	}

	m_id = id;
	auto &db = Database::getCharDb();
	auto &sql = db.getSession();
	soci::row row;
	sql.once
		<< "SELECT c.*, u.gm_level, u.admin "
		<< "FROM " << db.makeTable("characters") << " c "
		<< "INNER JOIN " << db.makeTable("accounts") << " u ON c.account_id = u.account_id "
		<< "WHERE c.character_id = :char",
		soci::use(id, "char"),
		soci::into(row);

	if (!sql.got_data()) {
		// Hacking
		disconnect();
		return;
	}

	m_name = row.get<string_t>("name");
	m_accountId = row.get<account_id_t>("account_id");
	m_map = row.get<map_id_t>("map");
	m_gmLevel = row.get<int32_t>("gm_level");
	m_admin = row.get<bool>("admin");
	m_face = row.get<face_id_t>("face");
	m_hair = row.get<hair_id_t>("hair");
	m_worldId = row.get<world_id_t>("world_id");
	m_gender = row.get<gender_id_t>("gender");
	m_skin = row.get<skin_id_t>("skin");
	m_mapPos = row.get<portal_id_t>("pos");
	m_buddylistSize = row.get<uint8_t>("buddylist_size");

	// Stats
	m_stats = make_owned_ptr<PlayerStats>(
		this,
		row.get<player_level_t>("level"),
		row.get<job_id_t>("job"),
		row.get<fame_t>("fame"),
		row.get<stat_t>("str"),
		row.get<stat_t>("dex"),
		row.get<stat_t>("int"),
		row.get<stat_t>("luk"),
		row.get<stat_t>("ap"),
		row.get<health_ap_t>("hpmp_ap"),
		row.get<stat_t>("sp"),
		row.get<health_t>("chp"),
		row.get<health_t>("mhp"),
		row.get<health_t>("cmp"),
		row.get<health_t>("mmp"),
		row.get<experience_t>("exp")
	);

	// Inventory
	m_mounts = make_owned_ptr<PlayerMounts>(this);
	m_pets = make_owned_ptr<PlayerPets>(this);
	array_t<inventory_slot_count_t, Inventories::InventoryCount> maxSlots;
	maxSlots[0] = row.get<inventory_slot_count_t>("equip_slots");
	maxSlots[1] = row.get<inventory_slot_count_t>("use_slots");
	maxSlots[2] = row.get<inventory_slot_count_t>("setup_slots");
	maxSlots[3] = row.get<inventory_slot_count_t>("etc_slots");
	maxSlots[4] = row.get<inventory_slot_count_t>("cash_slots");
	m_inventory = make_owned_ptr<PlayerInventory>(this, maxSlots, row.get<mesos_t>("mesos"));
	m_storage = make_owned_ptr<PlayerStorage>(this);

	// Skills
	m_skills = make_owned_ptr<PlayerSkills>(this);

	// Buffs/summons
	m_activeBuffs = make_owned_ptr<PlayerActiveBuffs>(this);
	m_summons = make_owned_ptr<PlayerSummons>(this);

	bool firstConnect = !hasTransferPacket;
	auto &config = channel.getConfig();
	if (hasTransferPacket) {
		parseTransferPacket(provider.getPacket(m_id));
	}
	else {
		// No packet, that means that they're connecting for the first time
		setConnectionTime(time(nullptr));
		m_gmChat = isGm() && config.defaultGmChatMode;
	}

	provider.playerEstablished(id);

	// The rest
	m_variables = make_owned_ptr<PlayerVariables>(this);
	m_buddyList = make_owned_ptr<PlayerBuddyList>(this);
	m_quests = make_owned_ptr<PlayerQuests>(this);
	m_monsterBook = make_owned_ptr<PlayerMonsterBook>(this);

	opt_int32_t bookCover = row.get<opt_int32_t>("book_cover");
	getMonsterBook()->setCover(bookCover.get(0));

	// Key Maps and Macros
	KeyMaps keyMaps;
	keyMaps.load(id);

	SkillMacros skillMacros;
	skillMacros.load(id);

	// Adjust down HP or MP if necessary
	getStats()->checkHpMp();

	if (isGm() || isAdmin()) {
		if (firstConnect) {
			m_map = Vana::Maps::GmMap;
			m_mapPos = -1;
		}
	}
	else if (Maps::getMap(m_map)->getForcedReturn() != Vana::Maps::NoMap) {
		m_map = Maps::getMap(m_map)->getForcedReturn();
		m_mapPos = -1;
	}
	else if (getStats()->isDead()) {
		m_map = Maps::getMap(m_map)->getReturnMap();
		m_mapPos = -1;
	}
	m_lastMap = m_map;

	m_pos = Maps::getMap(m_map)->getSpawnPoint(m_mapPos)->pos;
	m_stance = 0;
	m_foothold = 0;

	send(Packets::Player::connectData(shared_from_this()));

	if (!config.scrollingHeader.empty()) {
		send(Packets::showScrollingHeader(config.scrollingHeader));
	}

	for (int8_t i = 0; i < Inventories::MaxPetCount; i++) {
		if (Pet *pet = getPets()->getSummoned(i)) {
			pet->setPos(Maps::getMap(m_map)->getSpawnPoint(m_mapPos)->pos);
		}
	}

	send(Packets::Player::showKeys(&keyMaps));

	send(Packets::Buddy::update(shared_from_this(), Packets::Buddy::ActionTypes::Add));
	getBuddyList()->checkForPendingBuddy();

	send(Packets::Player::showSkillMacros(&skillMacros));

	provider.addPlayer(shared_from_this());
	Maps::addPlayer(shared_from_this(), m_map);

	channel.log(LogType::Info, [&](out_stream_t &log) {
		log << m_name << " (" << m_id << ") connected from " << getIp();
	});

	setOnline(true);
	m_isConnect = true;

	PlayerData data;
	const PlayerData * const existingData = provider.getPlayerData(m_id);
	bool firstConnectionSinceServerStarted = firstConnect && !existingData->initialized;

	if (firstConnectionSinceServerStarted) {
		data.admin = m_admin;
		data.level = getStats()->getLevel();
		data.job = getStats()->getJob();
		data.gmLevel = m_gmLevel;
		data.name = m_name;
		data.mutualBuddies = m_buddyList->getBuddyIds();
	}

	data.channel = channel.getChannelId();
	data.map = m_map;
	data.id = m_id;
	data.ip = getIp().get();

	channel.sendWorld(Packets::Interserver::Player::connect(data, firstConnectionSinceServerStarted));
}

auto Player::getMap() const -> Map * {
	return Maps::getMap(getMapId());
}

auto Player::internalSetMap(map_id_t mapId, portal_id_t portalId, const Point &pos, bool fromPosition) -> void {
	Map *oldMap = Maps::getMap(m_map);
	Map *newMap = Maps::getMap(mapId);

	oldMap->removePlayer(shared_from_this());
	if (m_map != mapId) {
		m_lastMap = m_map;
	}
	m_map = mapId;
	m_mapPos = portalId;
	m_usedPortals.clear();
	setPos(pos);
	setStance(0);
	setFoothold(0);
	setFallCounter(0);

	// Prevent chair Denial of Service
	if (getMapChair() != 0) {
		oldMap->playerSeated(getMapChair(), nullptr);
		setMapChair(0);
	}
	if (getChair() != 0) {
		setChair(0);
	}

	for (int8_t i = 0; i < Inventories::MaxPetCount; i++) {
		if (Pet *pet = getPets()->getSummoned(i)) {
			pet->setPos(pos);
		}
	}

	getSummons()->changedMap();

	if (!getChalkboard().empty() && !newMap->canChalkboard()) {
		setChalkboard("");
	}

	send(Packets::Map::changeMap(shared_from_this(), fromPosition, getPos()));
	Maps::addPlayer(shared_from_this(), mapId);

	ChannelServer::getInstance().getPlayerDataProvider().updatePlayerMap(shared_from_this());
}

auto Player::setMap(map_id_t mapId, portal_id_t portalId, const Point &pos) -> void {
	internalSetMap(mapId, portalId, pos, true);
}

auto Player::setMap(map_id_t mapId, const PortalInfo * const portal, bool instance) -> void {
	if (!Maps::getMap(mapId)) {
		send(Packets::Map::portalBlocked());
		return;
	}
	Map *oldMap = Maps::getMap(m_map);
	Map *newMap = Maps::getMap(mapId);

	const PortalInfo * const actualPortal = portal != nullptr ?
		portal :
		newMap->getSpawnPoint();

	if (!instance) {
		// Only trigger the message for natural map changes not caused by moveAllPlayers, etc.
		bool isPartyLeader = getParty() != nullptr && getParty()->isLeader(getId());
		if (Instance *i = oldMap->getInstance()) {
			i->playerChangeMap(getId(), mapId, m_map, isPartyLeader);
		}
		if (Instance *i = newMap->getInstance()) {
			i->playerChangeMap(getId(), mapId, m_map, isPartyLeader);
		}
	}

	internalSetMap(mapId, actualPortal->id, Point{actualPortal->pos.x, actualPortal->pos.y - 40}, false);
}

auto Player::getMedalName() -> string_t {
	out_stream_t ret;
	if (item_id_t itemId = getInventory()->getEquippedId(EquipSlots::Medal)) {
		// Check if there's an item at that slot
		ret << "<" << ChannelServer::getInstance().getItemDataProvider().getItemInfo(itemId)->name << "> ";
	}
	ret << getName();
	return ret.str();
}

auto Player::changeChannel(channel_id_t channel) -> void {
	ChannelServer::getInstance().sendWorld(Packets::Interserver::Player::changeChannel(shared_from_this(), channel));
}

auto Player::getTransferPacket() const -> PacketBuilder {
	PacketBuilder builder;
	builder
		.add<int64_t>(getConnectionTime())
		.add<player_id_t>(m_follow != nullptr ? m_follow->getId() : 0)
		.add<bool>(m_gmChat)
		.addBuffer(getActiveBuffs()->getTransferPacket())
		.addBuffer(getSummons()->getTransferPacket());

	return builder;
}

auto Player::parseTransferPacket(PacketReader &reader) -> void {
	setConnectionTime(reader.get<int64_t>());
	player_id_t followId = reader.get<player_id_t>();
	if (followId != 0) {
		auto &provider = ChannelServer::getInstance().getPlayerDataProvider();
		if (auto follow = provider.getPlayer(followId)) {
			provider.addFollower(shared_from_this(), follow);
		}
	}

	m_gmChat = reader.get<bool>();

	getActiveBuffs()->parseTransferPacket(reader);
	getSummons()->parseTransferPacket(reader);
}

auto Player::changeKey(PacketReader &reader) -> void {
	int32_t mode = reader.get<int32_t>();
	int32_t howMany = reader.get<int32_t>();

	enum KeyModes : int32_t {
		ChangeKeys = 0x00,
		AutoHpPotion = 0x01,
		AutoMpPotion = 0x02
	};

	if (mode == ChangeKeys) {
		if (howMany == 0) {
			return;
		}

		KeyMaps keyMaps;
		keyMaps.load(m_id);
		for (int32_t i = 0; i < howMany; i++) {
			int32_t pos = reader.get<int32_t>();
			KeyMapType type;
			if (EnumUtilities::tryCastFromUnderlying(reader.get<int8_t>(), type) != Result::Successful) {
				// Probably hacking
				return;
			}
			int32_t action = reader.get<int32_t>();
			keyMaps.add(pos, KeyMaps::KeyMap{type, action});
		}

		keyMaps.save(m_id);
	}
	else if (mode == AutoHpPotion) {
		getInventory()->setAutoHpPot(howMany);
	}
	else if (mode == AutoMpPotion) {
		getInventory()->setAutoMpPot(howMany);
	}
}

auto Player::changeSkillMacros(PacketReader &reader) -> void {
	uint8_t num = reader.get<uint8_t>();
	if (num == 0) {
		return;
	}
	SkillMacros skillMacros;
	for (uint8_t i = 0; i < num; i++) {
		string_t name = reader.get<string_t>();
		bool shout = reader.get<bool>();
		skill_id_t skill1 = reader.get<skill_id_t>();
		skill_id_t skill2 = reader.get<skill_id_t>();
		skill_id_t skill3 = reader.get<skill_id_t>();

		skillMacros.add(i, new SkillMacros::SkillMacro(name, shout, skill1, skill2, skill3));
	}
	skillMacros.save(getId());
}

auto Player::setHair(hair_id_t id) -> void {
	m_hair = id;
	send(Packets::Player::updateStat(Stats::Hair, id));
}

auto Player::setFace(face_id_t id) -> void {
	m_face = id;
	send(Packets::Player::updateStat(Stats::Face, id));
}

auto Player::setSkin(skin_id_t id) -> void {
	m_skin = id;
	send(Packets::Player::updateStat(Stats::Skin, id));
}

auto Player::saveStats() -> void {
	PlayerStats *s = getStats();
	PlayerInventory *i = getInventory();
	// Need local bindings
	// Stats
	player_level_t level = s->getLevel();
	job_id_t job = s->getJob();
	stat_t str = s->getStr();
	stat_t dex = s->getDex();
	stat_t intt = s->getInt();
	stat_t luk = s->getLuk();
	health_t hp = s->getHp();
	health_t maxHp = s->getMaxHp(true);
	health_t mp = s->getMp();
	health_t maxMp = s->getMaxMp(true);
	health_ap_t hpMpAp = s->getHpMpAp();
	stat_t ap = s->getAp();
	stat_t sp = s->getSp();
	fame_t fame = s->getFame();
	experience_t exp = s->getExp();
	// Inventory
	inventory_slot_count_t equip = i->getMaxSlots(Inventories::EquipInventory);
	inventory_slot_count_t use = i->getMaxSlots(Inventories::UseInventory);
	inventory_slot_count_t setup = i->getMaxSlots(Inventories::SetupInventory);
	inventory_slot_count_t etc = i->getMaxSlots(Inventories::EtcInventory);
	inventory_slot_count_t cash = i->getMaxSlots(Inventories::CashInventory);
	mesos_t money = i->getMesos();
	// Other
	int32_t rawCover = getMonsterBook()->getCover();
	opt_int32_t cover;
	if (rawCover != 0) {
		cover = rawCover;
	}

	auto &db = Database::getCharDb();
	auto &sql = db.getSession();
	sql.once
		<< "UPDATE " << db.makeTable("characters") << " "
		<< "SET "
		<< "	level = :level, "
		<< "	job = :job, "
		<< "	str = :str, "
		<< "	dex = :dex, "
		<< "	`int` = :int, "
		<< "	luk = :luk, "
		<< "	chp = :hp, "
		<< "	mhp = :maxhp, "
		<< "	cmp = :mp, "
		<< "	mmp = :maxmp, "
		<< "	hpmp_ap = :hpmpap, "
		<< "	ap = :ap, "
		<< "	sp = :sp, "
		<< "	exp = :exp, "
		<< "	fame = :fame, "
		<< "	map = :map, "
		<< "	pos = :pos, "
		<< "	gender = :gender, "
		<< "	skin = :skin, "
		<< "	face = :face, "
		<< "	hair = :hair, "
		<< "	mesos = :money, "
		<< "	equip_slots = :equip, "
		<< "	use_slots = :use, "
		<< "	setup_slots = :setup, "
		<< "	etc_slots = :etc, "
		<< "	cash_slots = :cash, "
		<< "	buddylist_size = :buddylist, "
		<< "	book_cover = :cover "
		<< "WHERE character_id = :char",
		soci::use(m_id, "char"),
		soci::use(level, "level"),
		soci::use(job, "job"),
		soci::use(str, "str"),
		soci::use(dex, "dex"),
		soci::use(intt, "int"),
		soci::use(luk, "luk"),
		soci::use(hp, "hp"),
		soci::use(maxHp, "maxhp"),
		soci::use(mp, "mp"),
		soci::use(maxMp, "maxmp"),
		soci::use(hpMpAp, "hpmpap"),
		soci::use(ap, "ap"),
		soci::use(sp, "sp"),
		soci::use(exp, "exp"),
		soci::use(fame, "fame"),
		soci::use(m_map, "map"),
		soci::use(m_mapPos, "pos"),
		soci::use(m_gender, "gender"),
		soci::use(m_skin, "skin"),
		soci::use(m_face, "face"),
		soci::use(m_hair, "hair"),
		soci::use(money, "money"),
		soci::use(equip, "equip"),
		soci::use(use, "use"),
		soci::use(setup, "setup"),
		soci::use(etc, "etc"),
		soci::use(cash, "cash"),
		soci::use(m_buddylistSize, "buddylist"),
		soci::use(cover, "cover");
}

auto Player::saveAll(bool saveCooldowns) -> void {
	saveStats();
	getInventory()->save();
	getStorage()->save();
	getMonsterBook()->save();
	getMounts()->save();
	getPets()->save();
	getQuests()->save();
	getSkills()->save(saveCooldowns);
	getVariables()->save();
}

auto Player::setOnline(bool online) -> void {
	int32_t onlineId = online ? ChannelServer::getInstance().getOnlineId() : 0;
	auto &db = Database::getCharDb();
	auto &sql = db.getSession();
	sql.once
		<< "UPDATE " << db.makeTable("accounts") << " u "
		<< "INNER JOIN " << db.makeTable("characters") << " c ON u.account_id = c.account_id "
		<< "SET "
		<< "	u.online = :onlineId, "
		<< "	c.online = :online "
		<< "WHERE c.character_id = :char",
		soci::use(m_id, "char"),
		soci::use(online, "online"),
		soci::use(onlineId, "onlineId");
}

auto Player::setLevelDate() -> void {
	auto &db = Database::getCharDb();
	auto &sql = db.getSession();
	sql.once << "UPDATE " << db.makeTable("characters") << " c SET c.time_level = NOW() WHERE c.character_id = :char", soci::use(m_id, "char");
}

auto Player::acceptDeath(bool wheel) -> void {
	// TODO FIXME verify
	// Is this correct? The old line was:
	// map_id_t toMap = Maps::getMap(m_map) ? Maps::getMap(m_map)->getReturnMap() : m_map;
	// That doesn't seem very useful to me since Maps::getMap(m_map) should always be true, otherwise how did the player get to the map?

	map_id_t toMap = Maps::getMap(m_map) != nullptr ? Maps::getMap(m_map)->getReturnMap() : m_map;
	if (wheel) {
		toMap = getMapId();
	}
	getActiveBuffs()->removeBuffs();
	getStats()->checkHpMp();
	getStats()->setHp(50, false);
	setMap(toMap);
}

auto Player::hasGmEquip() const -> bool {
	auto equippedUtility = [this](inventory_slot_t slot, item_id_t itemId) -> bool {
		return this->getInventory()->getEquippedId(slot) == itemId;
	};

	if (equippedUtility(EquipSlots::Helm, Items::GmHat)) {
		return true;
	}
	if (equippedUtility(EquipSlots::Top, Items::GmTop)) {
		return true;
	}
	if (equippedUtility(EquipSlots::Bottom, Items::GmBottom)) {
		return true;
	}
	if (equippedUtility(EquipSlots::Weapon, Items::GmWeapon)) {
		return true;
	}
	return false;
}

auto Player::isUsingGmHide() const -> bool {
	return m_activeBuffs->isUsingGmHide();
}

auto Player::hasGmBenefits() const -> bool {
	return isUsingGmHide() || hasGmEquip();
}

auto Player::setBuddyListSize(uint8_t size) -> void {
	m_buddylistSize = size;
	send(Packets::Buddy::showSize(shared_from_this()));
}

auto Player::getPortalCount(bool add) -> portal_count_t {
	if (add) {
		m_portalCount++;
	}
	return m_portalCount;
}

auto Player::initializeRng(PacketBuilder &builder) -> void {
	uint32_t seed1 = Randomizer::rand<uint32_t>();
	uint32_t seed2 = Randomizer::rand<uint32_t>();
	uint32_t seed3 = Randomizer::rand<uint32_t>();

	m_randStream = make_owned_ptr<TauswortheGenerator>(seed1, seed2, seed3);

	builder.add<uint32_t>(seed1);
	builder.add<uint32_t>(seed2);
	builder.add<uint32_t>(seed3);
}

auto Player::send(const PacketBuilder &builder) -> void {
	// TODO FIXME resource
	if (isDisconnecting()) return;
	PacketHandler::send(builder);
}

auto Player::send(const SplitPacketBuilder &builder) -> void {
	// TODO FIXME resource
	if (isDisconnecting()) return;
	send(builder.player);
}

auto Player::sendMap(const PacketBuilder &builder, bool excludeSelf) -> void {
	getMap()->send(builder, excludeSelf ? shared_from_this() : nullptr);
}

auto Player::sendMap(const SplitPacketBuilder &builder) -> void {
	getMap()->send(builder, shared_from_this());
}

}
}