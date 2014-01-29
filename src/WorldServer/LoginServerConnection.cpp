/*
Copyright (C) 2008-2014 Vana Development Team

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
#include "LoginServerConnection.hpp"
#include "LoginServerConnectHandler.hpp"
#include "InterHeader.hpp"
#include "PacketReader.hpp"
#include "SyncHandler.hpp"
#include "VanaConstants.hpp"
#include "WorldServer.hpp"
#include "WorldServerAcceptHandler.hpp"
#include <iostream>

LoginServerConnection::LoginServerConnection()
{
	setType(ServerType::World);
}

LoginServerConnection::~LoginServerConnection() {
	if (WorldServer::getInstance().isConnected()) {
		std::cout << "Disconnected from the LoginServer. Shutting down..." << std::endl;
		WorldServer::getInstance().shutdown();
	}
}

auto LoginServerConnection::handleRequest(PacketReader &packet) -> void {
	switch (packet.getHeader()) {
		case IMSG_WORLD_CONNECT: LoginServerConnectHandler::connect(this, packet); break;
		case IMSG_NEW_PLAYER: LoginServerConnectHandler::newPlayer(packet); break;
		case IMSG_REHASH_CONFIG: LoginServerConnectHandler::rehashConfig(packet); break;
		case IMSG_TO_CHANNELS: WorldServerAcceptHandler::sendPacketToChannels(packet); break;
		case IMSG_SYNC: SyncHandler::handle(this, packet); break;
	}
}