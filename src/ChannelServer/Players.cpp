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
#include "Players.h"
#include "Player.h"
#include "PlayersPacket.h"
#include "Maps.h" 
#include "Mobs.h"
#include "Shops.h"
#include "Inventory.h"
#include "Drops.h"
#include "Levels.h"
#include "ChannelServer.h"
#include "SkillsPacket.h"
#include "MapPacket.h"
#include "PacketCreator.h"
#include "BufferUtilities.h"
#include "CharUtilities.h"
#include "WorldServerConnectPlayerPacket.h"

hash_map <int, Player*> Players::players;

void Players::addPlayer(Player* player){
	players[player->getPlayerid()] = player;
}

void Players::deletePlayer(Player* player){
	players.erase(player->getPlayerid());
}

void Players::handleMoving(Player* player, unsigned char* packet, int size){
	Pos cpos;
	int pla = packet[5]*14+1;
	cpos.x = BufferUtilities::getShort(packet+size-4);
	cpos.y = BufferUtilities::getShort(packet+size-2);
	player->setPos(cpos);
	player->setType(packet[5+14*(packet[5]-1)+12]);
	PlayersPacket::showMoving(player, Maps::info[player->getMap()].Players, packet, pla);
}

void Players::faceExperiment(Player* player, unsigned char* packet){
	int face = BufferUtilities::getInt(packet);
	PlayersPacket::faceExperiment(player, Maps::info[player->getMap()].Players, face);
}
void Players::chatHandler(Player* player, unsigned char* packet){
	char chat[91];
	int chatsize = BufferUtilities::getShort(packet);
	BufferUtilities::getString(packet+2, chatsize, chat);
	if(chat[0] == '!'){
		if(!player->isGM()) return;
		char* next_token;
		char command[90] = "";
		if(chatsize>2)
			strcpy_s(command, 90, strtok_s(chat+1, " ", &next_token));
		if (strcmp(command, "map") == 0) {
			if (strlen(next_token) == 0) {
				char msg[60];
				sprintf_s(msg, 60, "Current Map: %i", player->getMap());
				return;
			}
			int mapid = atoi(strtok_s(NULL, " ",&next_token));
			if (Maps::info.find(mapid) != Maps::info.end())
				Maps::changeMap(player ,mapid, 0);
		}
		// Portal recalls
		else if (strcmp(command, "port") == 0) {
			if(strlen(next_token) == 0) return;
			int mapid = -1;
			
			// Towns and etc
			if (strcmp("town", next_token) == 0) mapid = Maps::info[player->getMap()].rm;
			else if (strcmp("gm", next_token) == 0) mapid = 180000000;
			else if (strcmp("fm", next_token) == 0) mapid = 910000000;
			else if (strcmp("4th", next_token) == 0) mapid = 240010501;
			else if (strcmp("showa", next_token) == 0) mapid = 801000000;
			else if (strcmp("armory", next_token) == 0) mapid = 801040004;
			else if (strcmp("shrine", next_token) == 0) mapid = 800000000;
			else if (strcmp("mansion", next_token) == 0) mapid = 682000100;
			else if (strcmp("phantom", next_token) == 0) mapid = 682000000;
			else if (strcmp("henesys", next_token) == 0) mapid = 100000000;
			else if (strcmp("perion", next_token) == 0) mapid = 102000000;
			else if (strcmp("ellinia", next_token) == 0) mapid = 101000000;
			else if (strcmp("sleepywood", next_token) == 0) mapid = 105040300;
			else if (strcmp("lith", next_token) == 0) mapid = 104000000;
			else if (strcmp("moose", next_token) == 0) mapid = 924000001;
			else if (strcmp("kerning", next_token) == 0) mapid = 103000000;
			else if (strcmp("orbis", next_token) == 0) mapid = 200000000;
			else if (strcmp("nath", next_token) == 0) mapid = 211000000;
			else if (strcmp("ludi", next_token) == 0) mapid = 220000000;
			else if (strcmp("kft", next_token) == 0) mapid = 222000000;
			else if (strcmp("aqua", next_token) == 0) mapid = 230000000;
			else if (strcmp("omega", next_token) == 0) mapid = 221000000;
			else if (strcmp("leafre", next_token) == 0) mapid = 240000000;
			else if (strcmp("mulung", next_token) == 0) mapid = 250000000;
			else if (strcmp("herbtown", next_token) == 0) mapid = 251000000;
			else if (strcmp("nlc", next_token) == 0) mapid = 600000000;
			else if (strcmp("amoria", next_token) == 0) mapid = 680000000;
			// Boss maps
			else if (strcmp("ergoth", next_token) == 0) mapid = 990000900;
			else if (strcmp("pap", next_token) == 0) mapid = 220080001;
			else if (strcmp("zakum", next_token) == 0) mapid = 280030000;
			else if (strcmp("horntail", next_token) == 0) mapid = 240060200;
			else if (strcmp("lordpirate", next_token) == 0) mapid = 925100500;
			else if (strcmp("alishar", next_token) == 0) mapid = 922010900;
			else if (strcmp("papapixie", next_token) == 0) mapid = 920010800;
			else if (strcmp("kingslime", next_token) == 0) mapid = 103000804;
			else if (strcmp("pianus", next_token) == 0) mapid = 230040420;
			else if (strcmp("manon", next_token) == 0) mapid = 240020401;
			else if (strcmp("griffey", next_token) == 0) mapid = 240020101;
			else if (strcmp("jrbalrog", next_token) == 0) mapid = 105090900;
			else if (strcmp("grandpa", next_token) == 0) mapid = 801040100;
			else if (strcmp("anego", next_token) == 0) mapid = 801040003;
			else if (strcmp("tengu", next_token) == 0) mapid = 800020130;
			if(mapid != -1)
				Maps::changeMap(player, mapid, 0);
		}
		else if(strcmp(command, "addsp") == 0){
			if(strlen(next_token) > 0){
				int skillid = atoi(strtok_s(NULL, " ",&next_token));
				int count = 1;
				if(strlen(next_token) > 0)
				count = atoi(next_token);
				player->skills->addSkillLevel(skillid, count);
			}
		}
		else if(strcmp(command, "summon") == 0 || strcmp(command, "spawn") == 0){
			if(strlen(next_token) == 0) return;
			int mobid = atoi(strtok_s(NULL, " ",&next_token));
			if(Mobs::mobinfo.find(mobid) == Mobs::mobinfo.end())
				return;
			int count = 1;
			if(strlen(next_token) > 0)
				count = atoi(next_token);
			for(int i=0; i<count && i<100; i++){
				Mobs::spawnMob(player, mobid);
			}
		}
		else if(strcmp(command, "notice") == 0){
			if(strlen(next_token) == 0) return;
			PlayersPacket::showMessage(next_token, 0);
		}
		else if(strcmp(command, "shop") == 0){
			Shops::showShop(player, 9999999);
		}
		else if(strcmp(command, "shop2") == 0){
			Shops::showShop(player, 9999996);
		}
		else if(strcmp(command, "shop3") == 0){
			Shops::showShop(player, 9999995);
		}
		else if(strcmp(command, "shop4") == 0){
			Shops::showShop(player, 9999998);
		}
		else if(strcmp(command, "shop5") == 0){
			Shops::showShop(player, 9999997);
		}
		else if(strcmp(command, "pos") == 0){
			char text[50];
			sprintf_s(text, 50, "X: %d Y: %d", player->getPos().x, player->getPos().y);
			PlayersPacket::showMessage(text, 0);
		}
		else if(strcmp(command, "item") == 0){
			if(strlen(next_token) == 0) return;
			int itemid = atoi(strtok_s(NULL, " ",&next_token));
			if(Drops::items.find(itemid) == Drops::items.end() && Drops::equips.find(itemid) == Drops::equips.end())
				return;
			int count = 1;
			if(strlen(next_token) > 0)
				count = atoi(next_token);
			Inventory::addNewItem(player, itemid, count);
		}
		else if(strcmp(command, "level") == 0){
			if(strlen(next_token) == 0) return;
			Levels::setLevel(player, atoi(strtok_s(NULL, " ",&next_token)));
		}
		// Jobs
		else if (strcmp(command, "job") == 0) {
			if (strlen(next_token) == 0) {
				char msg[60];
				sprintf_s(msg, 60, "Current Job: %i", player->getJob());
				PlayerPacket::showMessage(player, msg, 6);
				return;
			}

			int job = -1;
			if (strcmp(next_token, "beginner") == 0) job = 0;
			else if (strcmp(next_token, "warrior") == 0) job = 100;
			else if (strcmp(next_token, "fighter") == 0) job = 110;
			else if (strcmp(next_token, "sader") == 0) job = 111;
			else if (strcmp(next_token, "hero") == 0) job = 112;
			else if (strcmp(next_token, "page") == 0) job = 120;
			else if (strcmp(next_token, "wk") == 0) job = 121;
			else if (strcmp(next_token, "paladin") == 0) job = 122;
			else if (strcmp(next_token, "spearman") == 0) job = 130;
			else if (strcmp(next_token, "dk") == 0) job  = 131;
			else if (strcmp(next_token, "drk") == 0) job = 132;
			else if (strcmp(next_token, "mage") == 0) job = 200;
			else if (strcmp(next_token, "fpwiz") == 0) job = 210;
			else if (strcmp(next_token, "fpmage") == 0) job = 211;
			else if (strcmp(next_token, "fparch") == 0) job = 212;
			else if (strcmp(next_token, "ilwiz") == 0) job = 220;
			else if (strcmp(next_token, "ilmage") == 0) job = 221;
			else if (strcmp(next_token, "ilarch") == 0) job = 222;
			else if (strcmp(next_token, "cleric") == 0) job = 230;
			else if (strcmp(next_token, "priest") == 0) job = 231;
			else if (strcmp(next_token, "bishop") == 0) job = 232;
			else if (strcmp(next_token, "bowman") == 0) job = 300;
			else if (strcmp(next_token, "hunter") == 0) job = 310;
			else if (strcmp(next_token, "ranger") == 0) job = 311;
			else if (strcmp(next_token, "bm") == 0) job = 312;
			else if (strcmp(next_token, "xbowman") == 0) job = 320;
			else if (strcmp(next_token, "sniper") == 0) job = 321;
			else if (strcmp(next_token, "marksman") == 0) job = 322;
			else if (strcmp(next_token, "thief") == 0) job = 400;
			else if (strcmp(next_token, "sin") == 0) job = 410;
			else if (strcmp(next_token, "hermit") == 0) job = 411;
			else if (strcmp(next_token, "nl") == 0) job = 412;
			else if (strcmp(next_token, "dit") == 0) job = 420;
			else if (strcmp(next_token, "cb") == 0) job = 412;
			else if (strcmp(next_token, "shadower") == 0) job = 422;
			else if (strcmp(next_token, "gm") == 0) job = 500;
			else if (strcmp(next_token, "sgm") == 0) job = 510;
			else job = atoi(strtok_s(NULL, " ", &next_token));

			if (job >= 0)
				Levels::setJob(player, job);
		}
		else if(strcmp(command, "ap") == 0){
			if(strlen(next_token) == 0) return;
			player->setAp(player->getAp()+atoi(strtok_s(NULL, " ",&next_token)));
		}
		else if(strcmp(command, "sp") == 0){
			if(strlen(next_token) == 0) return;
			player->setSp(player->getSp()+atoi(strtok_s(NULL, " ",&next_token)));
		}
		else if(strcmp(command, "killnpc") == 0){
			player->setNPC(NULL);
		}
		else if(strcmp(command, "killall") == 0){
			int size=Mobs::mobs[player->getMap()].size();
			for (int j=0; j<size; j++){
				Mobs::dieMob(player, Mobs::mobs[player->getMap()][0]);
			}
		}
		else if(strcmp(command, "horntail") == 0){
			Mobs::spawnMob(player, 8810002);
			Mobs::spawnMob(player, 8810003);
			Mobs::spawnMob(player, 8810004);
			Mobs::spawnMob(player, 8810005);
			Mobs::spawnMob(player, 8810006);
			Mobs::spawnMob(player, 8810007);
            Mobs::spawnMob(player, 8810008);
            Mobs::spawnMob(player, 8810009);
        }
		else if(strcmp(command, "heal") == 0){
            player->setHP(player->getMHP());
            player->setMP(player->getMMP());
        }
		else if(strcmp(command, "mesos") == 0){
			if (strlen(next_token) == 0) return;
			long mesos = atoi(strtok_s(NULL, " ",&next_token));
			player->inv->setMesos(mesos);
		}
		else if (strcmp(command, "cleardrops") == 0) {
			while (Drops::drops[player->getMap()].size() != 0) {
				Drops::drops[player->getMap()][0]->removeDrop();
			}
		}
		else if (strcmp(command, "save") == 0) {
			player->save();
			PlayerPacket::showMessage(player, "Your progress has been saved.", 5);
		}
		else if (strcmp(command, "warp") == 0) {
			char *name = strtok_s(NULL, " ",&next_token);
			if (strlen(next_token) == 0) return;
			if (strlen(name) > 0)
				for (hash_map <int, Player*>::iterator iter = Players::players.begin(); iter != Players::players.end(); iter++)
					if (strcmp(iter->second->getName(), name) == 0)
						if (strlen(next_token) > 0) {
							int mapid = atoi(strtok_s(NULL, " ", &next_token));
							if(Maps::info.find(mapid) != Maps::info.end()){
								Maps::changeMap(iter->second ,mapid, 0);
								break;
							}
						}
		}
		else if (strcmp(command, "warpto") == 0) {
			char *name = strtok_s(NULL, " ",&next_token);
			if (strlen(name) > 0)
				for (hash_map <int, Player*>::iterator iter = Players::players.begin(); iter != Players::players.end(); iter++)
					if (strcmp(iter->second->getName(), name) == 0)
						Maps::changeMap(player , iter->second->getMap(), iter->second->getMappos());
		}
		else if (strcmp(command, "mwarpto") == 0) {
			char *name = strtok_s(NULL, " ",&next_token);
			if (strlen(name) > 0)
				for (hash_map <int, Player*>::iterator iter = Players::players.begin(); iter != Players::players.end(); iter++)
					if (strcmp(iter->second->getName(), name) == 0) {
						Maps::changeMap(iter->second, player->getMap(), player->getMappos());
						break;
					}
		}
		else if (strcmp(command, "warpall") == 0) { // Warp everyone to MapID or your current map
			int mapid = 0;
			if (strlen(next_token) == 0)
				mapid = player->getMap();
			else
				mapid = atoi(strtok_s(NULL, "", &next_token));

			for (hash_map <int, Player*>::iterator iter = Players::players.begin(); iter != Players::players.end(); iter++){
				if(Maps::info.find(mapid) != Maps::info.end()){
					if(mapid == player->getMap()){
						if(strcmp(player->getName(), iter->second->getName())!=0)
							Maps::changeMap(iter->second, mapid, 0);
					}
					else
						Maps::changeMap(iter->second, mapid, 0);
				}
			}
		}
		else if(strcmp(command, "kill") == 0){
			if(strcmp(next_token, "all") == 0){
				for (unsigned int x=0; x<Maps::info[player->getMap()].Players.size(); x++){
					Player* killpsa;
					killpsa = Maps::info[player->getMap()].Players[x];
					if (killpsa != player) {
						killpsa->setHP(0);
					}
				}
			}
			else if(strcmp(next_token, "gm") == 0){
				for (unsigned int x=0; x<Maps::info[player->getMap()].Players.size(); x++){	
					Player* killpsa;
					killpsa = Maps::info[player->getMap()].Players[x];
					if(killpsa != player){
						if(killpsa->isGM()){	
							killpsa->setHP(0);
						}
					}
				}
			}
			else if(strcmp(next_token, "players") == 0){
				for (unsigned int x=0; x<Maps::info[player->getMap()].Players.size(); x++){	
					Player* killpsa;
					killpsa = Maps::info[player->getMap()].Players[x];
					if(killpsa != player){
						if(!killpsa->isGM()){
							killpsa->setHP(0);
						}
					}
				}
			}
			else if(strcmp(next_token, "me") == 0){
				player->setHP(0);
			}
			else {
				for (unsigned int x=0; x<Maps::info[player->getMap()].Players.size(); x++){
					Player* killpsa;
					killpsa = Maps::info[player->getMap()].Players[x];
					if(killpsa != player){
						killpsa->setHP(0);
					}
				}
			}
		}
		else if(strcmp(command, "zakum") == 0){
			Mobs::spawnMob(player, 8800000);
			Mobs::spawnMob(player, 8800003);
			Mobs::spawnMob(player, 8800004);
			Mobs::spawnMob(player, 8800005);
			Mobs::spawnMob(player, 8800006);
			Mobs::spawnMob(player, 8800007);
			Mobs::spawnMob(player, 8800008);
			Mobs::spawnMob(player, 8800009);
			Mobs::spawnMob(player, 8800010);
		}
		else if(strcmp(command, "header") == 0){
			WorldServerConnectPlayerPacket::scrollingHeader(ChannelServer::Instance()->getWorldPlayer(), next_token);
		}
		else if	(strcmp(command, "dc") == 0)	{
			player->disconnect();
			return;
		}
		else if (strcmp(command, "shutdown") == 0) {
			ChannelServer::Instance()->shutdown();
		}
		else if (strcmp(command, "packet") == 0) {
			Packet packet;
			packet.addBytes(next_token);
			packet.send(player);
		}
		else if (strcmp(command, "timer") == 0) {
			MapPacket::showTimer(player, atoi(next_token));
		}
		return;
	}
	PlayersPacket::showChat(player, Maps::info[player->getMap()].Players, chat);
}

void Players::damagePlayer(Player* player, unsigned char* packet){
	int damage = BufferUtilities::getInt(packet+5);
	int mobid = BufferUtilities::getInt(packet+13);
	Mob* mob = NULL;
	for(unsigned int i=0; i<Mobs::mobs[player->getMap()].size(); i++)
		if(Mobs::mobs[player->getMap()][i]->getID() == mobid){
			mob = Mobs::mobs[player->getMap()][i];
			break;
		}

	// Magic Guard
	if(player->skills->getActiveSkillLevel(2001002) > 0){
		unsigned short mp = player->getMP();
        unsigned short hp = player->getHP();
        unsigned short reduc = Skills::skills[2001002][player->skills->getActiveSkillLevel(2001002)].x;
        int mpdamage = ((damage * reduc) / 100);
        int hpdamage = damage - mpdamage;
        if (mpdamage >= mp) {
            player->setMP(0);
            player->setHP(hp - (hpdamage + (mpdamage - mp)));
        }
        if (mpdamage < mp) {
            player->setMP(mp - mpdamage);
            player->setHP(hp - hpdamage);
        }
	}
	// Power Guard
	else if(player->skills->getActiveSkillLevel(1101007)>0 || player->skills->getActiveSkillLevel(1201007)>0){
		float reduc;
		if(player->skills->getActiveSkillLevel(1101007)>0)
			reduc = Skills::skills[1101007][player->skills->getActiveSkillLevel(1101007)].x;
		else
			reduc = Skills::skills[1201007][player->skills->getActiveSkillLevel(1201007)].x;
		if(damage>0){
			damage = (int) (damage-(damage*(reduc/100)));
			player->setHP(player->getHP()-damage);
		}
	}
	// Meso Guard
	else if(player->skills->getActiveSkillLevel(4211005) > 0 && player->inv->getMesos() > 0){
		unsigned short hp = player->getHP();
		// Get the rate of meso loss in %
		float mesorate = Skills::skills[4211005][player->skills->getActiveSkillLevel(4211005)].x;
		mesorate = mesorate/100;
		int mesoloss = (int) (mesorate)*(damage/2);
		// Only block damage if user has mesos
		if(player->inv->getMesos() > 0)
			player->setHP(hp-(damage/2));
		else
			player->setHP(hp-damage);
		// Do not let mesos go negative
		if(player->inv->getMesos()-mesoloss < 0){
			player->inv->setMesos(0);
		}
		else{
			player->inv->setMesos(player->inv->getMesos()-mesoloss);
		}
	}
	else{
		player->setHP(player->getHP()-damage);
	}
	if(mob != NULL)
		PlayersPacket::damagePlayer(player, Maps::info[player->getMap()].Players, damage, mob->getMobID());
}

void Players::healPlayer(Player* player, unsigned char* packet){
	player->setHP(player->getHP()+BufferUtilities::getShort(packet+4));
	player->setMP(player->getMP()+BufferUtilities::getShort(packet+6));
}

void Players::getPlayerInfo(Player* player, unsigned char* packet){
	PlayersPacket::showInfo(player, players[BufferUtilities::getInt(packet+4)]);
}

void Players::commandHandler(Player* player, unsigned char* packet){
	unsigned char type = packet[0];
	char name[20];

	int namesize = BufferUtilities::getShort(packet+1);
	BufferUtilities::getString(packet+3, namesize, name);
	
	char chat[91];
	if (type == 0x06) {
		int chatsize = BufferUtilities::getShort(packet+3+namesize);
		BufferUtilities::getString(packet+5+namesize, chatsize, chat);
	}

	hash_map <int, Player*>::iterator iter = Players::players.begin();
	for ( iter = Players::players.begin(); iter != Players::players.end(); iter++){
		if (_stricmp(iter->second->getName(), name) == 0){	
			if(type == 0x06){
				PlayersPacket::whisperPlayer(iter->second, player->getName(), player->getChannel(), chat);
				PlayersPacket::findPlayer(player,iter->second->getName(),-1,1);
			}
			else if(type == 0x05){
				PlayersPacket::findPlayer(player, iter->second->getName(), iter->second->getMap());
			}
			break;
		}
	}	
	if(iter == Players::players.end()){
		if (type == 0x05)
			WorldServerConnectPlayerPacket::findPlayer(ChannelServer::Instance()->getWorldPlayer(), player->getPlayerid(), name); // Let's connect to the world server to see if the player is on any other channel
		else
			WorldServerConnectPlayerPacket::whisperPlayer(ChannelServer::Instance()->getWorldPlayer(), player->getPlayerid(), name, chat);
	}
}
