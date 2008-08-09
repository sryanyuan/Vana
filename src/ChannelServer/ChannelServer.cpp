/*
Copyright (C) 2008 Vana Development Team

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
#include "ChannelServer.h"
#include "Acceptor.h"
#include "Connector.h"
#include "WorldServerConnectPlayer.h"
#include "InitializeChannel.h"
#include "InitializeCommon.h"
#include "Config.h"
#include "PacketCreator.h"
#include "ServerPacket.h"
#include "Player.h"
#include "Players.h"

ChannelServer * ChannelServer::singleton = 0;

void ChannelServer::listen() {
	new Acceptor(port, new PlayerFactory());
	Initializing::setUsersOffline(getOnlineId());
}

void ChannelServer::loadData() {
	Initializing::initializeMySQL();
	Initializing::checkSchemaVersion();
	Initializing::checkVEDBVersion();
	Initializing::initializeMobs();
	Initializing::initializeReactors();
	Initializing::initializeItems();
	Initializing::initializePets();
	Initializing::initializeDrops();
	Initializing::initializeEquips();
	Initializing::initializeShops();
	Initializing::initializeQuests();
	Initializing::initializeSkills();
	Initializing::initializeMaps();

	Connector *c = new Connector(login_ip, login_inter_port, new WorldServerConnectPlayerFactory());
	WorldServerConnectPlayer *loginPlayer = (WorldServerConnectPlayer *) c->getPlayer();
	loginPlayer->setIP(external_ip);
	loginPlayer->sendAuth(inter_password);
}

void ChannelServer::connectWorld() {
	Connector *c = new Connector(world_ip, world_port, new WorldServerConnectPlayerFactory());
	worldPlayer = (WorldServerConnectPlayer *) c->getPlayer();
	worldPlayer->setIP(external_ip);
	worldPlayer->sendAuth(inter_password);
}

void ChannelServer::loadConfig() {
	Config config("conf/channelserver.lua");
	login_ip = config.getString("login_ip");
	login_inter_port = config.getInt("login_inter_port");
	external_ip = config.getString("external_ip"); // External IP

	world = -1; // Will get from login server
	port = -1; // Will get from world server
	channel = -1; // Will get from world server
	exprate = 1; // Will get from world server
	questexprate = 1; // Will get from world server
	mesorate = 1; // Will get from world server
	droprate = 1; // Will get from world server
}

void ChannelServer::shutdown() {
	struct {
		void operator()(Player *player) {
			player->saveAll();
			player->getPacketHandler()->disconnect();
		}
	} saveAndDC;
	
	Players::Instance()->run(saveAndDC);
	exit(0);
}

void ChannelServer::sendToWorld(PacketCreator &packet) {
	worldPlayer->getPacketHandler()->sendPacket(packet);
}

void ChannelServer::setScrollingHeader(const string &message) {
	if (scrollingHeader != message) {
		scrollingHeader = message;
		if (scrollingHeader.size() == 0) {
			ServerPacket::scrollingHeaderOff();
		}
		else {
			ServerPacket::changeScrollingHeader(scrollingHeader);
		}
	}
}
