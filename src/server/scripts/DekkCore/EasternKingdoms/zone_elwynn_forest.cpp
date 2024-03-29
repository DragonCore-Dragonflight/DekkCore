/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ObjectMgr.h"
#include "World.h"
#include "PetAI.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Player.h"
#include "SpellScript.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"

#define NPC_WOLF    49871


enum
{
    QUEST_FEAR_NO_EVIL_WORGEN_WARRIOR = 28813,
    QUEST_FEAR_NO_EVIL_ALLIANCE = 29082,
    QUEST_FEAR_NO_EVIL_ALLIANCE_2 = 28809,
    QUEST_FEAR_NO_EVIL_ALLIANCE_3 = 28808,
    QUEST_FEAR_NO_EVIL_ALLIANCE_4 = 28811,
    QUEST_FEAR_NO_EIVL_ALLIANCE_5 = 28810,
    QUEST_FEAR_NO_EVIL_ALLIANCE_6 = 28806,
    QUEST_FEAR_NO_EVIL_ALLIANCE_NIGHT_ELF_WARLOCK_DK = 28812,
    QUEST_JASPERLODE_MINE = 76,
};



/*######
## npc_stormwind_infantry
######*/

class npc_stormwind_infantry : public CreatureScript
{
public:
    npc_stormwind_infantry() : CreatureScript("npc_stormwind_infantry") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_stormwind_infantryAI(creature);
    }

    struct npc_stormwind_infantryAI : public ScriptedAI
    {
        npc_stormwind_infantryAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 waitTime;
        ObjectGuid wolfTarget;

        void Reset() override
        {
            wolfTarget = ObjectGuid::Empty;
            me->SetSheath(SHEATH_STATE_MELEE);
            waitTime = urand(0, 2000);
        }

        void DamageTaken(Unit* doneBy, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (doneBy->ToCreature())
                if (me->GetHealth() <= damage || me->GetHealthPct() <= 80.0f)
                    damage = 0;
        }

        void DamageDealt(Unit* target, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (target->ToCreature())
                if (target->GetHealth() <= damage || target->GetHealthPct() <= 70.0f)
                    damage = 0;
        }

        void UpdateAI(uint32 diff) override
        {
            DoMeleeAttackIfReady();

            if (waitTime && waitTime >= diff)
            {
                waitTime -= diff;
                return;
            }

            waitTime = urand(10000, 20000);

            if (!wolfTarget.IsEmpty())
            {
                if (Creature* wolf = ObjectAccessor::GetCreature(*me, wolfTarget))
                {
                    if (wolf->IsAlive())
                    {
                        if (me->GetVictim() != wolf)
                        {
                            me->GetThreatManager().AddThreat(wolf, 1000000.0f);
                            wolf->GetThreatManager().AddThreat(me, 1000000.0f);
                            me->Attack(wolf, true);
                        }
                    }
                    else
                    {
                        wolf->DespawnOrUnsummon();
                        wolfTarget = ObjectGuid::Empty;
                    }
                }
            }
            else
            {
                Position wolfPos = me->GetPosition();
                GetPositionWithDistInFront(me, 2.5f, wolfPos);

                if (Creature* wolf = me->SummonCreature(NPC_WOLF, wolfPos))
                {
                    me->GetThreatManager().AddThreat(wolf, 1000000.0f);
                    wolf->GetThreatManager().AddThreat(me, 1000000.0f);
                    AttackStart(wolf);
                    wolf->SetFacingToObject(me);
                    wolfTarget = wolf->GetGUID();

                }
            }
        }
    };
};


/*######
## npc_training_dummy_elwynn
######*/

enum TrainingDummySpells
{
    SPELL_CHARGE = 100,
    SPELL_AUTORITE = 105361, // OnDamage
    SPELL_ASSURE = 56641,
    SPELL_EVISCERATION = 2098,
    SPELL_MOT_DOULEUR_1 = 589,
    SPELL_MOT_DOULEUR_2 = 124464, // Je ne sais pas si un des deux est le bon
    SPELL_NOVA = 122,
    SPELL_CORRUPTION_1 = 172,
    SPELL_CORRUPTION_2 = 87389,
    SPELL_CORRUPTION_3 = 131740,
    SPELL_PAUME_TIGRE = 100787
};

class npc_training_dummy_start_zones : public CreatureScript
{
public:
    npc_training_dummy_start_zones() : CreatureScript("npc_training_dummy_start_zones") { }

    struct npc_training_dummy_start_zonesAI : CritterAI
    {
        npc_training_dummy_start_zonesAI(Creature* creature) : CritterAI(creature)
        {}

        uint32 resetTimer;

        void Reset() override
        {
            me->SetControlled(true, UNIT_STATE_STUNNED);//disable rotate
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);//imune to knock aways like blast wave

            resetTimer = 5000;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (!_EnterEvadeMode())
                return;

            Reset();
        }

        void MoveInLineOfSight(Unit* p_Who) override
        {
            if (!me->IsWithinDistInMap(p_Who, 25.f) && p_Who->IsInCombat())
            {
                me->RemoveAllAuras();
            }
        }

        void DamageTaken(Unit* doneBy, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            resetTimer = 5000;
            damage = 0;

            if (doneBy->HasAura(SPELL_AUTORITE))
            {
                if (Player* player = doneBy->ToPlayer())
                {
                    player->KilledMonsterCredit(44175);
                    player->KilledMonsterCredit(44548);

                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            return;
        }

        void SpellHit(WorldObject* Caster, const SpellInfo* Spell) override
        {
            switch (Spell->Id)
            {
            case SPELL_CHARGE:
            case SPELL_ASSURE:
            case SPELL_EVISCERATION:
            case SPELL_MOT_DOULEUR_1:
            case SPELL_MOT_DOULEUR_2:
            case SPELL_NOVA:
            case SPELL_CORRUPTION_1:
            case SPELL_CORRUPTION_2:
            case SPELL_CORRUPTION_3:
            case SPELL_PAUME_TIGRE:
            {
                if (Player* player = Caster->ToPlayer())
                {
                    player->KilledMonsterCredit(44175);
                    player->KilledMonsterCredit(44548);
                }
                break;
            }
            default:
                break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (!me->HasUnitState(UNIT_STATE_STUNNED))
                me->SetControlled(true, UNIT_STATE_STUNNED);//disable rotate

            if (resetTimer <= diff)
            {
                EnterEvadeMode(EVADE_REASON_OTHER);
                resetTimer = 5000;
            }
            else
                resetTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_training_dummy_start_zonesAI(creature);
    }
};

/*######
## spell_quest_fear_no_evil
######*/

class spell_quest_fear_no_evil : public SpellScriptLoader
{
public:
    spell_quest_fear_no_evil() : SpellScriptLoader("spell_quest_fear_no_evil") { }

    class spell_quest_fear_no_evil_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_quest_fear_no_evil_SpellScript);

        void OnDummy(SpellEffIndex /*effIndex*/)
        {
            if (GetCaster())
                if (GetCaster()->ToPlayer())
                    GetCaster()->ToPlayer()->KilledMonsterCredit(50047);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_quest_fear_no_evil_SpellScript::OnDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_quest_fear_no_evil_SpellScript();
    }

};

/*######
## spell_quest_extincteur
######*/

enum eSpellQuestExtincteur
{
    NPC_FIRE = 42940,
};

class spell_quest_extincteur : public SpellScriptLoader
{
public:
    spell_quest_extincteur() : SpellScriptLoader("spell_quest_extincteur") { }

    class spell_quest_extincteur_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_quest_extincteur_SpellScript);

        void OnDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Creature* fire = GetHitCreature();

            if (!caster || !fire)
                return;

            if (fire->GetEntry() != NPC_FIRE)
                return;

            if (Player* player = caster->ToPlayer())
                player->KilledMonsterCredit(NPC_FIRE, fire->GetGUID());

            fire->DespawnOrUnsummon();
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_quest_extincteur_SpellScript::OnDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_quest_extincteur_SpellScript();
    }

};


/*######
## npc_hogger
######*/

enum HoggerSpellData
{
    SPELL_SUMMON_MINIONS = 87366,
    SPELL_VICIOUS_SLICE = 87337,
    SPELL_EATING = 87351,
    SPELL_TELEPORT_VISUAL_ONLY_1 = 87459,
    SPELL_TELEPORT_VISUAL_ONLY_2 = 64446,
    SPELL_BLOODY_STRIKE = 87359
};

enum HoggerEventData
{
    EVENT_VICIOUS_SLICE = 1,
    EVENT_HAMMOND_GROUP_START_WALKING = 2,
    EVENT_DISMOUNT_HAMMOND_CLAY = 3,
    EVENT_HOGGER_SAY_GRR = 4,
    EVENT_CLAYS_EXPLAINATION = 5,
    EVENT_CLAY_SAYS_TAKE_HIM = 6,
    EVENT_HOGGER_SAYS_NOO = 7,
    EVENT_CLAY_SPEAKS_TO_ANDROMATH = 8,
    EVENT_TELEPORT_BACK = 9,
    EVENT_CHECK_EAT_RANGE = 10,
    EVENT_BLOODY_STRIKE = 11
};

enum HoggerTextData
{
    SAY_HOGGER_SUMMON_MINIONS = 2,
    SAY_EATING = 3,
    SAY_STUNNED = 4,
    SAY_BEG = 5,
    SAY_RAND = 6,
    SAY_FINAL = 7
};

enum HammondTextData
{
    YELL_OPENING = 0,
    SAY_EXPLAINATION = 1,
    SAY_TAKE_HIM = 2,
    SAY_TO_ANDROMATH = 3
};

enum RagamuffinTextData
{
    SAY_CLAY = 0,
    SAY_WOW = 1,
};

enum HoggerCreatures
{
    NPC_GENERAL_HAMMOND_CLAY = 65153,
    NPC_ANDROMATH = 46941,
    NPC_DUMAS = 46940,
    NPC_HOGGER = 448,
    NPC_EATING_TARGET = 45979,
    NPC_RAGAMUFFIN = 46943,
    NPC_HOGGER_MINION = 46932
};

enum HoggerMiscData
{
    DISPLAYID_GENERAL_HAMMOND_CLAYS_MOUNT = 2410
};

// validated positions
static const Position generalHammondClayPositions[4] =
{
    { -10125.35f, 650.7324f, 36.05776f },
    { -10128.3f,  656.4648f, 36.05776f },
    { -10131.25f, 662.1973f, 36.05776f },
    { -10135.73f, 668.389f,  35.74807f }
};

// validated positions
static const Position andromathPositions[3] =
{
    { -10119.2f, 647.913f, 36.36745f },
    { -10123.0f, 656.875f, 36.05776f },
    { -10126.8f, 665.837f, 35.74807f }
};

// validated positions
static const Position dumasPositions[3] =
{
    { -10130.1f, 647.7671f, 36.04665f },
    { -10132.9f, 653.5605f, 36.05776f },
    { -10135.7f, 659.354f,  36.06887f }
};

// validated positions
static const Position hoggerPositions[1] =
{
    { -10136.9f, 670.009f, 36.03682f }
};

// validated positions
static const Position ragamuffinPositions[2] =
{
    { -10127.00f, 651.0f, 36.05776f },
    { -10123.0f, 651.0f,  36.06887f }
};

struct npc_hogger : public ScriptedAI
{
    npc_hogger(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();

        _minionsSummoned = false;
        _endingSceneActive = false;
        _alreadyEaten = false;
        _isEating = false;

        me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->SetFaction(20);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetWalk(false);
    }

void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (_endingSceneActive)
        {
            damage = 0;
            return;
        }

        if (!_minionsSummoned && me->HealthBelowPctDamaged(50, damage))
            SummonMinions();

        if (!_alreadyEaten && me->HealthBelowPctDamaged(30, damage))
            MoveToEatingPosition();


        if (me->GetHealth() <= damage)
        {
            damage = 0;
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->SetFaction(35);
            me->SetHealth(1);
            StartEndingScene();
            attacker->ToPlayer()->KilledMonsterCredit(448);

        }

        if (_isEating)
        {
            _isEating = false;
            me->SetReactState(REACT_AGGRESSIVE);
            Talk(SAY_STUNNED);
        }
    }

    void EnterCombat(Unit* /*who*/)
    {
        _events.ScheduleEvent(EVENT_VICIOUS_SLICE, 3s);
    }

    void MoveToEatingPosition()
    {
        _alreadyEaten = true;

        if (Creature* target = me->FindNearestCreature(NPC_EATING_TARGET, 100.0f))
        {
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(0, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), true);
            _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
        }
    }

    void StartEndingScene()
    {
        _endingSceneActive = true;
        _events.Reset();

        me->SetReactState(REACT_PASSIVE);
        me->StopMoving();
        me->AttackStop();

        Talk(SAY_BEG);

        SummonGeneralHammondClay();
        SummonAndromath();
        SummonDumas();

        _events.ScheduleEvent(EVENT_HAMMOND_GROUP_START_WALKING, 1s);
    }

    void SummonGeneralHammondClay()
    {
        if (TempSummon* hammond = me->SummonCreature(NPC_GENERAL_HAMMOND_CLAY, generalHammondClayPositions[0]))
        {
            _generalHammondGUID = hammond->GetGUID();
            hammond->CastSpell(hammond, SPELL_TELEPORT_VISUAL_ONLY_1, true);
            hammond->Mount(DISPLAYID_GENERAL_HAMMOND_CLAYS_MOUNT);
            hammond->AI()->Talk(YELL_OPENING);
        }

        // summon ragamuffins and do text
        if (TempSummon* ragamuffin1 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[0], TEMPSUMMON_TIMED_DESPAWN, 1min))
        {
            ragamuffin1->AI()->Talk(SAY_CLAY);
        }

        if (TempSummon* ragamuffin2 = me->SummonCreature(NPC_RAGAMUFFIN, ragamuffinPositions[1], TEMPSUMMON_TIMED_DESPAWN, 1min))
        {
            ragamuffin2->AI()->Talk(SAY_WOW);
        }
    }


    void MoveGeneralHammondClay()
    {
        if (GetHammond())
        {
            GetHammond()->SetWalk(true);
            GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[2], true);
            _events.ScheduleEvent(EVENT_DISMOUNT_HAMMOND_CLAY, 8500ms);
        }
    }

    void SummonAndromath()
    {
        TempSummon* andromath = me->SummonCreature(NPC_ANDROMATH, andromathPositions[0]);
        if (andromath)
        {
            _andromathGUID = andromath->GetGUID();
            andromath->CastSpell(andromath, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveAndromath()
    {
        if (GetAndromath())
        {
            GetAndromath()->SetWalk(true);
            GetAndromath()->GetMotionMaster()->MovePoint(0, andromathPositions[2], true);
        }
    }

    void SummonDumas()
    {
        TempSummon* dumas = me->SummonCreature(NPC_DUMAS, dumasPositions[0]);
        if (dumas)
        {
            _dumasGUID = dumas->GetGUID();
            dumas->CastSpell(dumas, SPELL_TELEPORT_VISUAL_ONLY_1, true);
        }
    }

    void MoveDumas()
    {
        if (GetDumas())
        {
            GetDumas()->SetWalk(true);
            GetDumas()->GetMotionMaster()->MovePoint(0, dumasPositions[2], true);
        }
    }

    void SummonMinions()
    {
        me->CastStop();
        Talk(SAY_HOGGER_SUMMON_MINIONS);
        DoCastSelf(SPELL_SUMMON_MINIONS, true);
        /*
        for (float distance : { 0.5f, 1.5f, 2.5f })
        {
            Position hogPos = me->GetPosition();
            GetPositionWithDistInFront(me, distance, hogPos);
            float z = me->GetMap()->GetHeight(me->GetPhaseShift(), hogPos.GetPositionX(), hogPos.GetPositionY(), hogPos.GetPositionZ());
            hogPos.m_positionZ = z;
            me->SummonCreature(NPC_HOGGER_MINION, hogPos);
        }
        */
        _minionsSummoned = true;
    }

    void MoveHoggerToFinalPosition()
    {
        me->SetWalk(true);
        me->GetMotionMaster()->MovePoint(0, hoggerPositions[0]);
    }

    void TeleportBack()
    {
        if (GetHammond() && GetAndromath() && GetDumas())
        {
            GetHammond()->CastSpell(GetHammond(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetAndromath()->CastSpell(GetAndromath(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            GetDumas()->CastSpell(GetDumas(), SPELL_TELEPORT_VISUAL_ONLY_2, true);
            DoCastSelf(SPELL_TELEPORT_VISUAL_ONLY_2, true);

            me->DisappearAndDie();
            GetHammond()->DisappearAndDie();
            GetAndromath()->DisappearAndDie();
            GetDumas()->DisappearAndDie();
        }
    }

    Creature* GetHammond()
    {
        return me->GetMap()->GetCreature(_generalHammondGUID);
    }

    Creature* GetAndromath()
    {
        return me->GetMap()->GetCreature(_andromathGUID);
    }

    Creature* GetDumas()
    {
        return me->GetMap()->GetCreature(_dumasGUID);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim() && !_endingSceneActive)
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_VICIOUS_SLICE:
                DoCastVictim(SPELL_VICIOUS_SLICE);
                _events.Repeat(3s);
                break;

            case EVENT_HAMMOND_GROUP_START_WALKING:
                MoveGeneralHammondClay();
                MoveAndromath();
                MoveDumas();
                MoveHoggerToFinalPosition();
                break;

            case EVENT_DISMOUNT_HAMMOND_CLAY:
                if (GetHammond())
                {
                    me->SetFacingToObject(GetHammond());

                    GetHammond()->Dismount();
                    GetHammond()->GetMotionMaster()->MovePoint(0, generalHammondClayPositions[3], true);

                    _events.ScheduleEvent(EVENT_HOGGER_SAY_GRR, 3s);
                }
                break;

            case EVENT_HOGGER_SAY_GRR:
                Talk(SAY_RAND);
                _events.ScheduleEvent(EVENT_CLAYS_EXPLAINATION, 3s);
                break;

            case EVENT_CLAYS_EXPLAINATION:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_EXPLAINATION);
                _events.ScheduleEvent(EVENT_CLAY_SAYS_TAKE_HIM, 4500ms);
                break;

            case EVENT_CLAY_SAYS_TAKE_HIM:
                if (GetHammond())
                    GetHammond()->AI()->Talk(SAY_TAKE_HIM);
                _events.ScheduleEvent(EVENT_HOGGER_SAYS_NOO, 2s);
                break;

            case EVENT_HOGGER_SAYS_NOO:
                Talk(SAY_FINAL);
                _events.ScheduleEvent(EVENT_CLAY_SPEAKS_TO_ANDROMATH, 3s);
                break;

            case EVENT_CLAY_SPEAKS_TO_ANDROMATH:
                if (GetHammond() && GetAndromath())
                {
                    GetHammond()->SetFacingToObject(GetAndromath());
                    GetAndromath()->SetFacingToObject(GetHammond());
                    GetHammond()->AI()->Talk(SAY_TO_ANDROMATH);
                }
                _events.ScheduleEvent(EVENT_TELEPORT_BACK, 4s);
                break;

            case EVENT_TELEPORT_BACK:
                TeleportBack();
                break;

            case EVENT_CHECK_EAT_RANGE:
                if (!me->FindNearestCreature(NPC_EATING_TARGET, 3.0f))
                {
                    _events.ScheduleEvent(EVENT_CHECK_EAT_RANGE, 200ms);
                    break;
                }
                else
                {
                    DoCast(SPELL_EATING);
                    Talk(SAY_EATING);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 100ms);
                    _isEating = true;
                }
                break;

            case EVENT_BLOODY_STRIKE:
                if (_isEating)
                {
                    if (Creature* dummy = me->FindNearestCreature(NPC_EATING_TARGET, 10.0f))
                        DoCast(dummy, SPELL_BLOODY_STRIKE, true);
                    _events.ScheduleEvent(EVENT_BLOODY_STRIKE, 1s);
                }

            default:
                break;
            }
        }

        if (!_endingSceneActive)
            DoMeleeAttackIfReady();
    }

private:
    EventMap _events;

    bool _minionsSummoned;
    bool _endingSceneActive;
    bool _alreadyEaten;
    bool _isEating;

    ObjectGuid _generalHammondGUID;
    ObjectGuid _andromathGUID;
    ObjectGuid _dumasGUID;
};

/*######
## npc_hogger_minion
######*/

enum HoggerMinionSpellData
{
    SPELL_ADVENTURERS_RUSH = 87402
};

struct npc_hogger_minion : public ScriptedAI
{
    npc_hogger_minion(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetReactState(REACT_AGGRESSIVE);

        if (Creature* hogger = me->FindNearestCreature(NPC_HOGGER, 35.0f, true))
            me->CastSpell(hogger, SPELL_ADVENTURERS_RUSH, true);
    }
};


//88
struct at_fargodeep_mine : public AreaTriggerAI
{
    at_fargodeep_mine(AreaTrigger* at) : AreaTriggerAI(at) { }

    void OnUnitEnter(Unit* unit) override
    {
        if (unit->IsPlayer())
        {
            if (Player* player = unit->ToPlayer())
            {
                if (player->GetQuestStatus(62) == QUEST_STATUS_INCOMPLETE)
                {
                    player->ForceCompleteQuest(62);
                }
            }
        }
    }
};

//87
struct at_jasperlode_mine : public AreaTriggerAI
{
    at_jasperlode_mine(AreaTrigger* at) : AreaTriggerAI(at) { }

    void OnUnitEnter(Unit* unit) override
    {
        if (unit->IsPlayer())
        {
            if (Player* player = unit->ToPlayer())
            {
                if (player->GetQuestStatus(76) == QUEST_STATUS_INCOMPLETE)
                {
                    player->ForceCompleteQuest(76);
                }
            }
        }
    }
};

enum bquest
{
    QUEST_ON_THE_MEND = 31309,
};

struct npc_erma : public ScriptedAI
{
    npc_erma(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        if (player->GetQuestStatus(QUEST_ON_THE_MEND) == QUEST_STATUS_INCOMPLETE)
            player->ForceCompleteQuest(QUEST_ON_THE_MEND);

        return true;
    }
};

void AddSC_DekkCore_elwyn_forest()
{
    new npc_stormwind_infantry();
    new npc_training_dummy_start_zones();
    new spell_quest_fear_no_evil();
    new spell_quest_extincteur();
    RegisterCreatureAI(npc_hogger);
    RegisterCreatureAI(npc_hogger_minion);
    RegisterAreaTriggerAI(at_fargodeep_mine);
    RegisterAreaTriggerAI(at_jasperlode_mine);
    RegisterCreatureAI(npc_erma);
}
