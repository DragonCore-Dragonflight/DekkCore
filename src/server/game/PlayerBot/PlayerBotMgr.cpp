//
#include "PlayerBotMgr.h"
#include "World.h"
#include "DB2Stores.h"
#include "Player.h"
#include "BattlegroundMgr.h"
#include "BotAI.h"
#include "BotFieldAI.h"
#include "BotGroupAI.h"
#include "BotDuelAI.h"
#include "BotArenaAI.h"
#include "OnlineMgr.h"
#include "Group.h"
#include "SocialMgr.h"
#include "LFGMgr.h"
#include "Config.h"
#include "PlayerBotSession.h"
#include "AccountMgr.h"
#include "CharacterPackets.h"
#include "WorldSession.h"
#include "World.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "OnlineMgr.h"

PlayerBotCharBaseInfo PlayerBotBaseInfo::empty;
std::map<uint32, std::list<UnitAI*> > PlayerBotMgr::m_DelayDestroyAIs;
std::mutex PlayerBotMgr::g_uniqueLock;

std::string PlayerBotCharBaseInfo::GetNameANDClassesText()
{
    //std::string clsName;
    uint32 clsEntry = 620000;
    switch (profession)
    {
        case 1:
            //clsName = "  O?  Ez : ";
            clsEntry += 1;
            break;
        case 2:
            //clsName = "  EACdEz : ";
            clsEntry += 2;
            break;
        case 3:
            //clsName = "  ��  C� : ";
            clsEntry += 3;
            break;
        case 4:
            //clsName = "  ��  �� : ";
            clsEntry += 4;
            break;
        case 5:
            //clsName = "  ��  E� : ";
            clsEntry += 5;
            break;
        case 6:
            //clsName = "  �R  Cd : ";
            clsEntry += 6;
            break;
        case 7:
            //clsName = "  Cr  �� : ";
            clsEntry += 7;
            break;
        case 8:
            //clsName = "  ��  E� : ";
            clsEntry += 8;
            break;
        case 9:
            //clsName = "  Eo  Ez : ";
            clsEntry += 9;
            break;
        case 11:
            //clsName = "  ���lN� : ";
            clsEntry += 10;
            break;
    }
    std::string clsText = sObjectMgr->GetTrinityStringForDBCLocale(clsEntry);
    //consoleToUtf8(clsName, clsText);
    return clsText + name;
}

uint32 PlayerBotBaseInfo::GetCharIDByNoArenaType(bool faction, uint32 prof, uint32 arenaType, std::vector<ObjectGuid>& fliters)
{
    for (ObjectGuid& guid : fliters)
    {
        if (ExistCharacterByGUID(guid))
            return 0;
    }
    if (characters.size() <= 0)
        return 0;
    for (CharInfoMap::iterator it = characters.begin();
        it != characters.end();
        it++)
    {
        if (!MatchRaceByFuction(faction, it->second.race))
            continue;
        if (it->second.profession != prof)
            continue;
    }
    return 0;
}

PlayerBotMgr::PlayerBotMgr() :
    m_BotAccountAmount(90),
    m_LastBotAccountIndex(0),
    m_MaxOnlineBot(10),
    m_BotOnlineCount(0),
    m_LFGSearchTick(0),
    m_ArenaSearchTick(0)
{
    m_BGTypes.push_back(BattlegroundTypeId::BATTLEGROUND_WS);
    m_BGTypes.push_back(BattlegroundTypeId::BATTLEGROUND_AB);
    m_BGTypes.push_back(BattlegroundTypeId::BATTLEGROUND_EY);
#ifndef NON_SINGLE_GAME
   /* m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundAlteracValley);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundIsleOfConquest);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::BattlegroundRandom);

    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldNagrandArena);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldBladesEdgeArena);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaRuinsOfLordaeron);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaOldRingOfValor);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaBladesEdge);
    m_BGTypes.push_back(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);*/
    m_BGTypes.push_back(BattlegroundTypeId::BATTLEGROUND_SA);
#endif
}

PlayerBotMgr::~PlayerBotMgr()
{
    ClearBaseInfo();
}

PlayerBotMgr* PlayerBotMgr::instance()
{
    static PlayerBotMgr instance;
    return &instance;
}

void PlayerBotMgr::SwitchPlayerBotAI(Player* player, PlayerBotAIType aiType, bool force)
{
    if (!force && player->IsInCombat())
        return;
    if (force && player->IsInCombat())
        player->ClearInCombat();
    player->SetSelection(ObjectGuid::Empty);
    UnitAI* pAI = player->GetAI();
    switch (aiType)
    {
        case PlayerBotAIType::PBAIT_FIELD:
            if (pAI)
            {
                if (dynamic_cast<BotFieldAI*>(pAI) != NULL)
                {
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotFieldAI::CreateBotFieldAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled();
            }
            break;
        case PlayerBotAIType::PBAIT_GROUP:
            if (pAI)
            {
                if (dynamic_cast<BotGroupAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled();
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotGroupAI::CreateBotGroupAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled();
            }
            break;
        case PlayerBotAIType::PBAIT_DUEL:
            if (pAI)
            {
                if (dynamic_cast<BotDuelAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled();
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotDuelAI::CreateBotDuelAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled();
                ((BotDuelAI*)pAI)->ResetBotAI();
            }
            break;
        case PlayerBotAIType::PBAIT_ARENA:
#ifndef CONVERT_ARENAAI_TOBG
            if (pAI)
            {
                if (dynamic_cast<BotArenaAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled = true;
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            pAI = BotArenaAI::CreateBotArenaAIByPlayerClass(player);
            if (pAI)
            {
                pAI->Reset();
                player->SetAI(pAI);
                player->IsAIEnabled = true;
            }
            break;
#endif
        case PlayerBotAIType::PBAIT_BG:
            if (pAI)
            {
                if (dynamic_cast<BotBGAI*>(pAI) != NULL)
                {
                    player->IsAIEnabled();
                    return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            player->SetAI(BotBGAI::CreateBotBGAIByPlayerClass(player));
            player->IsAIEnabled();
            break;
        case PlayerBotAIType::PBAIT_DUNGEON:
            if (pAI)
            {
                if (dynamic_cast<BotFieldAI*>(pAI) != NULL)
                {
                	player->IsAIEnabled();
                	return;
                }
                PlayerBotMgr::m_DelayDestroyAIs[getMSTime()].push_back(pAI);
                player->SetAI(NULL);
            }
            break;
    }
}

std::string PlayerBotMgr::GetPlayerLinkText(Player const* player) const
{
    const std::string& name = player->GetName();
    if (player->GetTeamId() == TeamId::TEAM_ALLIANCE)
        return "|cff0000ff|Hplayer:" + name + "|h[" + name + "]|h|r";
    else if (player->GetTeamId() == TeamId::TEAM_HORDE)
        return "|cffff0000|Hplayer:" + name + "|h[" + name + "]|h|r";
    return "|cffffffff|Hplayer:" + name + "|h[" + name + "]|h|r";
}

PlayerBotSession* PlayerBotMgr::GetBotSessionByCharGUID(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        if (!(pInfo->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(pInfo->id);
        if (!pWorldSession)
            return NULL;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        return pSession;
    }
    return NULL;
}

TeamId PlayerBotMgr::GetTeamIDByPlayerBotGUID(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        TeamId team = pInfo->GetTeamIDByChar(guid);
        if (team == TeamId::TEAM_NEUTRAL)
            continue;
        return team;
    }
    return TeamId::TEAM_NEUTRAL;
}

bool PlayerBotMgr::IsPlayerBot(WorldSession* pSession)
{
    if (pSession && typeid(pSession) == typeid(PlayerBotSession)) //   if (pSession && dynamic_cast<PlayerBotSession*> (pSession)) not work
        return true;
    return false;
}

bool PlayerBotMgr::IsBotAccuntName(std::string name)
{
    if (name.size() < 10)
        return false;
    std::string head = name.substr(0, 9);
    if (head != "playerbot")
        return false;
    std::string numText = name.substr(9);
    int num = atoi(numText.c_str());
    return num > 0;
}

bool PlayerBotMgr::IsIDLEPlayerBot(Player* player)
{
    if (!player/* || player->isBeingLoaded() */|| !player->IsInWorld())
        return false;
    if (player->IsInCombat() || !player->IsAlive())
        return false;
    if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() ||
        player->GetGroup() || player->isUsingLfg())
        return false;
    if (player->InBattlegroundQueue())
        return false;
    // if (!player->IsSettingFinish())
    //     return false;
    if (player->HasAura(26013))
        return false;
    return true;
}

std::set<uint32> PlayerBotMgr::GetArenaTeamPlayerBotIDCountByTeam(TeamId team, int32 count, ArenaTeamTypes type)
{
    std::vector<uint32> allInfoEntrys;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin(); itInfo != m_idPlayerBotBase.end(); itInfo++)
        allInfoEntrys.push_back(itInfo->first);
    Trinity::Containers::RandomShuffle(allInfoEntrys);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(allInfoEntrys.begin(), allInfoEntrys.end(), std::default_random_engine(seed));

    std::set<uint32> ids;
    if (allInfoEntrys.empty())
        return ids;
    for (uint32 infoEntry : allInfoEntrys)
    {
        PlayerBotBaseInfo* info = GetPlayerBotAccountInfo(infoEntry);
        if (!info)
            continue;
        std::vector<uint32> faildIDs = info->GetNoArenaTeamCharacterIDsByFuction((team == TEAM_ALLIANCE) ? true : false, type);
        if (faildIDs.empty())
            continue;
        for (uint32 id : faildIDs)
        {
            if (ids.find(id) != ids.end())
                continue;
            ids.insert(id);
            break;
        }
        if (ids.size() >= uint32(count))
            return ids;
    }
    ids.clear();
    return ids;
}

PlayerBotBaseInfo* PlayerBotMgr::GetPlayerBotAccountInfo(uint32 guid)
{
    std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.find(guid);
    if (it == m_idPlayerBotBase.end())
        return NULL;
    return it->second;
}

PlayerBotBaseInfo* PlayerBotMgr::GetAccountBotAccountInfo(uint32 guid)
{
    std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.find(guid);
    if (it == m_idAccountBotBase.end())
        return NULL;
    return it->second;
}

bool PlayerBotMgr::ExistClassByRace(uint8 race, uint8 prof)
{
    switch (prof)
    {
        case 1:
            return (race != 10);
        case 2:
            return (race == 1 || race == 3 || race == 10 || race == 11);
        case 3:
            return (race == 2 || race == 3 || race == 4 || race == 6 || race == 8 || race == 10 || race == 11);
        case 4:
            return (race == 1 || race == 2 || race == 3 || race == 4 || race == 5 || race == 7 || race == 8 || race == 10);
        case 5:
            return (race == 1 || race == 3 || race == 4 || race == 5 || race == 8 || race == 10 || race == 11);
        case 6:
            return true;
        case 7:
            return (race == 2 || race == 6 || race == 8 || race == 11);
        case 8:
            return (race == 1 || race == 5 || race == 7 || race == 8 || race == 10 || race == 11);
        case 9:
            return (race == 1 || race == 2 || race == 5 || race == 7 || race == 10);
        case 11:
            return (race == 4 || race == 6);
    }
    return false;
}

void PlayerBotMgr::InitializeCreatePlayerBotName()
{
    allName.clear();
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ALLRANDOM_NAME);
    PreparedQueryResult result = LoginDatabase.Query(stmt);
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            std::string dbName = fields[0].GetString();
            if (dbName.size() > 0)
                allName.push_back(dbName);
        } while (result->NextRow());
    }

    allArenaName.clear();
    LoginDatabasePreparedStatement* stmt2 = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ALLARENA_NAME);
    PreparedQueryResult result2 = LoginDatabase.Query(stmt2);
    if (result2)
    {
        do
        {
            Field* fields = result2->Fetch();
            std::string dbName = fields[0].GetString();
            if (dbName.size() > 0)
                allArenaName.push_back(dbName);
        } while (result2->NextRow());
    }
}

std::string PlayerBotMgr::RandomName()
{
    if (allName.size() <= 0)
        InitializeCreatePlayerBotName();
    int32 maxLoop = allName.size() / 2;
    if (maxLoop <= 0)
        return "";
    do
    {
        uint32 index = irand(0, allName.size() - 1);
        std::string selectName = allName[index];
        std::string sql = "SELECT guid FROM characters WHERE name = '" + selectName + "'";
        QueryResult result = CharacterDatabase.Query(sql.c_str());
        if (!result)
            return selectName;
        --maxLoop;
        if (maxLoop <= 0)
            break;
    } while (true);
    return "";
}

std::string PlayerBotMgr::RandomArenaName()
{
    return "Arena" + std::to_string(urand(0, 100));
}
 
uint8 PlayerBotMgr::RandomRace(bool group, uint8 prof)
{
    uint8 race = 1;
    for (int i = 0; i < 80; i++)
    {
        race = irand(1, 11);
        if (race == 9) continue;
        if (group)
        {
            if (race == 1 || race == 3 || race == 4 || race == 7 || race == 11)
            {
                if (ExistClassByRace(race, prof))
                    return race;
            }
        }
        else
        {
            if (race == 2 || race == 5 || race == 6 || race == 8 || race == 10)
            {
                if (ExistClassByRace(race, prof))
                    return race;
            }
        }
    }

    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom Race timeout!");
    if (group)
        return 1;
    return 2;
}

uint8 PlayerBotMgr::RandomSkinColor(uint8 race, uint8 gender, uint8 prof)
{
    uint8 skinColor = 0;
  /*  for (int i = 0; i < 80; i++)
    {
        skinColor = irand(0, 9);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Skin, 0, skinColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return skinColor;
        }
    }*/
    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom SkinColor timeout!");
    return 1;
}

uint8 PlayerBotMgr::RandomFace(uint8 race, uint8 gender, uint8 skinColor, uint8 prof)
{
    uint8 faceID = 0;
 /*   for (int i = 0; i < 80; i++)
    {
        faceID = irand(0, 9);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Face, faceID, skinColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return faceID;
        }
    }*/
    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom Face timeout!");
    return 1;
}

uint8 PlayerBotMgr::RandomHair(uint8 race, uint8 gender, uint8 prof)
{
    uint8 hairID = 0;
    /*for (uint8 hc = 0; hc < 80; hc++)
    {
        for (int i = 0; i < 80; i++)
        {
            hairID = irand(0, 8);
            CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Hair, hairID, hc);
            if (entry)
            {
                if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                    continue;
                if (!(entry->Flags & SECTION_FLAG_PLAYER))
                    continue;
                return hairID;
            }
        }
    }*/
    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom Hair timeout!");
    return 0;
}

uint8 PlayerBotMgr::RandomHairColor(uint8 race, uint8 gender, uint8 hairID, uint8 prof)
{
    uint8 hairColor = 0;
   /* for (int i = 0; i < 80; i++)
    {
        hairColor = irand(0, 8);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::Hair, hairID, hairColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return hairColor;
        }
    }*/
    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom HairColor timeout!");
    return 1;
}

uint8 PlayerBotMgr::RandomFacialHair(uint8 race, uint8 gender, uint8 hairColor, uint8 prof)
{
    uint8 facialHair = 0;
    /*for (int i = 0; i < 80; i++)
    {
        facialHair = irand(0, 7);
        CharSectionsEntry const* entry = sDB2Manager.GetCharSectionEntry(race, gender, CharBaseSectionVariation::FacialHair, facialHair, hairColor);
        if (entry)
        {
            if ((entry->Flags & SECTION_FLAG_DEATH_KNIGHT) && prof != CLASS_DEATH_KNIGHT)
                continue;
            if (!(entry->Flags & SECTION_FLAG_PLAYER))
                continue;
            return facialHair;
        }
    }*/
    TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Random player custom FacialHair timeout!");
    return 0;
}

WorldPacket PlayerBotMgr::BuildCreatePlayerData(bool group, uint8 prof)
{
    std::string name = RandomName();
    uint8 race = RandomRace(group, prof);
    uint8 gender = irand(0, 1);
    uint8 skinColor = RandomSkinColor(race, gender, prof);
    uint8 faceID = RandomFace(race, gender, skinColor, prof);
    uint8 hairID = RandomHair(race, gender, prof);
    uint8 hairColor = RandomHairColor(race, gender, hairID, prof);
    uint8 facialHair = RandomFacialHair(race, gender, hairColor, prof);
    WorldPacket cmd(0x36);
    cmd << name;
    cmd << race;
    cmd << prof;
    cmd << gender;
    cmd << skinColor;
    cmd << faceID;
    cmd << hairID;
    cmd << hairColor;
    cmd << facialHair;
    cmd << 0;//OutfitId

    return cmd;
}

void PlayerBotMgr::CreateOncePlayerBot()
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        if (pInfo->needCreateBots.size() > 0)
        {
            WorldPacket& packet = pInfo->needCreateBots.front();
            WorldSession* pSession = sWorld->FindSession(pInfo->id);
            if (pSession)
            {
                std::shared_ptr<WorldPackets::Character::CharacterCreateInfo> CreateInfo;
             //   pSession->HandleCharCreateOpcode(packet);
            }
            pInfo->needCreateBots.pop();
            break;
        }
    }
}

void PlayerBotMgr::ClearBaseInfo()
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        delete it->second;
    }
    m_idPlayerBotBase.clear();
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        delete it->second;
    }
    m_idAccountBotBase.clear();
}

void PlayerBotMgr::UpdateLastAccountIndex(std::string& username)
{
    std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    if (username.empty())
        return;
    std::string querySql = "SELECT id FROM account WHERE username='" + username + "'";
    QueryResult result = LoginDatabase.Query(querySql.c_str());
    if (result)
    {
        Field* fields = result->Fetch();
        if (fields)
        {
            uint32 id = fields[0].GetUInt32();
            m_LastBotAccountIndex = id;
        }
    }
}

void PlayerBotMgr::SupplementAccount()
{
    uint32 needAccount = m_BotAccountAmount * 2 - m_idPlayerBotBase.size();
    if (needAccount <= 0)
        return;

    for (uint32 i = 0; i < needAccount; i++)
    {
        ++m_LastBotAccountIndex;
        char indexText[25] ={ 0 };
        PlayerBotMgr::itoa(m_LastBotAccountIndex, indexText);
        std::string userName = "playerbot@hotmail.com";
        userName += indexText;
        if (AccountMgr::instance()->CreateAccount(userName, "botxxx") == AccountOpResult::AOR_OK)
        {
            std::string querySql = "SELECT id, email, sha_pass_hash FROM battlenet_accounts WHERE email='";
            querySql += userName + "'";
            QueryResult result = LoginDatabase.Query(querySql.c_str());
            if (result)
            {
                Field* fields = result->Fetch();
                if (fields)
                {
                    uint32 id = fields[0].GetUInt32();
                    std::string username = fields[1].GetString();
                    std::string pass = fields[2].GetString();

                    std::string lowerName = boost::algorithm::to_lower_copy(username);
                    if (m_idPlayerBotBase.find(id) == m_idPlayerBotBase.end())
                    {
                        PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, false);
                        m_idPlayerBotBase[id] = pInfo;
                    }
                    m_LastBotAccountIndex = id;
                }
            }
        }
    }
}

void PlayerBotMgr::DestroyBotMail(uint32 guid)
{
    char sql[256] ={ 0 };
    sprintf_s(sql, 255, "DELETE FROM mail WHERE receiver = %d", guid);
    CharacterDatabase.Execute(sql);
    memset(sql, 0, 256);
    sprintf_s(sql, 255, "DELETE FROM mail_items WHERE receiver = %d", guid);
    CharacterDatabase.Execute(sql);
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEMS);
    stmt->setUInt32(0, guid);
    CharacterDatabase.Execute(stmt);
}

void PlayerBotMgr::AddNewAccountBotBaseInfo(std::string name)
{
    std::string upperName = boost::algorithm::to_upper_copy(name);
    std::string sql("SELECT id, email, sha_pass_hash FROM battlenet_accounts WHERE `email`='"); sql += upperName + "'";
    QueryResult result = LoginDatabase.Query(sql.c_str());
    if (!result)
        return;
    Field* fields = result->Fetch();
    uint32 id = fields[0].GetUInt32();
    std::string username = fields[1].GetString();
    std::string pass = fields[2].GetString();
    if (m_idAccountBotBase.find(id) == m_idAccountBotBase.end())
    {
        PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, true);
        m_idAccountBotBase[id] = pInfo;
    }
}

void PlayerBotMgr::LoadPlayerBotBaseInfo()
{
    uint32 oldMSTime = getMSTime();

    ClearBaseInfo();
    QueryResult result = LoginDatabase.Query("SELECT id, email, sha_pass_hash FROM battlenet_accounts");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> LoadPlayerBot Find 0 account!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        std::string username = fields[1].GetString();
        std::string pass = fields[2].GetString();
        sOnlineMgr->AddNewAccount(id, username); // Real player acc and bot acc all in

        std::string lowerName = boost::algorithm::to_lower_copy(username);
        if (IsBotAccuntName(lowerName))
        {
            if (m_idPlayerBotBase.find(id) == m_idPlayerBotBase.end())
            {
                PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, false);
                m_idPlayerBotBase[id] = pInfo;
            }
            m_LastBotAccountIndex = id;
        }
        else
        {
            if (m_idAccountBotBase.find(id) == m_idAccountBotBase.end())
            {
                PlayerBotBaseInfo* pInfo = new PlayerBotBaseInfo(id, username.c_str(), pass, true);
                m_idAccountBotBase[id] = pInfo;
            }
        }
        if (m_idPlayerBotBase.size() >= m_BotAccountAmount * 2)
        	break;
    } while (result->NextRow());

    SupplementAccount();

    if (m_idPlayerBotBase.size() > 0 || m_idAccountBotBase.size() > 0)
        LoadCharBaseInfo();

    TC_LOG_INFO("server.loading", ">> Loaded {} Player bot account base info in {} ms", m_idPlayerBotBase.size(), GetMSTimeDiffToNow(oldMSTime));
}

void PlayerBotMgr::LoadCharBaseInfo()
{
    QueryResult result = CharacterDatabase.Query("SELECT guid, account, name, race, class, gender, level FROM characters");

    if (!result)
    {
        TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> LoadPlayerBot Find 0 characters!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint64 guid = fields[0].GetUInt64();
        uint32 accID = fields[1].GetUInt32();
        PlayerBotBaseInfo* pInfo = GetPlayerBotAccountInfo(accID);
        if (pInfo)
        {
            if (pInfo->characters.find(guid) == pInfo->characters.end())
            {
                std::string charName = fields[2].GetString();
                uint16 race = fields[3].GetInt16();
                uint16 pro = fields[4].GetInt16();
                uint16 gender = fields[5].GetInt16();
                uint16 level = fields[6].GetInt16();
                pInfo->characters.emplace(guid, PlayerBotCharBaseInfo{ guid, accID, charName, race, pro, gender, level });
                DestroyBotMail(guid);
            }
        }
        else if (pInfo = GetAccountBotAccountInfo(accID))
        {
            if (pInfo->characters.find(guid) == pInfo->characters.end())
            {
                std::string charName = fields[2].GetString();
                uint16 race = fields[3].GetInt16();
                uint16 pro = fields[4].GetInt16();
                uint16 gender = fields[5].GetInt16();
                uint16 level = fields[6].GetInt16();
                pInfo->characters.emplace(guid, PlayerBotCharBaseInfo{ guid, accID, charName, race, pro, gender, level });
                DestroyBotMail(guid);
            }
        }
    } while (result->NextRow());
}

void PlayerBotMgr::UpAllPlayerBotSession()
{
    PlayerBotSetting::Initialize();
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        UpPlayerBotSessionByBaseInfo(pInfo, false);
    }
}

PlayerBotSession* PlayerBotMgr::UpPlayerBotSessionByBaseInfo(PlayerBotBaseInfo* pAcc, bool accountInfo)
{
    if (!pAcc)
        return NULL;
    auto pSession = sWorld->FindSession(pAcc->id);
    if (pSession)
        return NULL;
    std::string name = pAcc->username.c_str();
    auto pBotSession = std::shared_ptr<PlayerBotSession>(new PlayerBotSession{ pAcc->id, name, AccountTypes::SEC_GAMEMASTER, 2, 0, LocaleConstant::LOCALE_zhTW, 0, false }); // SHARED PTR NOT WORK 
    // pSession->ReadAddonsInfo("");
    if (accountInfo)
        pBotSession->SetAccountBotSession();

    sWorld->AddSession(pSession);
    return pBotSession.get();
}

void PlayerBotMgr::OnPlayerBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level)
{
    PlayerBotBaseInfo* pInfo = GetPlayerBotAccountInfo(accountId);
    if (!pInfo)
        return;
    uint32 id = guid.GetGUIDLow();
    if (pInfo->characters.find(id) != pInfo->characters.end())
    {
        return;
    }
    pInfo->characters[id] = PlayerBotCharBaseInfo(id, accountId, name, uint16(race), uint16(playerClass), uint16(gender), uint16(level));

    CreateOncePlayerBot();
}

void PlayerBotMgr::OnAccountBotCreate(ObjectGuid const& guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level)
{
    PlayerBotBaseInfo* pInfo = GetAccountBotAccountInfo(accountId);
    if (!pInfo)
        return;
    uint32 id = guid.GetGUIDLow();
    if (pInfo->characters.find(id) != pInfo->characters.end())
    {
        return;
    }
    pInfo->characters[id] = PlayerBotCharBaseInfo(id, accountId, name, uint16(race), uint16(playerClass), uint16(gender), uint16(level));
}

void PlayerBotMgr::OnAccountBotDelete(ObjectGuid& guid, uint32 accountId)
{
    PlayerBotBaseInfo* pInfo = GetAccountBotAccountInfo(accountId);
    if (!pInfo)
        return;
    pInfo->RemoveCharacterByGUID(guid);
}

void PlayerBotMgr::OnPlayerBotLogin(WorldSession* pSession, Player* pPlayer)
{
    ++m_BotOnlineCount;
    Group* pGroup = pPlayer->GetGroup();
    if (pGroup)
    {
        if (!pGroup->GroupExistRealPlayer())
        {
            pPlayer->RemoveFromGroup(RemoveMethod::GROUP_REMOVEMETHOD_LEAVE);
        }
        else
            SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_GROUP, false);
    }
    else
        SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_FIELD, false);

    if (pPlayer->GetBattleground())
    {
        pPlayer->LeaveBattleground();
    //    if (pSession)
        //    pSession->HandleWorldPortAck();
    }

    BotUtility::RemoveArenaBotSpellsByPlayer(pPlayer);
    sPlayerBotTalkMgr->JoinDefaultChannel(pPlayer);
    if (pSession)
    {
        std::string outString;
        consoleToUtf8(std::string(" �D D�"), outString);
        sWorld->SendGlobalText((GetPlayerLinkText(pPlayer) + outString).c_str(), NULL);
    }
    if (PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession))
    {
        if (!pSession->HasSchedules()/* && !pPlayer->EquipIsTidiness()*/)
        {
            uint32 curTType = pPlayer->GetActiveTalentGroup(); 
            uint32 newTType = curTType;
            while (newTType == curTType)
                newTType = urand(0, 2);
            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = pPlayer->GetLevel();
            schedule2.parameter2 = pPlayer->GetLevel();
            schedule2.parameter3 = newTType + 1;
            pBotSession->PushScheduleToQueue(schedule2);
        }
    }
}

void PlayerBotMgr::OnPlayerBotLogout(WorldSession* pSession)
{
    --m_BotOnlineCount;
    if (m_BotOnlineCount < 0) m_BotOnlineCount = 0;

    std::string outString;
    consoleToUtf8(std::string("��C�C�D�D�"), outString);
    sWorld->SendGlobalText(outString.c_str(), NULL);
    PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession);
    if (pBotSession && !pBotSession->HasScheduleByType(BotGlobleScheduleType::BGSType_Online) &&
        !pBotSession->HasScheduleByType(BotGlobleScheduleType::BGSType_Online_GUID))
        pBotSession->ClearAllSchedule();
}

void PlayerBotMgr::OnPlayerBotLeaveOriginalGroup(Player* pPlayer)
{
    SwitchPlayerBotAI(pPlayer, PlayerBotAIType::PBAIT_FIELD, true);
}

void PlayerBotMgr::LoginGroupBotByPlayer(Player* pPlayer)
{
#ifndef INCOMPLETE_BOT
    if (pPlayer->IsPlayerBot())
        return;
    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup || pGroup->isBFGroup() || pGroup->isBGGroup())
        return;
    Group::MemberSlotList const& memList = pGroup->GetMemberSlots();
    for (Group::MemberSlot const& slot : memList)
    {
        ObjectGuid guid = slot.guid;
        if (guid == pPlayer->GetGUID())
            continue;
        Player* friendPlayer = ObjectAccessor::FindPlayer(guid);
        if (friendPlayer)
            continue;
        if (isok==0)
            AddNewPlayerBotByGUID2(guid);
        else
            DelayLoginPlayerBotByGUID(guid);
    }
#endif
}

void PlayerBotMgr::LoginFriendBotByPlayer(Player* pPlayer)
{
    #ifndef INCOMPLETE_BOT
    	if (pPlayer->IsPlayerBot())
    		return;
    	int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    
   /* 	std::vector<ObjectGuid> friends = pPlayer->GetSocial()->Get();
    	for (ObjectGuid& guid : friends)
    	{
    
    	if (isok==0)
    		AddNewPlayerBotByGUID2(guid);
    		else
    		DelayLoginPlayerBotByGUID(guid);
    	}*/
    #else
    	std::string allonlineText;
    	consoleToUtf8(std::string("|cffff8800ElN�c�T��OU�?sA�N��C�C��DD�?L|r"), allonlineText);
    	sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    #endif
}

void PlayerBotMgr::LogoutAllGroupPlayerBot(Group* pGroup, bool force)
{
    if (!pGroup)
        return;
    if (!force && pGroup->GroupExistRealPlayer())
        return;
    Group::MemberSlotList const& memList = pGroup->GetMemberSlots();
    for (Group::MemberSlot const& slot : memList)
    {
        ObjectGuid guid = slot.guid;
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || !player->IsPlayerBot())
            continue;
        WorldSession* pSession = player->GetSession();
        if (pSession)
            pSession->LogoutPlayer(false);
    }
}

bool PlayerBotMgr::AllPlayerLeaveBG(uint32 account)
{
    if (account == 0)
        return false;
    std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    WorldSession* pSession = sWorld->FindSession(account);
    if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading())
    {
        Player* pPlayer = pSession->GetPlayer();
        if (!pPlayer || pPlayer->GetTransport() || !pPlayer->InBattleground())
            return false;
        const Battleground* bg = pPlayer->GetBattleground();
        if (!bg || bg->GetStatus() == BattlegroundStatus::STATUS_WAIT_LEAVE || bg->GetStatus() == BattlegroundStatus::STATUS_WAIT_JOIN)
            return false;
        ((Battleground*)bg)->EndBattleground((pPlayer->GetTeamId() == TEAM_ALLIANCE) ? HORDE : ALLIANCE);
        return true;
    }
    return false;
}

void PlayerBotMgr::AllPlayerBotRandomLogin(const char* name)
{
    int32 needOnline = m_MaxOnlineBot - m_BotOnlineCount;
    if (needOnline <= 0)
        return;
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin(); it != m_idPlayerBotBase.end(); it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        WorldSession* pSession = sWorld->FindSession(pInfo->id)->GetPlayer()->GetSession();
        if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading() && !pSession->GetPlayer())
        {
            if (pInfo->characters.size() <= 0)
                continue;

            if (name[0] != '\0')
            {
                for (auto i = 0; i < pInfo->characters.size(); ++i)
                {
                    if (strcmp(name, pInfo->characters[i].name.c_str()) == 0)
                    {
                        PlayerBotCharBaseInfo& charInfo = pInfo->characters[i];
                        WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
                        WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
                        cmd.Guid = ObjectGuid::Create<HighGuid::Player>(charInfo.guid);
                        cmd.FarClip = 0.0f;
                        pSession->HandlePlayerLoginOpcode(cmd);
                        pSession->HandleContinuePlayerLogin();
                        return;
                    }
                }
            }
            else
            {
                int sel = irand(0, pInfo->characters.size() - 1);
                for (auto itChar = pInfo->characters.begin();
                    itChar != pInfo->characters.end();
                    itChar++)
                {
                    if (sel <= 0)
                    {
                        PlayerBotCharBaseInfo& charInfo = itChar->second;
                        WorldPacket _worldPacket(CMSG_PLAYER_LOGIN);
                        WorldPackets::Character::PlayerLogin cmd(std::move(_worldPacket));
                        cmd.Guid = ObjectGuid::Create<HighGuid::Player>(charInfo.guid);
                        cmd.FarClip = 0.0f;
                        pSession->HandlePlayerLoginOpcode(cmd);
                        pSession->HandleContinuePlayerLogin();
                        --needOnline;
                        break;
                    }
                    --sel;
                }
            }
            if (needOnline <= 0)
                return;
        }
    }
}

void PlayerBotMgr::AllPlayerBotLogout()
{
    std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idPlayerBotBase.begin();
        it != m_idPlayerBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        PlayerBotLogout(pInfo->id);
    }
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        PlayerBotLogout(pInfo->id);
    }
}

bool PlayerBotMgr::PlayerBotLogout(uint32 account)
{
    if (account == 0)
        return false;
    std::unique_lock<std::mutex> sessionGuard(PlayerBotMgr::g_uniqueLock);
    WorldSession* pSession = sWorld->FindSession(account);
    if (pSession && pSession->IsBotSession() && !pSession->PlayerLoading())
    {
        Player* pBot = pSession->GetPlayer();
        if (!pBot || !pBot->IsPlayerBot() || !pBot->IsSettingFinish())
            return false;
        PlayerBotSession* pBotSession = dynamic_cast<PlayerBotSession*>(pSession);
        if (pBotSession)
        {
            if (pBotSession->HasSchedules())
                pBotSession->ClearAllSchedule();

            pSession->LogoutPlayer(false);
            return true;
        }
    }
    return false;
}

void PlayerBotMgr::SupplementPlayerBot()
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        if (pInfo->username.size() <= 0)
            continue;
        if (pInfo->characters.size() >= 18)
            continue;
        WorldSession* pSession = sWorld->FindSession(pInfo->id);
        if (!pSession)
        {
            TC_LOG_INFO("LOG_FILTER_PLAYER", "server.loading", ">> Create bot, but session offline.");
            continue;
        }
        std::string firstName = pInfo->username.substr(6);
        char botname[30] ={ 0 };
        for (int i = 1; i < 10; i++)
        {
            if (i == 6) continue;
            if (!pInfo->ExistClass(true, i))
            {
                memset(botname, 0, 30);
                sprintf(botname, "%sA%d", firstName.c_str(), i);
                pInfo->needCreateBots.push(BuildCreatePlayerData(true, i));
            }
            if (!pInfo->ExistClass(false, i))
            {
                memset(botname, 0, 30);
                sprintf(botname, "%sB%d", firstName.c_str(), i);
                pInfo->needCreateBots.push(BuildCreatePlayerData(false, i));
            }
        }
        if (!pInfo->ExistClass(true, 11))
        {
            memset(botname, 0, 30);
            sprintf(botname, "%sA11", firstName.c_str());
            pInfo->needCreateBots.push(BuildCreatePlayerData(true, 11));
        }
        if (!pInfo->ExistClass(false, 11))
        {
            memset(botname, 0, 30);
            sprintf(botname, "%sB11", firstName.c_str());
            pInfo->needCreateBots.push(BuildCreatePlayerData(false, 11));
        }
    }
    CreateOncePlayerBot();
}

void PlayerBotMgr::OnRealPlayerJoinBattlegroundQueue(uint32 bgTypeId, uint32 level)
{
}

//void PlayerBotMgr::OnRealPlayerLeaveBattlegroundQueue(uint32 bgTypeId, uint32 level)
//{
//    uint8 bgQueueTypeId = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(bgTypeId, 0);
//    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
//    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgTypeId));
//    if (!bg)
//        return;
//    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), level);
//    if (!bracketEntry)
//        return;
//
//    if (bgQueue.ExistRealPlayer(bracketEntry))
//        return;
//    const SessionMap& allSession = sWorld->GetAllSessions();
//    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
//    {
//        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
//        if (!pSession)
//            continue;
//        Player* player = pSession->GetPlayer();
//        if (!player || player->isBeingLoaded())
//            continue;
//        if (player->InBattleground() || player->InArena() || !player->IsInWorld())
//            continue;
//        uint32 lv = player->getLevel();
//        if (lv < bracketEntry->MinLevel || lv > bracketEntry->MaxLevel)
//            continue;
//        if (player->InBattlegroundQueue())
//        {
//            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutBGQueue, 0);
//            schedule1.parameter1 = bgTypeId;
//            pSession->PushScheduleToQueue(schedule1);
//        }
//    }
//}
//
//void PlayerBotMgr::OnRealPlayerLeaveArenaQueue(uint32 bgTypeId, uint32 level, uint32 aaType)
//{
//    uint8 bgQueueTypeId = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(bgTypeId, aaType);
//    BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeId);
//    Battleground* bg = sBattlegroundMgr->GetBattlegroundTemplate(BattlegroundTypeId(bgTypeId));
//    if (!bg)
//        return;
//    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), level);
//    if (!bracketEntry)
//        return;
//
//    if (bgQueue.ExistRealPlayer(bracketEntry))
//        return;
//    const SessionMap& allSession = sWorld->GetAllSessions();
//    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
//    {
//        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second.get());
//        if (!pSession)
//            continue;
//        Player* player = pSession->GetPlayer();
//        if (!player || player->isBeingLoaded())
//            continue;
//        if (player->InBattleground() || player->InArena() || !player->IsInWorld())
//            continue;
//        uint32 lv = player->getLevel();
//        if (lv < bracketEntry->MinLevel || lv > bracketEntry->MaxLevel)
//            continue;
//        if (player->InBattlegroundQueue())
//        {
//            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_OutAAQueue, 0);
//            schedule1.parameter1 = bgTypeId;
//            schedule1.parameter2 = bracketEntry->RangeIndex;
//            schedule1.parameter3 = aaType;
//            pSession->PushScheduleToQueue(schedule1);
//        }
//    }
//}

void PlayerBotMgr::OnRealPlayerEnterBattleground(uint32 bgTypeId, uint32 level)
{
}

//void PlayerBotMgr::OnRealPlayerLeaveBattleground(const Player* player)
//{
//    Battleground* bg = player->GetBattleground();
//    if (!bg || bg->GetStatus() != BattlegroundStatus::STATUS_WAIT_LEAVE)
//        return;
//    PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketByLevel(bg->GetMapId(), player->getLevel());
//    if (!bracketEntry)
//        return;
//
//    auto& allPlayerIDs = bg->GetPlayers();
//    for (auto itIDs = allPlayerIDs.begin(); itIDs != allPlayerIDs.end(); itIDs++)
//    {
//        Player* otherPlayer = ObjectAccessor::FindPlayer(itIDs->first);
//        if (!otherPlayer || otherPlayer->isBeingLoaded())
//            continue;
//        if (!otherPlayer->InBattleground())
//            continue;
//        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)otherPlayer->GetSession());
//        if (pSession)
//        {
//            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_LeaveBG, 0);
//            pSession->PushScheduleToQueue(schedule1);
//        }
//    }
//}

bool PlayerBotMgr::LoginBotByAccountIndex(uint32 account, uint32 index)
{
#ifndef INCOMPLETE_BOT
    PlayerBotBaseInfo* botInfo = GetPlayerBotAccountInfo(account);
    if (!botInfo)
        return false;
    if (index >= botInfo->characters.size())
        return false;
    for (auto itChar = botInfo->characters.begin(); itChar != botInfo->characters.end(); itChar++)
    {
        if (index > 0)
        {
            --index;
            continue;
        }
        PlayerBotCharBaseInfo& charInfo = itChar->second;
        WorldSession* pWorldSession = sWorld->FindSession(account);
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->GetPlayer())
            return false;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, charInfo.guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }
#endif
    return false;
}

bool PlayerBotMgr::AddNewPlayerBotByGUID(ObjectGuid& guid)
{
    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);

    if (isok==0 && !guid)
        return false;

    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot)
        return false;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
            //    return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
           // if (player->isBeingLoaded())
             //   return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }


    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first);
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
            return false;
        if (Player* player = pSession->GetPlayer())
        {
            if (!player->GetGroup() && !player->IsInCombat() && pSession->GetPlayer()->GetGUID() != guid)
            {
                BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                pSession->PushScheduleToQueue(schedule);
            }
            else
                return false;
        }
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idAccountBotBase.begin();
        itInfo != m_idAccountBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first);
        if (pWorldSession)
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
            if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
                return false;
            if (Player* player = pSession->GetPlayer())
            {
                if (!player->GetGroup() && !player->IsInCombat() && pSession->GetPlayer()->GetGUID() != guid)
                {
                    BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                    pSession->PushScheduleToQueue(schedule);
                }
                else
                    return false;
            }
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
        else
        {
            PlayerBotSession* pSession = UpPlayerBotSessionByBaseInfo(itInfo->second, true);
            if (!pSession)
                return false;
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
    }
    return false;
}

bool PlayerBotMgr::AddNewPlayerBotByGUID2(ObjectGuid& guid)
{
    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot) return false;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
        //    if (player->isBeingLoaded())
            //    return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
            //    return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
            //    return false;
            if (player->GetSession()->PlayerLoading())
                return false;

        }
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first);
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
            return false;
        if (Player* player = pSession->GetPlayer())
        {
            if (!player->GetGroup() && !player->IsInCombat() && pSession->GetPlayer()->GetGUID() != guid)
            {
                BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                pSession->PushScheduleToQueue(schedule);
            }
            else
                return false;
        }
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
        pSession->PushScheduleToQueue(schedule1);
        return true;
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idAccountBotBase.begin();
        itInfo != m_idAccountBotBase.end();
        itInfo++)
    {
        if (!(itInfo->second->ExistCharacterByGUID(guid)))
            continue;
        WorldSession* pWorldSession = sWorld->FindSession(itInfo->first);
        if (pWorldSession)
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
            if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules())
                return false;
            if (Player* player = pSession->GetPlayer())
            {
                if (!player->GetGroup() && !player->IsInCombat() && pSession->GetPlayer()->GetGUID() != guid)
                {
                    BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
                    pSession->PushScheduleToQueue(schedule);
                }
                else
                    return false;
            }
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
        else
        {
            PlayerBotSession* pSession = UpPlayerBotSessionByBaseInfo(itInfo->second, true);
            if (!pSession)
                return false;
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, guid);
            pSession->PushScheduleToQueue(schedule1);
            return true;
        }
    }
    return false;
}

void PlayerBotMgr::AddNewPlayerBot(bool faction, Classes prof, uint32 count)
{

    int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
    if (isok==0 && prof == CLASS_NONE)
        return;

    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot)
        return;

    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
         //   if (player->isBeingLoaded())
              //  return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }


    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
             //   return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

    if (count <= 0)
        return;

    if (prof == CLASS_NONE)
    {
        uint32 rndCls = 0;
        while (rndCls == 0 || rndCls == 6 || rndCls == 10 || rndCls > 11)
        {
            rndCls = urand(Classes::CLASS_WARRIOR, Classes::CLASS_DRUID);
        }
        prof = Classes(rndCls);
    }
#ifdef INCOMPLETE_BOT
    if (prof != 1 && prof != 5 && prof != 9)
    {
        if (prof == 2 || prof == 6)
            prof = Classes::CLASS_WARRIOR;
        else if (prof == 3 || prof == 4 || prof == 8 || prof == 10)
            prof = Classes::CLASS_WARLOCK;
        else if (prof == 7 || prof == 11)
            prof = Classes::CLASS_PRIEST;
    }
#endif
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (count <= 0)
            return;
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
            continue;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
        if (faction)
        {
            schedule1.parameter1 = 1;
        }
        else
        {
            schedule1.parameter1 = 2;
        }
        schedule1.parameter2 = prof;
        pSession->PushScheduleToQueue(schedule1);
        --count;
    }

    if (count > 0)
    {
        std::string allonlineText;
        consoleToUtf8(std::string("|cffff8800�u�D��C�C�O�sLNNl�C�?z��D�L��T���DD�D»�C�C�?L|r"), allonlineText);
        sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    }
}

void PlayerBotMgr::AddNewAccountBot(bool faction, Classes prof)
{
    int32 isok = sConfigMgr->GetIntDefault("pbotall", 1);
    if (isok==0)
        return;

    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot)
        return;


    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->GetSession()->PlayerLoading())
                return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            if (player->GetSession()->PlayerLoading())
                return;
        }
    }
    std::string allonlineText;
#ifdef INCOMPLETE_BOT
    consoleToUtf8(std::string("|cffff8800ElN�c�T��OU�?�DD���?�O�sL?�ɫ|r"), allonlineText);
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    return;
#endif
    if (prof == CLASS_NONE)
    {
        uint32 rndCls = 0;
        while (rndCls == 0 || rndCls == 6 || rndCls == 10 || rndCls > 11)
        {
            rndCls = urand(Classes::CLASS_WARRIOR, Classes::CLASS_DRUID);
        }
        prof = Classes(rndCls);
    }
    std::set<uint32> hasSessionIDs;
	
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || !pSession->IsAccountBotSession())
            continue;
        hasSessionIDs.insert(pSession->GetAccountId());
        Player* player = pSession->GetPlayer();
        if (player)
            continue;
        PlayerBotBaseInfo* pBaseInfo = GetAccountBotAccountInfo(pSession->GetAccountId());
        if (!pBaseInfo)
            continue;
        if (!pBaseInfo->ExistClass(faction, prof))
            continue;
        BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
        if (faction)
            schedule1.parameter1 = 1;
        else
            schedule1.parameter1 = 2;
        schedule1.parameter2 = prof;
        pSession->PushScheduleToQueue(schedule1);
        return;
    }

    for (std::map<uint32, PlayerBotBaseInfo*>::iterator it = m_idAccountBotBase.begin();
        it != m_idAccountBotBase.end();
        it++)
    {
        PlayerBotBaseInfo* pInfo = it->second;
        WorldSession* pSession = sWorld->FindSession(pInfo->id);
        if (pSession)
            continue;
        if (!pInfo->ExistClass(faction, prof))
            continue;
        PlayerBotSession* pBotSession = UpPlayerBotSessionByBaseInfo(pInfo, true);
        if (pBotSession)
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            if (faction)
                schedule1.parameter1 = 1;
            else
                schedule1.parameter1 = 2;
            schedule1.parameter2 = prof;
            pBotSession->PushScheduleToQueue(schedule1);
            return;
        }
    }

    consoleToUtf8(std::string("|cffff8800A��DON�?s��aDrͬO��S��ָ��ְN�����?�O�sL?�ɫ|r"), allonlineText);
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
}

void PlayerBotMgr::AddNewPlayerBotByClass(uint32 count, Classes prof)
{
    int32 isok = sConfigMgr->GetIntDefault("pbotall", 1);
    if (isok==0)
        return;

    int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
    int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);

    if ((allianceCount+hordeCount) >= m_MaxOnlineBot) return;

    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
             //   return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }


    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
            //if (player->isBeingLoaded())
             //   return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

#ifdef INCOMPLETE_BOT
    if (prof != 1 && prof != 5 && prof != 9)
    {
        if (prof == 2 || prof == 6)
            prof = Classes::CLASS_WARRIOR;
        else if (prof == 3 || prof == 4 || prof == 8 || prof == 10)
            prof = Classes::CLASS_WARLOCK;
        else if (prof == 7 || prof == 11)
            prof = Classes::CLASS_PRIEST;
    }
#endif

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (allianceCount <= 0 && hordeCount <= 0)
            return;
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player && player->GetClass() != prof)
            continue;
        if (player)
        {
            if (player->GetTeamId() == TeamId::TEAM_ALLIANCE)
                --allianceCount;
            else
                --hordeCount;
        }
        else
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            if (allianceCount > 0)
            {
                schedule1.parameter1 = 1;
                --allianceCount;
            }
            else if (hordeCount > 0)
            {
                schedule1.parameter1 = 2;
                --hordeCount;
            }
            else
                continue;
            schedule1.parameter2 = prof;
            pSession->PushScheduleToQueue(schedule1);
        }
    }

    if (allianceCount > 0 || hordeCount > 0)
    {
        std::string allonlineText;
        consoleToUtf8(std::string("|cffff8800�u�D��C�C�O�sLNNl�C�?z��D�L��T���DD�D»�C�C�?L|r"), allonlineText);
        sWorld->SendGlobalText(allonlineText.c_str(), NULL);
    }
}

bool PlayerBotMgr::ChangePlayerBotSetting(uint32 account, uint32 minLV, uint32 maxLV, uint32 talent)
{
    if (account == 0)
        return false;
    maxLV = PlayerBotSetting::CheckMaxLevel(maxLV);
    if (maxLV < minLV)
        maxLV = minLV;
    WorldSession* pWorldSession = sWorld->FindSession(account);
    if (pWorldSession && pWorldSession->IsBotSession() && !pWorldSession->PlayerLoading())
    {
        Player* player = pWorldSession->GetPlayer();
        if (!player || player->InBattlegroundQueue() || player->InBattleground() ||
            player->GetBattleground() || player->IsInCombat())
            return false;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->HasSchedules())
            return false;
        BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
        schedule2.parameter1 = minLV;
        schedule2.parameter2 = maxLV;
        schedule2.parameter3 = talent + 1;
        pSession->PushScheduleToQueue(schedule2);
        return true;
    }
    return false;
}

void PlayerBotMgr::AddNewPlayerBotToBG(TeamId team, uint32 minLV, uint32 maxLV, BattlegroundTypeId bgTypeID)
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
         //   if (player->isBeingLoaded())
              //  return;
            if (player->GetSession()->PlayerLoading())
                return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
          //  if (player->isBeingLoaded())
            //    return;
            if (player->GetSession()->PlayerLoading())
                return;

        }
    }

    maxLV = PlayerBotSetting::CheckMaxLevel(maxLV);
    if (maxLV < minLV)
        maxLV = minLV;
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
        {
            if (!player /*|| player->isBeingLoaded()*/ || !player->IsInWorld())
                continue;
            if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() || player->isUsingLfg())
                continue;
            if (player->InBattlegroundQueue())
                continue;
            if (!player->IsSettingFinish())
                continue;
            if (player->GetTeamId() != team)
                continue;

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = maxLV;
            schedule2.parameter2 = maxLV;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InBGQueue, 0);
            schedule3.parameter1 = bgTypeID;
            pSession->PushScheduleToQueue(schedule3);
            return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player)
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            schedule1.parameter1 = (team == TeamId::TEAM_ALLIANCE) ? 1 : 2;
            pSession->PushScheduleToQueue(schedule1);

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = minLV;
            schedule2.parameter2 = maxLV;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InBGQueue, 0);
            schedule3.parameter1 = bgTypeID;
            pSession->PushScheduleToQueue(schedule3);
            return;
        }
    }

    std::string allonlineText;
    consoleToUtf8(std::string("|cffff8800�u�D��C�C�O�sLNNl�C�?z��D�L��T��L�C�D»�C�C˵?O?l?�D?L|r"), allonlineText);
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
}

////void PlayerBotMgr::AddNewPlayerBotToLFG(lfg::LFGBotRequirement* botRequirement)
////{
////	if (!botRequirement || botRequirement->selectedDungeons.empty())
////		return;
////		
////	int32 isok = sConfigMgr->GetIntDefault("pbotall", 1);
////	if (isok==0)
////		return;
////
////int32 isoka = sConfigMgr->GetIntDefault("pbotasl", 88);
////
////	int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
////	int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);
////
////if ((allianceCount+hordeCount) >isoka) return;
////
////
////	const SessionMap& allSession = sWorld->GetAllSessions();
////	for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
////	{
////
////		Player* player = itSession->second->GetPlayer();
////		if (player)
////{
////		if (player->isBeingLoaded())
////			return;
////if (player->GetSession()->PlayerLoading())
////return;
//////if (player->isInCombat())
//////return;
////	}
////	}
////	
////
////	//const SessionMap& allSession = sWorld->GetAllSessions();
////	for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
////	{
////
////		Player* player = itSession->second->GetPlayer();
////		if (player)
////{
////		if (player->isBeingLoaded())
////			return;
////if (player->GetSession()->PlayerLoading())
////return;
//////if (player->isInCombat())
//////return;
////	}
////	}
////		
////	std::vector<uint32> duns;
////	for (lfg::LfgDungeonSet::iterator itLfgSet = botRequirement->selectedDungeons.begin(); itLfgSet != botRequirement->selectedDungeons.end(); itLfgSet++)
////	{
////		uint32 dun = *itLfgSet;
////		duns.push_back(dun);
////	}
//////	const SessionMap& allSession = sWorld->GetAllSessions();
////	for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
////	{
////		if (!itSession->second->IsBotSession())
////			continue;
////		PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
////		if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
////			continue;
////		Player* player = pSession->GetPlayer();
////		if (!player)
////		{
////			BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
////			if (!FillOnlineBotScheduleByLFGRequirement(botRequirement, &schedule1))
////				continue;
////			pSession->PushScheduleToQueue(schedule1);
////
////			BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
////			schedule2.parameter1 = botRequirement->needLevel;
////			schedule2.parameter2 = botRequirement->needLevel;
////			schedule2.parameter3 = GetScheduleTalentByLFGRequirement(botRequirement->needRole, schedule1.parameter2) + 1;
////			pSession->PushScheduleToQueue(schedule2);
////
////			BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InLFGQueue, 0);
////			schedule3.parameter1 = uint32(botRequirement->needRole);
////			uint32 count = botRequirement->selectedDungeons.size();
////			schedule3.parameter2 = (count > 3) ? 3 : count;
////			if (count >= 1)
////				schedule3.parameter3 = duns[0];
////			if (count >= 2)
////				schedule3.parameter4 = duns[1];
////			if (count >= 3)
////				schedule3.parameter5 = duns[2];
////			pSession->PushScheduleToQueue(schedule3);
////
////			return;
////		}
////		else if (IsIDLEPlayerBot(player))
////		{
////			if (player->GetTeamId() != botRequirement->needTeam)
////				continue;
////			lfg::LfgRoles playerRole = lfg::LfgRoles::PLAYER_ROLE_NONE;
////			if (BotFieldAI* pFieldAI = dynamic_cast<BotFieldAI*>(player->GetAI()))
////			{
////				if ((player->getClass() == 1 && player->FindTalentType() == 2) || (player->getClass() == 2 && player->FindTalentType() == 1))
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_TANK;
////				else if (pFieldAI->IsHealerBotAI())
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_HEALER;
////				else
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
////			}
////			else if (BotGroupAI* pGroupAI = dynamic_cast<BotGroupAI*>(player->GetAI()))
////			{
////				if (pGroupAI->IsTankBotAI())
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_TANK;
////				else if (pGroupAI->IsHealerBotAI())
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_HEALER;
////				else
////					playerRole = lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
////			}
////			if (playerRole != botRequirement->needRole)
////				continue;
////
////			BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
////			schedule2.parameter1 = botRequirement->needLevel;
////			schedule2.parameter2 = botRequirement->needLevel;
////			schedule2.parameter3 = player->FindTalentType() + 1;
////			pSession->PushScheduleToQueue(schedule2);
////
////			BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InLFGQueue, 0);
////			schedule3.parameter1 = uint32(botRequirement->needRole);
////			uint32 count = botRequirement->selectedDungeons.size();
////			schedule3.parameter2 = (count > 3) ? 3 : count;
////			if (count >= 1)
////				schedule3.parameter3 = duns[0];
////			if (count >= 2)
////				schedule3.parameter4 = duns[1];
////			if (count >= 3)
////				schedule3.parameter5 = duns[2];
////			pSession->PushScheduleToQueue(schedule3);
////
////			return;
////		}
////	}
////
////	std::string allonlineText;
////	consoleToUtf8(std::string("|cffff8800�u�D��C�C�O�sLNNl�C�?z��D�L��T��L�C�D»�C�C˵?�RD�lǶ��D�D?L|r"), allonlineText);
////	sWorld->SendGlobalText(allonlineText.c_str(), NULL);
////}

void PlayerBotMgr::AddNewPlayerBotToAA(TeamId team, BattlegroundTypeId bgTypeID, uint32 bracketID, uint32 aaType)
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        Player* player = itSession->second->GetPlayer();
        if (player)
        {
           // if (player->isBeingLoaded())
              //  return;
            if (player->GetSession()->PlayerLoading())
                return;
            if (player->IsInCombat())
            return;
        }
    }

    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {

        Player* player = itSession->second->GetPlayer();
        if (player)
        {
         //   if (player->isBeingLoaded())
            //    return;
            if (player->GetSession()->PlayerLoading())
                return;
            if (player->IsInCombat())
            return;
        }
    }


    if (aaType != 2 && aaType != 3 && aaType != 5)
        return;
    std::vector<uint32> rndIDs;
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
        rndIDs.push_back(itSession->first);
    Trinity::Containers::RandomShuffle(rndIDs);
    for (uint32 account : rndIDs)
    {
        WorldSession* pWorldSession = sWorld->FindSession(account);
        if (!pWorldSession || !pWorldSession->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (player)
        {
            if (!player /*|| player->isBeingLoaded()*/ || !player->IsInWorld())
                continue;
            if (player->InBattleground() || player->InArena() || player->GetMap()->IsDungeon() || player->isUsingLfg())
                continue;
            if (player->InBattlegroundQueue())
                continue;
            if (!player->IsSettingFinish())
                continue;
            if (player->GetTeamId() != team)
                continue;

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = 110;
            schedule2.parameter2 = 110;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
            schedule3.parameter1 = 14;
            schedule3.parameter2 = 4;
            pSession->PushScheduleToQueue(schedule3);

            return;
        }
    }

    for (uint32 account : rndIDs)
    {
        WorldSession* pWorldSession = sWorld->FindSession(account);
        if (!pWorldSession || !pWorldSession->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(pWorldSession);
        if (!pSession || pSession->PlayerLoading() || pSession->HasSchedules() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player)
        {
            BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online, 0);
            schedule1.parameter1 = (team == TeamId::TEAM_ALLIANCE) ? 1 : 2;
            pSession->PushScheduleToQueue(schedule1);

            BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
            schedule2.parameter1 = 110;
            schedule2.parameter2 = 110;
            schedule2.parameter3 = 4;
            pSession->PushScheduleToQueue(schedule2);

            BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
            schedule3.parameter1 = 14;
            schedule3.parameter2 = 4;
            pSession->PushScheduleToQueue(schedule3);

            return;
        }
    }

    std::string allonlineText;
    consoleToUtf8(std::string("|cffff8800�u�D��C�C�O�sLNNl�C�?z��D�L��T��L�C�D»�C�C˵?lsLLl?�D?L|r"), allonlineText);
    sWorld->SendGlobalText(allonlineText.c_str(), NULL);
}

//void PlayerBotMgr::AddTeamBotToRatedArena(uint32 arenaTeamId)
//{
//    //	int32 isok = sConfigMgr->GetIntDefault("pbot", 1);
//    //	if (isok==0)
//    //		return;
//    //
//    //int32 isoka = sConfigMgr->GetIntDefault("pbotasl", 88);
//    //
//    //	int32 allianceCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_ALLIANCE, true);
//    //	int32 hordeCount = (int32)sPlayerBotMgr->GetOnlineBotCount(TEAM_HORDE, true);
//    //
//    //if ((allianceCount+hordeCount) >isoka) return;
//    //
//    //
//    //	const SessionMap& allSession = sWorld->GetAllSessions();
//    //	for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
//    //	{
//    //
//    //		Player* player = itSession->second->GetPlayer();
//    //		if (player)
//    //{
//    //		if (player->isBeingLoaded())
//    //			return;
//    //if (player->GetSession()->PlayerLoading())
//    //return;
//    //
//    ////if (player->isInCombat())
//    ////return;
//    //	}
//    //	}
//    //	
//    //
//    //	if (arenaTeamId == 0)
//    //		return;
//    //	ArenaTeam* arenaTeam = sArenaTeamMgr->GetArenaTeamById(arenaTeamId);
//    //	if (!arenaTeam)
//    //		return;
//    //	BattlegroundQueueTypeId bgQueueTypeID = BattlegroundMgr::BGQueueTypeId(BattlegroundTypeId(BattlegroundTypeId::BATTLEGROUND_AA), arenaTeam->GetType());
//    //	BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
//    //	for (ArenaTeam::MemberList::iterator itaMem = arenaTeam->m_membersBegin(); itaMem != arenaTeam->m_membersEnd(); itaMem++)
//    //	{
//    //		ArenaTeamMember& mem = *itaMem;
//    //		ObjectGuid guid = mem.Guid;
//    //		if (bgQueue.ExistQueueByRatedArena(guid, true))
//    //			continue;
//    //		if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
//    //		{
//    //			if (!player->IsPlayerBot() || player->GetGroup())
//    //				continue;
//    //			if (PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>(player->GetSession()))
//    //			{
//    //				BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
//    //				schedule2.parameter1 = 80;
//    //				schedule2.parameter2 = 80;
//    //				schedule2.parameter3 = sArenaTeamMgr->FindArenaTeamPlayerBotTalent(guid) + 1;
//    //				schedule2.parameter4 = (pSession->IsAccountBotSession()) ? 0 : 1;
//    //				pSession->PushScheduleToQueue(schedule2);
//    //
//    //				BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
//    //				schedule3.parameter1 = BattlegroundTypeId::BATTLEGROUND_AA;
//    //				if (arenaTeam->GetType() == 2)
//    //					schedule3.parameter2 = 0;
//    //				else if (arenaTeam->GetType() == 3)
//    //					schedule3.parameter2 = 1;
//    //				else if (arenaTeam->GetType() == 5)
//    //					schedule3.parameter2 = 2;
//    //				schedule3.parameter3 = 1;
//    //				pSession->PushScheduleToQueue(schedule3);
//    //
//    //				BotGlobleSchedule schedule4(BotGlobleScheduleType::BGSType_EnterAA, 0);
//    //				schedule4.parameter1 = BattlegroundTypeId::BATTLEGROUND_AA;
//    //				schedule4.parameter2 = 14;
//    //				schedule4.parameter3 = arenaTeam->GetType();
//    //				schedule4.parameter4 = 1;
//    //				pSession->PushScheduleToQueue(schedule4);
//    //			}
//    //		}
//    //		else if (PlayerBotSession* pSession = GetBotSessionByCharGUID(guid))
//    //		{
//    //			Player* player = pSession->GetPlayer();
//    //			if (player)
//    //			{
//    //				BotGlobleSchedule schedule(BotGlobleScheduleType::BGSType_Offline, 0);
//    //				pSession->PushScheduleToQueue(schedule);
//    //			}
//    //			BotGlobleSchedule schedule1(BotGlobleScheduleType::BGSType_Online_GUID, 0);
//    //			schedule1.parameter1 = guid.GetCounter();
//    //			pSession->PushScheduleToQueue(schedule1);
//    //
//    //			BotGlobleSchedule schedule2(BotGlobleScheduleType::BGSType_Settting, 0);
//    //			schedule2.parameter1 = 80;
//    //			schedule2.parameter2 = 80;
//    //			schedule2.parameter3 = sArenaTeamMgr->FindArenaTeamPlayerBotTalent(guid) + 1;
//    //			schedule2.parameter4 = (pSession->IsAccountBotSession()) ? 0 : 1;
//    //			pSession->PushScheduleToQueue(schedule2);
//    //
//    //			BotGlobleSchedule schedule3(BotGlobleScheduleType::BGSType_InAAQueue, 0);
//    //			schedule3.parameter1 = BattlegroundTypeId::BATTLEGROUND_AA;
//    //			if (arenaTeam->GetType() == 2)
//    //				schedule3.parameter2 = 0;
//    //			else if (arenaTeam->GetType() == 3)
//    //				schedule3.parameter2 = 1;
//    //			else if (arenaTeam->GetType() == 5)
//    //				schedule3.parameter2 = 2;
//    //			schedule3.parameter3 = 1;
//    //			pSession->PushScheduleToQueue(schedule3);
//    //
//    //			BotGlobleSchedule schedule4(BotGlobleScheduleType::BGSType_EnterAA, 0);
//    //			schedule4.parameter1 = BattlegroundTypeId::BATTLEGROUND_AA;
//    //			schedule4.parameter2 = 14;
//    //			schedule4.parameter3 = arenaTeam->GetType();
//    //			schedule4.parameter4 = 1;
//    //			pSession->PushScheduleToQueue(schedule4);
//    //		}
//    //	}
//}
//
////bool PlayerBotMgr::FillOnlineBotScheduleByLFGRequirement(lfg::LFGBotRequirement* botRequirement, BotGlobleSchedule* botSchedule)
////{
////	if (!botRequirement || !botSchedule)
////		return false;
////	botSchedule->parameter1 = 0;
////	if (botRequirement->needTeam == TEAM_ALLIANCE)
////		botSchedule->parameter1 = 1;
////	else if (botRequirement->needTeam == TEAM_HORDE)
////		botSchedule->parameter1 = 2;
////	else
////		return false;
////	std::vector<uint32> matchClasses;
////	botSchedule->parameter2 = 0;
////	if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_TANK)
////	{
////		matchClasses.push_back(1);
////#ifndef INCOMPLETE_BOT
////		matchClasses.push_back(2);
////#endif
////		//matchClasses.push_back(6);
////	}
////	else if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_HEALER)
////	{
////		matchClasses.push_back(5);
////#ifndef INCOMPLETE_BOT
////		matchClasses.push_back(2);
////		matchClasses.push_back(7);
////		matchClasses.push_back(11);
////#endif
////	}
////	else if (botRequirement->needRole == lfg::LfgRoles::PLAYER_ROLE_DAMAGE)
////	{
////#ifndef INCOMPLETE_BOT
////		matchClasses.push_back(3);
////		matchClasses.push_back(4);
////		matchClasses.push_back(8);
////#endif
////		matchClasses.push_back(9);
////	}
////	if (matchClasses.empty())
////		return false;
////	botSchedule->parameter2 = matchClasses[urand(0, matchClasses.size() - 1)];
////	return true;
////}
//
//uint32 PlayerBotMgr::GetScheduleTalentByLFGRequirement(lfg::LfgRoles roles, uint32 botCls)
//{
//    uint32 talentType = 3;
//    switch (botCls)
//    {
//        case 1:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
//                talentType = 2;
//            else
//                talentType = urand(0, 1);
//            break;
//        case 2:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
//                talentType = 1;
//            else if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
//                talentType = 0;
//            else
//                talentType = 2;
//            break;
//        case 6:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_TANK)
//                talentType = 1;
//            else
//            {
//                if (urand(0, 1) == 0)
//                    talentType = 0;
//                else
//                    talentType = 2;
//            }
//            break;
//        case 5:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_DAMAGE)
//                talentType = 2;
//            else
//                talentType = urand(0, 1);
//            break;
//        case 7:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
//                talentType = 2;
//            else
//                talentType = urand(0, 1);
//            break;
//        case 11:
//            if (roles == lfg::LfgRoles::PLAYER_ROLE_HEALER)
//                talentType = 2;
//            else
//                talentType = urand(0, 1);
//            break;
//    }
//    return talentType;
//}
//
//lfg::LfgRoles PlayerBotMgr::GetPlayerBotCurrentLFGRoles(Player* player)
//{
//    if (!player)
//        return lfg::LfgRoles::PLAYER_ROLE_NONE;
//    uint32 talentType = player->FindTalentType();
//    switch (player->getClass())
//    {
//        case 1:
//            if (talentType == 2)
//                return lfg::LfgRoles::PLAYER_ROLE_TANK;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        case 2:
//            if (talentType == 0)
//                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
//            else if (talentType == 1)
//                return lfg::LfgRoles::PLAYER_ROLE_TANK;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        case 6:
//            if (talentType == 1)
//                return lfg::LfgRoles::PLAYER_ROLE_TANK;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        case 3:
//        case 4:
//        case 8:
//        case 9:
//            return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        case 5:
//            if (talentType == 2)
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
//            break;
//        case 7:
//            if (talentType == 2)
//                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        case 11:
//            if (talentType == 2)
//                return lfg::LfgRoles::PLAYER_ROLE_HEALER;
//            else
//                return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//            break;
//        default:
//            return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//    }
//    return lfg::LfgRoles::PLAYER_ROLE_DAMAGE;
//}
//
//ObjectGuid PlayerBotMgr::GetNoArenaMatchCharacter(TeamId team, uint32 arenaType, Classes cls, std::vector<ObjectGuid>& fliters)
//{
//    if (team != TEAM_ALLIANCE && team != TEAM_HORDE)
//        return ObjectGuid::Empty;
//    if (arenaType != 2 && arenaType != 3 && arenaType != 5)
//        return ObjectGuid::Empty;
//    if (m_idPlayerBotBase.empty())
//        return ObjectGuid::Empty;
//    std::vector<PlayerBotBaseInfo*> allBotBaseInfos;
//    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
//        itInfo != m_idPlayerBotBase.end();
//        itInfo++)
//        allBotBaseInfos.push_back(itInfo->second);
//    for (uint32 i = allBotBaseInfos.size() - 1; i >= 0; --i)
//    {
//        PlayerBotBaseInfo* pInfo = allBotBaseInfos[i];
//        uint32 id = pInfo->GetCharIDByNoArenaType((team == TEAM_ALLIANCE) ? true : false, cls, arenaType, fliters);
//        if (id == 0)
//            continue;
//        bool exist = false;
//        for (ObjectGuid& guid : fliters)
//        {
//            if (guid.GetCounter() == id)
//            {
//                exist = true;
//                break;
//            }
//        }
//        if (!exist)
//            return ObjectGuid::Create<HighGuid::Player>(id);
//    }
//
//    //for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
//    //	itInfo != m_idPlayerBotBase.end();
//    //	itInfo++)
//    //{
//    //	PlayerBotBaseInfo* pInfo = itInfo->second;
//    //	uint32 id = pInfo->GetCharIDByNoArenaType((team == TEAM_ALLIANCE) ? true : false, cls, arenaType, fliters);
//    //	if (id == 0)
//    //		continue;
//    //	bool exist = false;
//    //	for (ObjectGuid& guid : fliters)
//    //	{
//    //		if (guid.GetCounter() == id)
//    //		{
//    //			exist = true;
//    //			break;
//    //		}
//    //	}
//    //	if (!exist)
//    //		return ObjectGuid(uint64(id));
//    //}
//    return ObjectGuid::Empty;
//}
//
std::string PlayerBotMgr::GetNameANDClassesText(ObjectGuid& guid)
{
    for (std::map<uint32, PlayerBotBaseInfo*>::iterator itInfo = m_idPlayerBotBase.begin();
        itInfo != m_idPlayerBotBase.end();
        itInfo++)
    {
        PlayerBotBaseInfo* pInfo = itInfo->second;
        std::string text = pInfo->GetCharNameANDClassesText(guid);
        if (!text.empty())
            return text;
    }
    return "";
}

bool PlayerBotMgr::CanReadyArenaByArenaTeamID(uint32 arenaTeamId)
{
    return true;
}

void PlayerBotMgr::QueryBattlegroundRequirement()
{
    for (BattleGroundTypes::iterator itType = m_BGTypes.begin(); itType != m_BGTypes.end(); itType++)
    {
        BattlegroundTypeId bgTypeID = *itType;
        Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeID);
        if (!bg_template)
            continue;
      /*  auto& bgFreeSlotQueues = sBattlegroundMgr->BGFreeSlotQueue;
        for (auto itr = bgFreeSlotQueues.begin(); itr != bgFreeSlotQueues.end(); itr++)
        {
            Battleground* bg = *itr;
            if (!bg->ExistRealPlayer())
                continue;
            uint32 minLV = bg->GetMinLevel();
            uint32 maxLV = bg->GetMaxLevel();
            if (!bg->IsRated() && bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
            {
                if (bg->GetFreeSlotsForTeam(Team::ALLIANCE) > 0 && bg->GetFreeSlotsForTeam(Team::ALLIANCE) >= bg->GetFreeSlotsForTeam(Team::HORDE))
                    AddNewPlayerBotToBG(TeamId::TEAM_ALLIANCE, minLV, maxLV, bgTypeID);
                else if (bg->GetFreeSlotsForTeam(Team::HORDE) > 0)
                    AddNewPlayerBotToBG(TeamId::TEAM_HORDE, minLV, maxLV, bgTypeID);
                else
                    continue;
                return;
            }
        }*/

        //uint8 bgQueueTypeID = GetBgQueueTypeIdByBgTypeID(bgTypeID, 0);
       // BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
     /*   for (uint8 j = 0; j < MS::Battlegrounds::MaxBrackets; j++)
        {
            uint8 bracket = j;
            PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketById(bg_template->GetMapId(), bracket);
            if (!bracketEntry)
                continue;
            if (!bgQueue.ExistRealPlayer(bracketEntry))
                continue;
            int32 needAlliancePlayerCount = 0;
            int32 needHordePlayerCount = 0;
            if (bgQueue.QueryNeedPlayerCount(bgTypeID, bracket, 0, needAlliancePlayerCount, needHordePlayerCount))
            {
                if (needAlliancePlayerCount > 0 && needAlliancePlayerCount >= needHordePlayerCount)
                    AddNewPlayerBotToBG(TeamId::TEAM_ALLIANCE, bracketEntry->MinLevel, bracketEntry->MaxLevel, bgTypeID);
                else if (needHordePlayerCount > 0)
                    AddNewPlayerBotToBG(TeamId::TEAM_HORDE, bracketEntry->MinLevel, bracketEntry->MaxLevel, bgTypeID);
                else
                    continue;
                return;
            }
        }*/
    }
}

//void PlayerBotMgr::QueryRatedArenaRequirement()
//{
//    if (m_ArenaSearchTick > 0)
//    {
//        --m_ArenaSearchTick;
//        return;
//    }
//    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);
//    if (!bg_template)
//        return;
//    PVPDifficultyEntry const* bracketEntry = FindBGBracketEntry(bg_template, 80);
//    if (!bracketEntry)
//        return;
//
//    uint32 arenaType[3] ={ 2, 3, 5 };
//    for (uint32 i = 0; i < 3; i++)
//    {
//        uint8 bgQueueTypeID = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, arenaType[i]);
//        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
//        if (!bgQueue.ExistRealPlayer(bracketEntry, true))
//        {
//            bgQueue.AllPlayerBotLeaveQueueFromRatedArena(bracketEntry->RangeIndex);
//            continue;
//        }
//        uint32 allianceGroupID = 0;
//        uint32 hordeGroupID = 0;
//        GroupQueueInfo* allianceGroupInfo = bgQueue.GetFirstRealPlayerGroupInfo(bracketEntry->RangeIndex, MS::Battlegrounds::QueueGroupTypes::PremadeAlliance);
//        GroupQueueInfo* hordeGroupInfo = bgQueue.GetFirstRealPlayerGroupInfo(bracketEntry->RangeIndex, MS::Battlegrounds::QueueGroupTypes::PremadeHorde);
//        if (!allianceGroupInfo && !hordeGroupInfo)
//            continue;
//        if (allianceGroupInfo && hordeGroupInfo)
//        {
//            allianceGroupID = allianceGroupInfo->GroupId;
//            hordeGroupID = hordeGroupInfo->GroupId;
//        }
//        /*else if (allianceGroupInfo && !BotUtility::DownBotArenaTeam)
//        {
//            allianceGroupID = allianceGroupInfo->GroupId;
//            hordeGroupID = hordeGroupInfo->GroupId;
//            if (hordeGroupID == 0)
//                continue;
//            AddTeamBotToRatedArena(hordeGroupID);
//        }
//        else if (hordeGroupInfo && !BotUtility::DownBotArenaTeam)
//        {
//            hordeGroupID = hordeGroupInfo->GroupId;
//            allianceGroupID = sArenaTeamMgr->SearchEnemyArenaTeam(hordeGroupID, ALLIANCE);
//            if (allianceGroupID == 0)
//                continue;
//            AddTeamBotToRatedArena(allianceGroupID);
//        }*/
//        else
//            continue;
//        m_ArenaSearchTick = 14;
//        break;
//    }
//}
//
//void PlayerBotMgr::QueryNonRatedArenaRequirement()
//{
//    Battleground* bg_template = sBattlegroundMgr->GetBattlegroundTemplate(MS::Battlegrounds::BattlegroundTypeId::ArenaAll);
//    if (!bg_template)
//        return;
//    PVPDifficultyEntry const* bracketEntry = FindBGBracketEntry(bg_template, 110);
//    if (!bracketEntry)
//        return;
//    auto& bgFreeSlotQueues = sBattlegroundMgr->BGFreeSlotQueue;
//    for (auto itr = bgFreeSlotQueues.begin(); itr != bgFreeSlotQueues.end(); itr++)
//    {
//        Battleground* bg = *itr;
//        if (!bg->ExistRealPlayer())
//            continue;
//        if (bg->GetStatus() > STATUS_WAIT_QUEUE && bg->GetStatus() < STATUS_WAIT_LEAVE)
//        {
//            if (bg->GetFreeSlotsForTeam(Team::ALLIANCE) > 0)
//                AddNewPlayerBotToAA(TeamId::TEAM_ALLIANCE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, bg->GetMaxPlayers() / 2);
//            else if (bg->GetFreeSlotsForTeam(Team::HORDE) > 0)
//                AddNewPlayerBotToAA(TeamId::TEAM_HORDE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, bg->GetMaxPlayers() / 2);
//            else
//                continue;
//            return;
//        }
//    }
//
//    uint32 arenaType[3] ={ 2, 3, 5 };
//    for (uint32 i = 0; i < 3; i++)
//    {
//        uint8 bgQueueTypeID = MS::Battlegrounds::GetBgQueueTypeIdByBgTypeID(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, arenaType[i]);
//        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(bgQueueTypeID);
//        if (!bgQueue.ExistRealPlayer(bracketEntry, false))
//            continue;
//        int32 needAlliancePlayerCount = 0;
//        int32 needHordePlayerCount = 0;
//        if (bgQueue.QueryNeedPlayerCount(MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i], needAlliancePlayerCount, needHordePlayerCount))
//        {
//            if (needAlliancePlayerCount > 0 && needAlliancePlayerCount >= needHordePlayerCount)
//                AddNewPlayerBotToAA(TeamId::TEAM_ALLIANCE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i]);
//            else if (needHordePlayerCount > 0)
//                AddNewPlayerBotToAA(TeamId::TEAM_HORDE, MS::Battlegrounds::BattlegroundTypeId::ArenaAll, bracketEntry->RangeIndex, arenaType[i]);
//            else
//                continue;
//            return;
//        }
//    }
//}

void PlayerBotMgr::OnlinePlayerBotByGUIDQueue()
{
    while (!m_DelayOnlineBots.empty())
    {
        ObjectGuid& guid = m_DelayOnlineBots.front();
        m_DelayOnlineBots.pop();
        if (AddNewPlayerBotByGUID(guid))
            return;
    }
}

bool PlayerBotMgr::ExistUnBGPlayerBot()
{
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (!itSession->second->IsBotSession())
            continue;
        PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
        if (!pSession)
            continue;
        if (pSession->PlayerLoading() || pSession->IsAccountBotSession())
            continue;
        Player* player = pSession->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;
        if (player->InBattleground() || player->InBattlegroundQueue() || player->isUsingLfg() || player->GetGroup())
            continue;
        if (player->HasAura(26013))
            continue;
        if (!pSession->HasSchedules())
            continue;
        return true;
    }
    return false;
}
//
//PVPDifficultyEntry const* PlayerBotMgr::FindBGBracketEntry(Battleground* bg_template, uint32 level)
//{
//    if (!bg_template)
//        return NULL;
//    for (uint32 j = 0; j < MS::Battlegrounds::MaxBrackets; j++)
//    {
//        uint8 bracket = j;
//        PVPDifficultyEntry const* bracketEntry = sDB2Manager.GetBattlegroundBracketById(bg_template->GetMapId(), bracket);
//        if (!bracketEntry)
//            continue;
//        if (bracketEntry->MinLevel <= level && bracketEntry->MaxLevel >= level)
//            return bracketEntry;
//    }
//    return NULL;
//}

uint32 PlayerBotMgr::GetOnlineBotCount(TeamId team, bool hasReal)
{
    uint32 onlineCount = 0;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (itSession->second->IsBotSession())
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
            if (!pSession)
                continue;
            if (pSession->PlayerLoading())
            {
                ++onlineCount;
                continue;
            }
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;
            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
        else if (hasReal)
        {
            WorldSession* pSession = itSession->second;
            if (pSession->PlayerLoading())
                continue;
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;
            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
    }
    return onlineCount;
}

uint32 PlayerBotMgr::GetOnlineBotCount2(TeamId team, bool hasReal)
{
    uint32 onlineCount = 0;
    const SessionMap& allSession = sWorld->GetAllSessions();
    for (SessionMap::const_iterator itSession = allSession.begin(); itSession != allSession.end(); itSession++)
    {
        if (itSession->second->IsBotSession())
        {
            PlayerBotSession* pSession = dynamic_cast<PlayerBotSession*>((WorldSession*)itSession->second);
            if (!pSession)
                continue;
            if (pSession->PlayerLoading())
            {
                ++onlineCount;
                continue;
            }
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            if (!player->InBattleground() &&  !player->InBattlegroundQueue())
                continue;

            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
        else if (hasReal)
        {
            WorldSession* pSession = itSession->second;
            if (pSession->PlayerLoading())
                continue;
            Player* player = pSession->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            if (!player->InBattleground() &&  !player->InBattlegroundQueue())
                continue;

            if (team != TEAM_NEUTRAL)
            {
                if (player->GetTeamId() != team)
                    continue;
            }
            ++onlineCount;
        }
    }
    return onlineCount;
}

void PlayerBotMgr::Update()
{

    OnlinePlayerBotByGUIDQueue();

    if (!ExistUnBGPlayerBot())
    QueryBattlegroundRequirement();
#ifndef INCOMPLETE_BOT
    //sArenaTeamMgr->CheckPlayerBotArenaTeam();
  //  QueryRatedArenaRequirement();
   // QueryNonRatedArenaRequirement();
    if (m_LFGSearchTick > 1)
    {
        m_LFGSearchTick = 0;
        //if (lfg::LFGBotRequirement* pbotReq = sLFGMgr->SearchLFGBotRequirement())
        //{
        //	AddNewPlayerBotToLFG(pbotReq);
        //	delete pbotReq;
        //}
    }
    else
        ++m_LFGSearchTick;
#endif

    std::list<std::map<uint32, std::list<UnitAI*> >::iterator > delITer;
    uint32 currentTick = getMSTime();
    for (std::map<uint32, std::list<UnitAI*> >::iterator itDelayAi = m_DelayDestroyAIs.begin();
        itDelayAi != m_DelayDestroyAIs.end();
        itDelayAi++)
    {
        uint32 delayTick = itDelayAi->first;
        if (delayTick + 5000 >= currentTick)
        {
            for (std::list<UnitAI*>::iterator itAI = itDelayAi->second.begin();
                itAI != itDelayAi->second.end();
                itAI++)
            {
                UnitAI* pAI = (*itAI);
                if (pAI)
                    delete pAI;
            }
            itDelayAi->second.clear();
            delITer.push_back(itDelayAi);
        }
    }
    for (std::list<std::map<uint32, std::list<UnitAI*> >::iterator >::iterator itDel = delITer.begin();
        itDel != delITer.end();
        itDel++)
    {
        m_DelayDestroyAIs.erase(*itDel);
    }
}
