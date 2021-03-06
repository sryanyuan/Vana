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
#include "ChannelServer.hpp"
#include "Common/ConfigFile.hpp"
#include "Common/ConnectionListenerConfig.hpp"
#include "Common/ConnectionManager.hpp"
#include "Common/ExitCodes.hpp"
#include "Common/InitializeCommon.hpp"
#include "Common/MiscUtilities.hpp"
#include "Common/PacketBuilder.hpp"
#include "Common/ServerType.hpp"
#include "ChannelServer/ChatHandler.hpp"
#include "ChannelServer/Map.hpp"
#include "ChannelServer/Player.hpp"
#include "ChannelServer/PlayerDataProvider.hpp"
#include "ChannelServer/ServerPacket.hpp"
#include "ChannelServer/SyncPacket.hpp"
#include "ChannelServer/WorldServerPacket.hpp"

namespace Vana {
namespace ChannelServer {

ChannelServer::ChannelServer() :
	AbstractServer{ServerType::Channel},
	m_worldIp{0}
{
}

auto ChannelServer::listen() -> void {
	m_sessionPool.initialize(1);

	auto &config = getInterServerConfig();
	getConnectionManager().listen(
		ConnectionListenerConfig{
			config.clientPing,
			config.clientEncryption,
			ConnectionType::EndUser,
			MapleVersion::ChannelSubversion,
			m_port,
			Ip::Type::Ipv4
		},
		[&] { return make_ref_ptr<Player>(); }
	);

	Initializing::setUsersOffline(this, getOnlineId());
}

auto ChannelServer::finalizePlayer(ref_ptr_t<Player> session) -> void {
	m_sessionPool.store(session);
}

auto ChannelServer::shutdown() -> void {
	// If we don't do this and the connection disconnects, it will try to call shutdown() again
	m_channelId = -1;
	AbstractServer::shutdown();
}

auto ChannelServer::loadData() -> Result {
	if (Initializing::checkSchemaVersion(this) == Result::Failure) {
		return Result::Failure;
	}
	if (Initializing::checkMcdbVersion(this) == Result::Failure) {
		return Result::Failure;
	}

	m_buffDataProvider.loadData();
	m_validCharDataProvider.loadData();
	m_equipDataProvider.loadData();
	m_curseDataProvider.loadData();
	m_npcDataProvider.loadData();
	m_dropDataProvider.loadData();
	m_beautyDataProvider.loadData();
	m_mobDataProvider.loadData();
	m_scriptDataProvider.loadData();
	m_skillDataProvider.loadData();
	m_reactorDataProvider.loadData();
	m_shopDataProvider.loadData();
	m_questDataProvider.loadData();
	m_itemDataProvider.loadData(m_buffDataProvider);
	m_mapDataProvider.loadData();
	m_eventDataProvider.loadData();

	std::cout << std::setw(Initializing::OutputWidth) << std::left << "Initializing Commands... ";
	ChatHandler::initializeCommands();
	std::cout << "DONE" << std::endl;

	auto &config = getInterServerConfig();
	auto result = getConnectionManager().connect(
		config.loginIp,
		config.loginPort,
		config.serverPing,
		ServerType::Channel,
		[&] {
			return make_ref_ptr<LoginServerSession>();
		});

	if (result.first == Result::Failure) {
		return Result::Failure;
	}

	sendAuth(result.second);
	return Result::Successful;
}

auto ChannelServer::reloadData(const string_t &args) -> void {
	if (args == "all") {
		m_itemDataProvider.loadData(m_buffDataProvider);
		m_dropDataProvider.loadData();
		m_shopDataProvider.loadData();
		m_mobDataProvider.loadData();
		m_beautyDataProvider.loadData();
		m_scriptDataProvider.loadData();
		m_skillDataProvider.loadData();
		m_reactorDataProvider.loadData();
		m_questDataProvider.loadData();
	}
	else if (args == "items") m_itemDataProvider.loadData(m_buffDataProvider);
	else if (args == "drops") m_dropDataProvider.loadData();
	else if (args == "shops") m_shopDataProvider.loadData();
	else if (args == "mobs") m_mobDataProvider.loadData();
	else if (args == "beauty") m_beautyDataProvider.loadData();
	else if (args == "scripts") m_scriptDataProvider.loadData();
	else if (args == "skills") m_skillDataProvider.loadData();
	else if (args == "reactors") m_reactorDataProvider.loadData();
	else if (args == "quest") m_questDataProvider.loadData();
}

auto ChannelServer::makeLogIdentifier() const -> opt_string_t {
	return buildLogIdentifier([&](out_stream_t &id) {
		id << "World: " << static_cast<int32_t>(m_worldId) << "; ID: " << static_cast<int32_t>(m_channelId);
	});
}

auto ChannelServer::getLogPrefix() const -> string_t {
	return "channel";
}

auto ChannelServer::connectToWorld(world_id_t worldId, port_t port, const Ip &ip) -> Result {
	m_worldId = worldId;
	m_worldPort = port;
	m_worldIp = ip;

	auto &config = getInterServerConfig();
	auto result = getConnectionManager().connect(
		ip,
		port,
		config.serverPing,
		ServerType::Channel,
		[&] {
			return make_ref_ptr<WorldServerSession>();
		});

	if (result.first == Result::Failure) {
		return Result::Failure;
	}

	sendAuth(result.second);
	return Result::Successful;
}

auto ChannelServer::onConnectToLogin(ref_ptr_t<LoginServerSession> session) -> void {
	m_loginConnection = session;
}

auto ChannelServer::onDisconnectFromLogin() -> void {
	m_loginConnection.reset();
}

auto ChannelServer::onConnectToWorld(ref_ptr_t<WorldServerSession> session) -> void {
	m_worldConnection = session;
}

auto ChannelServer::onDisconnectFromWorld() -> void {
	m_worldConnection.reset();

	if (isConnected()) {
		log(LogType::ServerDisconnect, "Disconnected from the WorldServer. Shutting down...");
		m_playerDataProvider.disconnect();
		ExitCodes::exit(ExitCodes::ServerDisconnection);
	}
}

auto ChannelServer::establishedWorldConnection(channel_id_t channelId, port_t port, const WorldConfig &config) -> void {
	m_channelId = channelId;

	log(LogType::ServerConnect, [&](out_stream_t &str) {
		str << "Handling channel " << static_cast<int32_t>(channelId) << " on port " << port;
	});

	m_port = port;
	m_config = config;
	Map::setMapUnloadTime(config.mapUnloadTime);
	listen();
	displayLaunchTime();
}

auto ChannelServer::getConfig() const -> const WorldConfig & {
	return m_config;
}

auto ChannelServer::getValidCharDataProvider() const -> const ValidCharDataProvider & {
	return m_validCharDataProvider;
}

auto ChannelServer::getEquipDataProvider() const -> const EquipDataProvider & {
	return m_equipDataProvider;
}

auto ChannelServer::getCurseDataProvider() const -> const CurseDataProvider & {
	return m_curseDataProvider;
}

auto ChannelServer::getNpcDataProvider() const -> const NpcDataProvider & {
	return m_npcDataProvider;
}

auto ChannelServer::getMobDataProvider() const -> const MobDataProvider & {
	return m_mobDataProvider;
}

auto ChannelServer::getBeautyDataProvider() const -> const BeautyDataProvider & {
	return m_beautyDataProvider;
}

auto ChannelServer::getDropDataProvider() const -> const DropDataProvider & {
	return m_dropDataProvider;
}

auto ChannelServer::getSkillDataProvider() const -> const SkillDataProvider & {
	return m_skillDataProvider;
}

auto ChannelServer::getShopDataProvider() const -> const ShopDataProvider & {
	return m_shopDataProvider;
}

auto ChannelServer::getScriptDataProvider() const -> const ScriptDataProvider & {
	return m_scriptDataProvider;
}

auto ChannelServer::getReactorDataProvider() const -> const ReactorDataProvider & {
	return m_reactorDataProvider;
}

auto ChannelServer::getItemDataProvider() const -> const ItemDataProvider & {
	return m_itemDataProvider;
}

auto ChannelServer::getQuestDataProvider() const -> const QuestDataProvider & {
	return m_questDataProvider;
}

auto ChannelServer::getBuffDataProvider() const -> const BuffDataProvider & {
	return m_buffDataProvider;
}

auto ChannelServer::getEventDataProvider() const -> const EventDataProvider & {
	return m_eventDataProvider;
}

auto ChannelServer::getMapDataProvider() const -> const MapDataProvider & {
	return m_mapDataProvider;
}

auto ChannelServer::getPlayerDataProvider() -> PlayerDataProvider & {
	return m_playerDataProvider;
}

auto ChannelServer::getTrades() -> Trades & {
	return m_trades;
}

auto ChannelServer::getMapleTvs() -> MapleTvs & {
	return m_mapleTvs;
}

auto ChannelServer::getInstances() -> Instances & {
	return m_instances;
}

auto ChannelServer::getMap(int32_t mapId) -> Map * {
	return m_mapDataProvider.getMap(mapId);
}

auto ChannelServer::unloadMap(int32_t mapId) -> void {
	return m_mapDataProvider.unloadMap(mapId);
}

auto ChannelServer::isConnected() const -> bool {
	return m_channelId != -1;
}

auto ChannelServer::getWorldId() const -> world_id_t {
	return m_worldId;
}

auto ChannelServer::getChannelId() const -> channel_id_t {
	return m_channelId;
}

auto ChannelServer::getOnlineId() const -> int32_t {
	return 20000 + static_cast<int32_t>(m_worldId) * 100 + m_channelId;
}

auto ChannelServer::sendWorld(const PacketBuilder &builder) -> void {
	m_worldConnection->send(builder);
}

auto ChannelServer::setScrollingHeader(const string_t &message) -> void {
	if (m_config.scrollingHeader != message) {
		m_config.scrollingHeader = message;
		m_playerDataProvider.send(Packets::showScrollingHeader(message));
	}
}

auto ChannelServer::modifyRate(int32_t rateType, int32_t newValue) -> void {
	RatesConfig currentRates = m_config.rates;
	if (rateType & RatesConfig::Types::mobExpRate) currentRates.mobExpRate = newValue;
	if (rateType & RatesConfig::Types::questExpRate) currentRates.questExpRate = newValue;
	if (rateType & RatesConfig::Types::dropRate) currentRates.dropRate = newValue;
	if (rateType & RatesConfig::Types::dropMeso) currentRates.dropMeso = newValue;
	if (rateType & RatesConfig::Types::globalDropRate) currentRates.globalDropRate = newValue;
	if (rateType & RatesConfig::Types::globalDropMeso) currentRates.globalDropMeso = newValue;
	sendWorld(Packets::Interserver::Config::modifyRates(currentRates));
}

auto ChannelServer::setRates(const RatesConfig &rates) -> void {
	m_config.rates = rates;
}

auto ChannelServer::setConfig(const WorldConfig &config) -> void {
	setScrollingHeader(config.scrollingHeader);
	if (config.mapUnloadTime != m_config.mapUnloadTime) {
		Map::setMapUnloadTime(config.mapUnloadTime);
	}
	m_config = config;
}

}
}