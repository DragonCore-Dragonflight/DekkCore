/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Chat.h"
#include "AccountMgr.h"
#include "CellImpl.h"
#include "CharacterCache.h"
#include "ChatCommand.h"
#include "ChatPackets.h"
#include "Common.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Language.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Optional.h"
#include "Player.h"
#include "Realm.h"
#include "StringConvert.h"
#include "World.h"
#include "WorldSession.h"
#ifdef ELUNA
#include "LuaEngine.h"
#endif
#include <boost/algorithm/string/replace.hpp>
#include <sstream>

Player* ChatHandler::GetPlayer() const { return m_session ? m_session->GetPlayer() : nullptr; }

char* ChatHandler::LineFromMessage(char*& pos)
{
    char* start = strtok(pos, "\n");
    pos = nullptr;
    return start;
}

char const* ChatHandler::GetTrinityString(uint32 entry) const
{
    return m_session->GetTrinityString(entry);
}

bool ChatHandler::HasPermission(uint32 permission) const
{
    return m_session->HasPermission(permission);
}

std::string ChatHandler::GetNameLink() const
{
    return GetNameLink(m_session->GetPlayer());
}

bool ChatHandler::HasLowerSecurity(Player* target, ObjectGuid guid, bool strong)
{
    WorldSession* target_session = nullptr;
    uint32 target_account = 0;

    if (target)
        target_session = target->GetSession();
    else if (!guid.IsEmpty())
        target_account = sCharacterCache->GetCharacterAccountIdByGuid(guid);

    if (!target_session && !target_account)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return true;
    }

    return HasLowerSecurityAccount(target_session, target_account, strong);
}

bool ChatHandler::HasLowerSecurityAccount(WorldSession* target, uint32 target_account, bool strong)
{
    uint32 target_sec;

    // allow everything from console and RA console
    if (!m_session)
        return false;

    // ignore only for non-players for non strong checks (when allow apply command at least to same sec level)
    if (m_session->HasPermission(rbac::RBAC_PERM_CHECK_FOR_LOWER_SECURITY) && !strong && !sWorld->getBoolConfig(CONFIG_GM_LOWER_SECURITY))
        return false;

    if (target)
        target_sec = target->GetSecurity();
    else if (target_account)
        target_sec = AccountMgr::GetSecurity(target_account, realm.Id.Realm);
    else
        return true;                                        // caller must report error for (target == nullptr && target_account == 0)

    AccountTypes target_ac_sec = AccountTypes(target_sec);
    if (m_session->GetSecurity() < target_ac_sec || (strong && m_session->GetSecurity() <= target_ac_sec))
    {
        SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
        SetSentErrorMessage(true);
        return true;
    }

    return false;
}

void ChatHandler::SendSysMessage(std::string_view str, bool escapeCharacters)
{
    std::string msg{ str };

    // Replace every "|" with "||" in msg
    if (escapeCharacters && msg.find('|') != std::string::npos)
    {
        std::vector<std::string_view> tokens = Trinity::Tokenize(msg, '|', true);
        std::ostringstream stream;
        for (size_t i = 0; i < tokens.size() - 1; ++i)
            stream << tokens[i] << "||";
        stream << tokens[tokens.size() - 1];

        msg = stream.str();
    }

    WorldPackets::Chat::Chat packet;
    for (std::string_view line : Trinity::Tokenize(str, '\n', true))
    {
        packet.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        m_session->SendPacket(packet.Write());
    }
}

void ChatHandler::SendGlobalSysMessage(const char* str)
{
    WorldPackets::Chat::Chat packet;
    for (std::string_view line : Trinity::Tokenize(str, '\n', true))
    {
        packet.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        sWorld->SendGlobalMessage(packet.Write());
    }
}

void ChatHandler::SendGlobalGMSysMessage(const char* str)
{
    WorldPackets::Chat::Chat packet;
    for (std::string_view line : Trinity::Tokenize(str, '\n', true))
    {
        packet.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        sWorld->SendGlobalGMMessage(packet.Write());
    }
}

void ChatHandler::SendSysMessage(uint32 entry)
{
    SendSysMessage(GetTrinityString(entry));
}

bool ChatHandler::_ParseCommands(std::string_view text)
{
    if (Trinity::ChatCommands::TryExecuteCommand(*this, text))
        return true;

    // Pretend commands don't exist for regular players
    if (m_session && !m_session->HasPermission(rbac::RBAC_PERM_COMMANDS_NOTIFY_COMMAND_NOT_FOUND_ERROR))
        return false;

    // Send error message for GMs
    PSendSysMessage(LANG_CMD_INVALID, STRING_VIEW_FMT_ARG(text));
    SetSentErrorMessage(true);
    return true;
}

bool ChatHandler::ParseCommands(std::string_view text)
{
    ASSERT(!text.empty());

    // chat case (.command or !command format)
    if ((text[0] != '!') && (text[0] != '.'))
        return false;

    // ignore single . and ! in line
    if (text.length() < 2)
        return false;

    // ignore messages staring from many dots.
    if (text[1] == text[0])
        return false;

    // ignore messages with separator after .
    if (text[1] == Trinity::Impl::ChatCommands::COMMAND_DELIMITER)
        return false;

    return _ParseCommands(text.substr(1));
}

Player* ChatHandler::getSelectedPlayer()
{
    if (!m_session)
        return nullptr;

    ObjectGuid selected = m_session->GetPlayer()->GetTarget();
    if (!selected)
        return m_session->GetPlayer();

    return ObjectAccessor::FindConnectedPlayer(selected);
}

Unit* ChatHandler::getSelectedUnit()
{
    if (!m_session)
        return nullptr;

    if (Unit* selected = m_session->GetPlayer()->GetSelectedUnit())
        return selected;

    return m_session->GetPlayer();
}

WorldObject* ChatHandler::getSelectedObject()
{
    if (!m_session)
        return nullptr;

    ObjectGuid guid = m_session->GetPlayer()->GetTarget();

    if (!guid)
        return GetNearbyGameObject();

    return ObjectAccessor::GetUnit(*m_session->GetPlayer(), guid);
}

Creature* ChatHandler::getSelectedCreature()
{
    if (!m_session)
        return nullptr;

    return ObjectAccessor::GetCreatureOrPetOrVehicle(*m_session->GetPlayer(), m_session->GetPlayer()->GetTarget());
}

Player* ChatHandler::getSelectedPlayerOrSelf()
{
    if (!m_session)
        return nullptr;

    ObjectGuid selected = m_session->GetPlayer()->GetTarget();
    if (!selected)
        return m_session->GetPlayer();

    // first try with selected target
    Player* targetPlayer = ObjectAccessor::FindConnectedPlayer(selected);
    // if the target is not a player, then return self
    if (!targetPlayer)
        targetPlayer = m_session->GetPlayer();

    return targetPlayer;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* linkType, char** something1)
{
    // skip empty
    if (!text)
        return nullptr;

    // skip spaces
    while (*text == ' ' || *text == '\t' || *text == '\b')
        ++text;

    if (!*text)
        return nullptr;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r

    char* check = strtok(text, "|");                        // skip color
    if (!check)
        return nullptr;                                     // end of data

    char* cLinkType = strtok(nullptr, ":");                 // linktype
    if (!cLinkType)
        return nullptr;                                     // end of data

    if (strcmp(cLinkType, linkType) != 0)
    {
        strtok(nullptr, " ");                               // skip link tail (to allow continue strtok(nullptr, s) use after retturn from function
        SendSysMessage(LANG_WRONG_LINK_TYPE);
        return nullptr;
    }

    char* cKeys = strtok(nullptr, "|");                     // extract keys and values
    char* cKeysTail = strtok(nullptr, "");

    char* cKey = strtok(cKeys, ":|");                       // extract key
    if (something1)
        *something1 = strtok(nullptr, ":|");                // extract something

    strtok(cKeysTail, "]");                                 // restart scan tail and skip name with possible spaces
    strtok(nullptr, " ");                                   // skip link tail (to allow continue strtok(nullptr, s) use after return from function
    return cKey;
}

char* ChatHandler::extractKeyFromLink(char* text, char const* const* linkTypes, int* found_idx, char** something1)
{
    // skip empty
    if (!text)
        return nullptr;

    // skip spaces
    while (*text == ' ' || *text == '\t' || *text == '\b')
        ++text;

    if (!*text)
        return nullptr;

    // return non link case
    if (text[0] != '|')
        return strtok(text, " ");

    // [name] Shift-click form |color|linkType:key|h[name]|h|r
    // or
    // [name] Shift-click form |color|linkType:key:something1:...:somethingN|h[name]|h|r
    // or
    // [name] Shift-click form |linkType:key|h[name]|h|r

    char* tail;

    if (text[1] == 'c')
    {
        char* check = strtok(text, "|");                    // skip color
        if (!check)
            return nullptr;                                 // end of data

        tail = strtok(nullptr, "");                         // tail
    }
    else
        tail = text + 1;                                      // skip first |

    char* cLinkType = strtok(tail, ":");                    // linktype
    if (!cLinkType)
        return nullptr;                                     // end of data

    for (int i = 0; linkTypes[i]; ++i)
    {
        if (strcmp(cLinkType, linkTypes[i]) == 0)
        {
            char* cKeys = strtok(nullptr, "|");             // extract keys and values
            char* cKeysTail = strtok(nullptr, "");

            char* cKey = strtok(cKeys, ":|");               // extract key
            if (something1)
                *something1 = strtok(nullptr, ":|");        // extract something

            strtok(cKeysTail, "]");                         // restart scan tail and skip name with possible spaces
            strtok(nullptr, " ");                           // skip link tail (to allow continue strtok(nullptr, s) use after return from function
            if (found_idx)
                *found_idx = i;
            return cKey;
        }
    }

    strtok(nullptr, " ");                                   // skip link tail (to allow continue strtok(nullptr, s) use after return from function
    SendSysMessage(LANG_WRONG_LINK_TYPE);
    return nullptr;
}

GameObject* ChatHandler::GetNearbyGameObject()
{
    if (!m_session)
        return nullptr;

    Player* pl = m_session->GetPlayer();
    GameObject* obj = nullptr;
    Trinity::NearestGameObjectCheck check(*pl);
    Trinity::GameObjectLastSearcher<Trinity::NearestGameObjectCheck> searcher(pl, obj, check);
    Cell::VisitGridObjects(pl, searcher, SIZE_OF_GRIDS);
    return obj;
}

GameObject* ChatHandler::GetObjectFromPlayerMapByDbGuid(ObjectGuid::LowType lowguid)
{
    if (!m_session)
        return nullptr;
    auto bounds = m_session->GetPlayer()->GetMap()->GetGameObjectBySpawnIdStore().equal_range(lowguid);
    if (bounds.first != bounds.second)
        return bounds.first->second;
    return nullptr;
}

Creature* ChatHandler::GetCreatureFromPlayerMapByDbGuid(ObjectGuid::LowType lowguid)
{
    if (!m_session)
        return nullptr;
    // Select the first alive creature or a dead one if not found
    Creature* creature = nullptr;
    auto bounds = m_session->GetPlayer()->GetMap()->GetCreatureBySpawnIdStore().equal_range(lowguid);
    for (auto it = bounds.first; it != bounds.second; ++it)
    {
        creature = it->second;
        if (it->second->IsAlive())
            break;
    }
    return creature;
}

enum GuidLinkType
{
    GUID_LINK_PLAYER = 0,                              // must be first for selection in not link case
    GUID_LINK_CREATURE = 1,
    GUID_LINK_GAMEOBJECT = 2
};

static char const* const guidKeys[] =
{
    "Hplayer",
    "Hcreature",
    "Hgameobject",
    nullptr
};

ObjectGuid::LowType ChatHandler::extractLowGuidFromLink(char* text, HighGuid& guidHigh)
{
    int type = 0;

    // |color|Hcreature:creature_guid|h[name]|h|r
    // |color|Hgameobject:go_guid|h[name]|h|r
    // |color|Hplayer:name|h[name]|h|r
    char* idS = extractKeyFromLink(text, guidKeys, &type);
    if (!idS)
        return 0;

    switch (type)
    {
    case GUID_LINK_PLAYER:
    {
        guidHigh = HighGuid::Player;
        std::string name = idS;
        if (!normalizePlayerName(name))
            return 0;

        if (Player* player = ObjectAccessor::FindPlayerByName(name))
            return player->GetGUID().GetCounter();

        ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(name);
        if (guid.IsEmpty())
            return 0;

        return guid.GetCounter();
    }
    case GUID_LINK_CREATURE:
    {
        guidHigh = HighGuid::Creature;
        ObjectGuid::LowType lowguid = Trinity::StringTo<ObjectGuid::LowType>(idS).value_or(UI64LIT(0));
        return lowguid;
    }
    case GUID_LINK_GAMEOBJECT:
    {
        guidHigh = HighGuid::GameObject;
        ObjectGuid::LowType lowguid = Trinity::StringTo<ObjectGuid::LowType>(idS).value_or(UI64LIT(0));
        return lowguid;
    }
    }

    // unknown type?
    return 0;
}

std::string ChatHandler::extractPlayerNameFromLink(char* text)
{
    // |color|Hplayer:name|h[name]|h|r
    char* name_str = extractKeyFromLink(text, "Hplayer");
    if (!name_str)
        return "";

    std::string name = name_str;
    if (!normalizePlayerName(name))
        return "";

    return name;
}

bool ChatHandler::extractPlayerTarget(char* args, Player** player, ObjectGuid* player_guid /*= nullptr*/, std::string* player_name /*= nullptr*/)
{
    if (args && *args)
    {
        std::string name = extractPlayerNameFromLink(args);
        if (name.empty())
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        Player* pl = ObjectAccessor::FindPlayerByName(name);

        // if allowed player pointer
        if (player)
            *player = pl;

        // if need guid value from DB (in name case for check player existence)
        ObjectGuid guid = !pl && (player_guid || player_name) ? sCharacterCache->GetCharacterGuidByName(name) : ObjectGuid::Empty;

        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetGUID() : guid;

        if (player_name)
            *player_name = pl || !guid.IsEmpty() ? name : "";
    }
    else
    {
        // populate strtok buffer to prevent crashes
        static char dummy[1] = "";
        strtok(dummy, "");

        Player* pl = getSelectedPlayerOrSelf();
        // if allowed player pointer
        if (player)
            *player = pl;
        // if allowed player guid (if no then only online players allowed)
        if (player_guid)
            *player_guid = pl ? pl->GetGUID() : ObjectGuid::Empty;

        if (player_name)
            *player_name = pl ? pl->GetName() : "";
    }

    // some from req. data must be provided (note: name is empty if player does not exist)
    if ((!player || !*player) && (!player_guid || !*player_guid) && (!player_name || player_name->empty()))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}

char* ChatHandler::extractQuotedArg(char* args)
{
    if (!args || !*args)
        return nullptr;

    if (*args == '"')
        return strtok(args + 1, "\"");
    else
    {
        // skip spaces
        while (*args == ' ')
        {
            args += 1;
            continue;
        }

        // return nullptr if we reached the end of the string
        if (!*args)
            return nullptr;

        // since we skipped all spaces, we expect another token now
        if (*args == '"')
        {
            // return an empty string if there are 2 "" in a row.
            // strtok doesn't handle this case
            if (*(args + 1) == '"')
            {
                strtok(args, " ");
                static char arg[1];
                arg[0] = '\0';
                return arg;
            }
            else
                return strtok(args + 1, "\"");
        }
        else
            return nullptr;
    }
}

bool ChatHandler::needReportToTarget(Player* chr) const
{
    Player* pl = m_session->GetPlayer();
    return pl != chr && pl->IsVisibleGloballyFor(chr);
}

LocaleConstant ChatHandler::GetSessionDbcLocale() const
{
    return m_session->GetSessionDbcLocale();
}

LocaleConstant ChatHandler::GetSessionDbLocaleIndex() const
{
    return m_session->GetSessionDbLocaleIndex();
}

std::string ChatHandler::playerLink(std::string const& name) const
{
    return m_session ? "|cffffffff|Hplayer:" + name + "|h[" + name + "]|h|r" : name;
}

std::string ChatHandler::GetNameLink(Player* chr) const
{
    return playerLink(chr->GetName());
}

char const* CliHandler::GetTrinityString(uint32 entry) const
{
    return sObjectMgr->GetTrinityStringForDBCLocale(entry);
}

void CliHandler::SendSysMessage(std::string_view str, bool /*escapeCharacters*/)
{
    m_print(m_callbackArg, str);
    m_print(m_callbackArg, "\r\n");
}

bool CliHandler::ParseCommands(std::string_view str)
{
    if (str.empty())
        return false;
    // Console allows using commands both with and without leading indicator
    if (str[0] == '.' || str[0] == '!')
        str = str.substr(1);
    return _ParseCommands(str);
}

std::string CliHandler::GetNameLink() const
{
    return GetTrinityString(LANG_CONSOLE_COMMAND);
}

bool CliHandler::needReportToTarget(Player* /*chr*/) const
{
    return true;
}

bool ChatHandler::GetPlayerGroupAndGUIDByName(const char* cname, Player*& player, Group*& group, ObjectGuid& guid, bool offline)
{
    player = nullptr;
    guid.Clear();

    if (cname)
    {
        std::string name = cname;
        if (!name.empty())
        {
            if (!normalizePlayerName(name))
            {
                SendSysMessage(LANG_PLAYER_NOT_FOUND);
                SetSentErrorMessage(true);
                return false;
            }

            player = ObjectAccessor::FindPlayerByName(name);
            if (offline)
                guid = sCharacterCache->GetCharacterGuidByName(name);
        }
    }

    if (player)
    {
        group = player->GetGroup();
        if (!guid || !offline)
            guid = player->GetGUID();
    }
    else
    {
        if (getSelectedPlayer())
            player = getSelectedPlayer();
        else
            player = m_session->GetPlayer();

        if (!guid || !offline)
            guid = player->GetGUID();
        group = player->GetGroup();
    }

    return true;
}

LocaleConstant CliHandler::GetSessionDbcLocale() const
{
    return sWorld->GetDefaultDbcLocale();
}

LocaleConstant CliHandler::GetSessionDbLocaleIndex() const
{
    return sObjectMgr->GetDBCLocaleIndex();
}

std::string const AddonChannelCommandHandler::PREFIX = "TrinityCore";

bool AddonChannelCommandHandler::ParseCommands(std::string_view str)
{
    if (str.length() < 5)
        return false;
    char opcode = str[0];
    echo = &str[1];

    switch (opcode)
    {
    case 'p': // p Ping
        SendAck();
        return true;
    case 'h': // h Issue human-readable command
    case 'i': // i Issue command
    {
        if (!str[5])
            return false;
        humanReadable = (opcode == 'h');
        std::string_view cmd = str.substr(5);
        if (_ParseCommands(cmd)) // actual command starts at str[5]
        {
            if (!hadAck)
                SendAck();
            if (HasSentErrorMessage())
                SendFailed();
            else
                SendOK();
        }
        else
        {
            PSendSysMessage(LANG_CMD_INVALID, STRING_VIEW_FMT_ARG(cmd));
            SendFailed();
        }
        return true;
    }
    default:
        return false;
    }
}

void AddonChannelCommandHandler::Send(std::string const& msg)
{
    WorldPackets::Chat::Chat chat;
    chat.Initialize(CHAT_MSG_WHISPER, LANG_ADDON, GetSession()->GetPlayer(), GetSession()->GetPlayer(), msg, 0, "", LOCALE_enUS, PREFIX);
    GetSession()->SendPacket(chat.Write());
}

void AddonChannelCommandHandler::SendAck() // a Command acknowledged, no body
{
    ASSERT(echo);
    char ack[6] = "a";
    memcpy(ack + 1, echo, 4);
    ack[5] = '\0';
    Send(ack);
    hadAck = true;
}

void AddonChannelCommandHandler::SendOK() // o Command OK, no body
{
    ASSERT(echo);
    char ok[6] = "o";
    memcpy(ok + 1, echo, 4);
    ok[5] = '\0';
    Send(ok);
}

void AddonChannelCommandHandler::SendFailed() // f Command failed, no body
{
    ASSERT(echo);
    char fail[6] = "f";
    memcpy(fail + 1, echo, 4);
    fail[5] = '\0';
    Send(fail);
}

// m Command message, message in body
void AddonChannelCommandHandler::SendSysMessage(std::string_view str, bool escapeCharacters)
{
    ASSERT(echo);
    if (!hadAck)
        SendAck();

    std::string msg = "m";
    msg.append(echo, 4);
    std::string body(str);
    if (escapeCharacters)
        boost::replace_all(body, "|", "||");
    size_t pos, lastpos;
    for (lastpos = 0, pos = body.find('\n', lastpos); pos != std::string::npos; lastpos = pos + 1, pos = body.find('\n', lastpos))
    {
        std::string line(msg);
        line.append(body, lastpos, pos - lastpos);
        Send(line);
    }
    msg.append(body, lastpos, pos - lastpos);
    Send(msg);
}
