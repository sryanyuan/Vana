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
#pragma once

#include "AbstractConnection.hpp"
#include "ExternalIp.hpp"
#include "ExternalIpResolver.hpp"
#include "Types.hpp"
#include "VanaConstants.hpp"
#include <string>
#include <vector>

class AbstractServer;
class PacketReader;

class AbstractServerConnection : public AbstractConnection {
public:
	auto sendAuth(const string_t &pass, const IpMatrix &extIp) -> void;
	auto getType() const -> ServerType { return m_type; }
protected:
	AbstractServerConnection()
	{
		m_isServer = true;
	}

	auto setType(ServerType type) -> void { m_type = type; }
private:
	ServerType m_type = ServerType::None;
};

class AbstractServerAcceptConnection : public AbstractConnection {
public:
	auto processAuth(AbstractServer &server, PacketReader &packet) -> Result;
	virtual auto authenticated(ServerType type) -> void = 0;

	auto isAuthenticated() const -> bool { return m_isAuthenticated; }
	auto getType() const -> ServerType { return m_type; }

	auto matchSubnet(const Ip &test) const -> Ip { return m_resolver.matchIpToSubnet(test); }
	auto setExternalIpInformation(const Ip &defaultIp, const IpMatrix &matrix) -> void { m_resolver.setExternalIpInformation(defaultIp, matrix); }
	auto getExternalIps() const -> const IpMatrix & { return m_resolver.getExternalIps(); }
protected:
	AbstractServerAcceptConnection()
	{
		m_isServer = true;
	}

	auto setType(ServerType type) -> void { m_type = type; }
private:
	bool m_isAuthenticated = false;
	ServerType m_type = ServerType::None;
	ExternalIpResolver m_resolver;
};