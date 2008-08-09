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

#define SEND_PING 0x11

// Login
#define SEND_LOGIN_INFO_REPLY 0x00
#define SEND_SHOW_CHANNEL 0x03
#define SEND_GENDER_DONE 0x04
#define SEND_LOGIN_PROCESS 0x06
#define SEND_PIN_ASSIGNED 0x07
#define SEND_SHOW_WORLD 0x0a
#define SEND_SHOW_CHARACTERS 0x0b
#define SEND_CHANNEL_SERVER_INFO 0x0c
#define SEND_CHECK_NAME 0x0d
#define SEND_SHOW_CHARACTER 0x0e
#define SEND_DELETE_CHAR 0x0f
#define SEND_CHANNEL_SELECT 0x14
#define SEND_RELOG_RESPONSE 0x16

// Channel
#define SEND_CHANGE_CHANNEL 0x10
#define SEND_MOVE_ITEM 0x1a
#define SEND_UPDATE_INVENTORY_SLOTS 0x1b
#define SEND_UPDATE_STAT 0x1c
#define SEND_USE_SKILL 0x1d
#define SEND_CANCEL_SKILL 0x1e
#define SEND_ADD_SKILL 0x21
#define SEND_FAME 0x23
#define SEND_NOTE 0x24 // Bottom-right corner notice
#define SEND_FINISH_QUEST 0x2e
#define SEND_USE_SKILLBOOK 0x30 // Skillbooks
#define SEND_SHOW_ITEM_EFFECT 0x31
#define SEND_PLAYER_INFO 0x3a
#define SEND_PARTY_ACTION 0x3b
#define SEND_NOTICE 0x41
#define SEND_SHOW_MESSENGER 0x54 // Also known as avatar megaphone
#define SEND_SKILL_MACRO 0x5a
#define SEND_CHANGE_MAP 0x5b
#define SEND_MAKE_APPLE 0x62 // Wear apple suit
#define SEND_GROUP_CHAT 0x63 // Groups like party, buddies and guild
#define SEND_COMMAND_RESPOND 0x64 // Respond to / commands i.e. "/find"
#define SEND_MAP_EFFECT 0x67 // Change Music/Boss HP Bars
#define SEND_USE_WEATHER 0x68 // Weather effects
#define SEND_TIME 0x6d // For places with clock i.e. ship station
#define SEND_SHOW_PLAYER 0x77
#define SEND_REMOVE_PLAYER 0x78
#define SEND_CHAT 0x79
#define SEND_USE_SCROLL 0x7d
#define SEND_PET_SUMMONED 0x7e
#define SEND_PET_SHOW_MOVING 0x80
#define SEND_PET_SHOW_CHAT 0x81
#define SEND_PET_NAME_CHANGE 0x82
#define SEND_PET_SHOW 0x83
#define SEND_PET_ANIMATION 0x84
#define SEND_MOVE_PLAYER 0x8c
#define SEND_DAMAGE_MOB 0x8d
#define SEND_DAMAGE_MOB_RANGED 0x8e
#define SEND_DAMAGE_MOB_SPELL 0x8f
#define SEND_SPECIAL_SKILL 0x91
#define SEND_SPECIAL_SKILL_END 0x92
#define SEND_DAMAGE_PLAYER 0x93
#define SEND_FACE_EXPERIMENT 0x94
#define SEND_SIT_CHAIR 0x96
#define SEND_UPDATE_CHAR_LOOK 0x97
#define SEND_SHOW_SKILL 0x98
#define SEND_SHOW_OTHERS_SKILL 0x99 // From other player
#define SEND_CANCEL_OTHERS_BUFF 0x9a // From other player
#define SEND_STOP_CHAIR 0x9f
#define SEND_GAIN_ITEM 0xa0 // Gain item notice in chat
#define SEND_UPDATE_QUEST 0xa5
#define SEND_INSTRUCTION_BUBBLE 0xa8
#define SEND_COOLDOWN 0xab // Skill cooldown
#define SEND_SHOW_MOB 0xad
#define SEND_KILL_MOB 0xae
#define SEND_CONTROL_MOB 0xaf
#define SEND_MOVE_MOB 0xb0
#define SEND_MOVE_MOB_RESPONSE 0xb1
#define SEND_SHOW_MOB_HP 0xbb
#define SEND_SHOW_DRAGGED 0xbc // New in .56, Monster Magnet'd mobs get their own packet
#define SEND_SHOW_NPC 0xc0
#define SEND_SHOW_NPC2 0xc2
#define SEND_DROP_ITEM 0xcb
#define SEND_TAKE_DROP 0xcc
#define SEND_TRIGGER_REACTOR 0xd4
#define SEND_SHOW_REACTOR 0xd6
#define SEND_DESTROY_REACTOR 0xd7
#define SEND_NPC_TALK 0xea
#define SEND_SHOP_OPEN 0xeb
#define SEND_SHOP_BOUGHT 0xec
#define SEND_STORAGE_ACTION 0xed
#define SEND_SHOP_ACTION 0xf2 // Trades, player shops
#define SEND_KEYMAP 0x104
