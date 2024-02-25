#pragma execution_character_set("utf-8")
#include "_Bot.h"
#include "Player.h"
#include "Config.h"
#include "../game/Globals/ObjectAccessor.h"
// #include "LoginQueryHolder.h"
#include "DatabaseEnv.h"
#include "DatabaseEnvFwd.h"
#include "QueryHolder.h"
#include "../game/Server/WorldSession.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "CustomTalkMenu.h"
#include "PlayerBotMgr.h"
#include "LoginQueryHolder.h"
#include "CharacterHandler.cpp"
#include <stdio.h>

std::unordered_map<uint32, uint32> InFakerMap;
std::unordered_map<uint32, uint32> FakerMap;
std::vector<FakerLocationTemplate> FakerLocation;
class MyBot;

MyBot::~MyBot()
{
    delete this;
}

inline MyBot* MyBot::instance()
{
    static MyBot instance;
    return &instance;
}

void MyBot::Load()
{
	sCustomTalkMenu->Initialize();

	FakerMap.clear();
	FakerLocation.clear();
	InFakerMap.clear();
	QueryResult result;

	if (result = WorldDatabase.PQuery("SELECT 账号ID FROM _假人_在线假人"))
	{
		do
		{
			if (QueryResult charResult = CharacterDatabase.PQuery("SELECT guid FROM `characters` WHERE `account` = '%u' LIMIT 0, 1", result->Fetch()[0].GetUInt32()))
			{
				do
				{
					FakerMap.insert(std::make_pair(result->Fetch()[0].GetUInt32(), charResult->Fetch()[0].GetUInt32()));
				} while (charResult->NextRow());
			}
		} while (result->NextRow());
	}

	if (result = WorldDatabase.PQuery("SELECT 地图ID,X坐标,Y坐标,Z坐标,O坐标 FROM _假人_在线假人_坐标"))
	{
		do
		{
			Field* fields = result->Fetch();
			FakerLocationTemplate Temp;
			Temp.Map = fields[0].GetUInt32();
			Temp.X = fields[1].GetFloat();
			Temp.Y = fields[2].GetFloat();
			Temp.Z = fields[3].GetFloat();
			Temp.O = fields[4].GetFloat();
			FakerLocation.push_back(Temp);
		} while (result->NextRow());
	}
}

void MyBot::LogoutPlayerBot(ObjectGuid guid, bool Save)
{
	if (Player *pPlayerBot = ObjectAccessor::FindPlayer(guid))
	{
		if (pPlayerBot && pPlayerBot->m_bot)
		{
			WorldSession* pPlayerBotWorldSession = pPlayerBot->GetSession();
			uint32 guids = pPlayerBot->GetGUID().GetGUIDLow();
			uint32 accids = pPlayerBot->GetSession()->GetAccountId();
			if (InFakerMap.find(accids) != InFakerMap.end())
				InFakerMap.erase(accids);

			pPlayerBotWorldSession->LogoutPlayer(Save);
			delete pPlayerBotWorldSession;

			CharacterDatabase.PExecute("UPDATE characters SET online = 0 WHERE guid = %u", guids);
			LoginDatabase.PExecute("UPDATE account SET online = 0 WHERE id = %u", accids);
			m_playerBots.erase(guid);
			m_playerBottime.erase(guid);
		}
	}
}

inline void MyBot::UpdateAllSessions(uint32 diff)
{
	for (auto itr = InFakerMap.begin(); itr != InFakerMap.end(); itr++)
	{
		ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(itr->second);
		if (Player* pPlayerBot = ObjectAccessor::FindPlayer(guid))
		if (pPlayerBot->IsBeingTeleportedFar())
			pPlayerBot->GetSession()->HandleWorldPortAck();
	}
}

/*
void WorldSession::HandleMoveWorldportAck()
{
	WorldPacket* packet = nullptr;
	while (_recvQueue.next(packet))
	{
		OpcodeClient opcode = static_cast<OpcodeClient>(packet->GetOpcode());
		uint32 packetSize = packet->size();
		ClientOpcodeHandler const* opHandle = opcodeTable[opcode];
		opHandle->Call(this, *packet);
		delete packet;
	}
}
*/
void MyBot::Update(Player* faker, uint32 diff)
{
	if (!faker->m_bot)
		return;

	if (faker->IsBeingTeleportedNear())
		faker->GetSession()->HandleBotMoveTeleportAck();
	else if (faker->IsInWorld())
		faker->GetSession()->HandleMoveWorldportAck();


	faker->FakerMoveTimer += diff;

	if (faker->FakerMoveTimer >= 10 * IN_MILLISECONDS)
	{
		faker->FakerMoveTimer = 0;

		if (faker->isMoving() || faker->IsBeingTeleported() || FakerLocation.empty())
			return;

		FakerLocationTemplate location = FakerLocation[urand(0, FakerLocation.size() - 1)];

		if (faker->GetMapId() != location.Map)
			faker->TeleportTo(location.Map, location.X + frand(-1, 1), location.Y + frand(-1, 1), location.Z, location.O);
		else
			faker->GetMotionMaster()->MoveCharge(location.X + frand(-1, 1), location.Y + frand(-1, 1), location.Z, faker->GetSpeed(MOVE_RUN));
	}
}

void MyBot::PlBotupdate()
{
	//BOT上线

    if (_charBotLoginCallback.valid() && _charBotLoginCallback.wait_for(Seconds(0)) == std::future_status::ready)
        _charBotLoginCallback.get();
 //    HandlePlayerBotLogin(reinterpret_cast<LoginQueryHolder*>(_charBotLoginCallback.get()));

	std::set<ObjectGuid> guids;
	for (PlayerBotMap::const_iterator itr = m_playerBots.begin(); itr != m_playerBots.end(); ++itr)
	{
		m_playerBottime[itr->first] += 1;
		if (m_playerBottime[itr->first] >= sConfigMgr->GetIntDefault("World.BotoffTime", 10))
			guids.insert(itr->first);
	}

	if (!guids.empty())
	{
		for (std::set<ObjectGuid>::const_iterator itr1 = guids.begin(); itr1 != guids.end(); ++itr1)
			LogoutPlayerBot(*itr1, true);
	}
}

inline void MyBot::AddPlayerBot(uint32 accid, uint32 playerGuid)
{
    ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(playerGuid);
    if (Player* botplr = ObjectAccessor::FindPlayer(guid))
        return;


    CharacterQueryHolder* holder = new CharacterQueryHolder(accid, guid);
    if (!holder->Initialize())
    {
        delete holder;
        return;
    }

   //  charBotLoginCallback. = CharacterDatabase.DelayQueryHolder((SQLQueryHolderBase*)holder);

}
/*
// This is the origianl code !!
void MyBot::AddPlayerBot(uint32 accid, uint32 playerGuid)
{
    ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(playerGuid);
    if (Player* botplr = ObjectAccessor::FindPlayer(guid))
        return;

    LoginQueryHolder* holder = new LoginQueryHolder(accid, guid);
    if (!holder->Initialize())
    {
        delete holder;
        return;
    }

    _charBotLoginCallback = CharacterDatabase.DelayQueryHolder((SQLQueryHolder*)holder);
}
*/
void MyBot::LoginPlayerBot(uint32 cumot)
{
	for (auto it = FakerMap.begin(); it != FakerMap.end(); it++)
	{
		if (cumot < 1)
			continue;

		if(InFakerMap.find(it->first) != InFakerMap.end())
			continue;

		ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(it->second);
		if (Player* botplr = ObjectAccessor::FindPlayer(guid))
			continue;

		cumot--;

		InFakerMap.insert(std::make_pair(it->first, it->second));
		AddPlayerBot(it->first, it->second);
	}

}

void MyBot::HandlePlayerBotLogin(CharacterQueryHolder* holder)
{
	if (!holder)
		return;


    LoginQueryHolder* lqh = (LoginQueryHolder*)holder;
	if (!lqh)
	{
		if (holder)
			delete holder;
		return;
	}
    
    char text[1] = "";

	std::string so = "Wn64";
	const std::shared_ptr<WorldSocket> sock;
    WorldSession* botSession = new WorldSession(lqh->GetAccountId(), text, 0, sock, SEC_ADMINISTRATOR, 8, 0, so, LOCALE_enUS, 0, false); // , AT_AUTH_FLAG_NONE, 0);
	if (!botSession)
	{
		if (holder)
			delete holder;
		if (botSession)
			delete botSession;
		return;
	}
    botSession->SetAddress("playbot");
	botSession->Setexpansion(9);
    
    ObjectGuid guid = lqh->GetGuid();
	if (!guid)
	{
		if (holder)
			delete holder;
		if (botSession)
			delete botSession;
		return;
	}

    botSession->HandleBotPlayerLogin(lqh);
	Player *botPlayer = botSession->GetPlayer();

    TC_LOG_INFO("player", "player bot 1111111111 Login...");

	if (!botPlayer)
	{
		if (holder)
			delete holder;
		if (botSession)
			delete botSession;
		return;
	}

	botPlayer->m_bot = true;

	botPlayer->m_plguid = 0;

	m_playerBots[guid] = botPlayer;
}

class bot_commandscript : public CommandScript
{
public:
	bot_commandscript() : CommandScript("bot_commandscript") { }

	std::vector<ChatCommand> GetCommands() const override
	{
		static std::vector<ChatCommand> MybotcommandTable =
		{
            { "login",			rbac::RBAC_ROLE_ADMINISTRATOR,  false, &OnlineCmd, ""},
            { "logout",			rbac::RBAC_ROLE_ADMINISTRATOR,  false, &OutlineCmd, ""},
            { "max",			rbac::RBAC_ROLE_ADMINISTRATOR,  false, &MaxCmd, ""},
		};

		static std::vector<ChatCommand> commandTable =
		{
			{ "bot",             rbac::RBAC_ROLE_ADMINISTRATOR,  false, NULL,            "", MybotcommandTable }
		};
		return commandTable;
	}

    static bool MaxCmd(ChatHandler* handler, const char* args)
    {
		int max = atoi(args);
		if (max > 0)
	        sPlayerBotMgr->SetMax(max);
        return true;
    }

    static bool OnlineCmd(ChatHandler* handler, const char* args)
    {
		sPlayerBotMgr->AllPlayerBotRandomLogin(args);
        return true;
    }

    static bool OutlineCmd(ChatHandler* handler, const char* args)
    {
        sPlayerBotMgr->AllPlayerBotLogout();
        return true;
    }
};

class FakerLogin : PlayerScript
{
public:
	FakerLogin() : PlayerScript("FakerLogin") {}

	void OnUpdate(Player* player, uint32 diff) 
	{
		if (!player->m_bot)
			return;

	}

	void OnLogin(Player* player)
	{
		if (!player->m_bot)
			return;
	}

    bool operator==(const FakerLogin& other) const = default;
};

void AddSC_bot_commandscript()
{
	new FakerLogin();
	new bot_commandscript();
}

