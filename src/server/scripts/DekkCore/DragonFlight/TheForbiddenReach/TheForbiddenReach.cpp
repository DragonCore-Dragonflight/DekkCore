/*
 * Copyright 2022 DekkCore
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

#include "PhasingHandler.h"
#include "ScriptMgr.h"
#include "TheForbiddenReach.h"
#include "Containers.h"
#include "Player.h"
#include "Spell.h"
#include "SpellScript.h"

enum DracthyrLoginSpells
{
    // Spells
    SPELL_DRACTHYR_LOGIN = 369728, // teleports to random room, plays scene for the room, binds the home position
    SPELL_STASIS_1 = 369735, // triggers 366620
    SPELL_STASIS_2 = 366620, // triggers 366636
    SPELL_STASIS_3 = 366636, // removes 365560, sends first quest (64864)
    SPELL_STASIS_4 = 365560, // freeze the target
    SPELL_DRACTHYR_MOVIE_ROOM_01 = 394245, // scene for room 1
    SPELL_DRACTHYR_MOVIE_ROOM_02 = 394279, // scene for room 2
    SPELL_DRACTHYR_MOVIE_ROOM_03 = 394281, // scene for room 3
    SPELL_DRACTHYR_MOVIE_ROOM_04 = 394282, // scene for room 4
    //SPELL_DRACTHYR_MOVIE_ROOM_05    = 394283, // scene for room 5 (only plays sound, unused?)
};

std::array<std::pair<uint32, Position>, 4> LoginRoomData =
{
    {
        { SPELL_DRACTHYR_MOVIE_ROOM_01, { 5725.32f, -3024.26f, 251.047f, 0.01745329238474369f } },
        { SPELL_DRACTHYR_MOVIE_ROOM_02, { 5743.03f, -3067.28f, 251.047f, 0.798488140106201171f } },
        { SPELL_DRACTHYR_MOVIE_ROOM_03, { 5787.1597f, -3083.3906f, 251.04698f, 1.570796370506286621f } },
        { SPELL_DRACTHYR_MOVIE_ROOM_04, { 5829.32f, -3064.49f, 251.047f, 2.364955902099609375f } }
    }
};

class DracthyrEvokers : public PlayerScript
{
public:
    DracthyrEvokers() : PlayerScript("DracthyrEvokers") { }

    void OnLogin(Player* player, bool firstLogin) override
    {
        if (player->GetClass() != CLASS_EVOKER)
            return;

        player->KilledMonsterCredit(181494);

        if(firstLogin)
        if (auto quest_ev = sObjectMgr->GetQuestTemplate(QUEST_AWAKEN_DRACTHYR))
            player->AddQuest(quest_ev, nullptr);
    }
    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (player->GetClass() == CLASS_EVOKER && player->GetLevel() >= 10)
        {
            player->LearnSpell(200749, false);
            player->ActivateTalentGroup(sChrSpecializationStore.LookupEntry(1467));
            //if (player->GetTeam() == TEAM_ALLIANCE)
            //    player->CompletedAchievement(sAchievementStore.LookupEntry(15325));
            //else if (player->GetTeam() == TEAM_HORDE)
            //    player->CompletedAchievement(sAchievementStore.LookupEntry(15638));
        }
    }
};

// 369728 - Dracthyr Login
class spell_dracthyr_login : public SpellScript
{
    PrepareSpellScript(spell_dracthyr_login);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DRACTHYR_MOVIE_ROOM_01, SPELL_DRACTHYR_MOVIE_ROOM_02, SPELL_DRACTHYR_MOVIE_ROOM_03, SPELL_DRACTHYR_MOVIE_ROOM_04 });
    }

    void HandleTeleport(SpellEffIndex /*effIndex*/)
    {
        std::pair<uint32, Position> const& room = LoginRoomData[urand(0, 3)];

        WorldLocation dest = GetHitUnit()->GetWorldLocation();
        SetExplTargetDest(dest);

        if (auto hitDest = GetHitDest())
            hitDest->Relocate(room.second);

        if (auto caster = GetCaster())
            caster->CastSpell(GetHitUnit(), room.first, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dracthyr_login::HandleTeleport, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
    }
};

// 3730 - Dracthyr Evoker Intro (Post Movie)
class scene_dracthyr_evoker_intro : public SceneScript
{
public:
    scene_dracthyr_evoker_intro() : SceneScript("scene_dracthyr_evoker_intro") { }

    void OnSceneComplete(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        player->CastSpell(player, SPELL_STASIS_1, true);
    }

    void OnSceneCancel(Player* player, uint32 /*sceneInstanceID*/, SceneTemplate const* /*sceneTemplate*/) override
    {
        player->CastSpell(player, SPELL_STASIS_1, true);
    }
};

//181494 Dervishian
class npc_Dervishian : public CreatureScript
{
public:
    npc_Dervishian() : CreatureScript("npc_Dervishian") { }

    struct npc_DervishianAI : public ScriptedAI
    {
        npc_DervishianAI(Creature* creature) : ScriptedAI(creature) { }

        bool OnGossipHello(Player* player) override
        {

        if (player->HasQuest(QUEST_AWAKEN_DRACTHYR))
            player->KilledMonsterCredit(181680);
            player->KilledMonsterCredit(187015);
            player->KilledMonsterCredit(181712);
            player->KilledMonsterCredit(181494);

            return true;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_DervishianAI(creature);
    }
};

class Lapisagos_Death : public CreatureScript
{
public:
    Lapisagos_Death() : CreatureScript("Lapisagos_Death") { }

    struct Lapisagos_DeathAI : public ScriptedAI
    {
        Lapisagos_DeathAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
        }

        void JustDied(Unit* /*killer*/) override
        {
            std::list<Player*> players;
            Trinity::AnyPlayerInObjectRangeCheck checker(me, 100.0f);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, players, checker);
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = (*itr);
                if (player->HasQuest(64871))
                {
                    player->ForceCompleteQuest(64871);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new Lapisagos_DeathAI(creature);
    }
};

void AddSC_TheForbiddenReach()
{
    new DracthyrEvokers();
    RegisterSpellScript(spell_dracthyr_login);
    new scene_dracthyr_evoker_intro();
    new npc_Dervishian();
    new Lapisagos_Death();
}
