#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "vault_of_the_wardens.h"

enum Says
{
    SAY_AGGRO = 0,
    SAY_SHADOWSTEP = 1,
    SAY_SHADOWSTEP_EMOTE = 2,
    SAY_SHADOWSTEP_REMOVE = 3,
    SAY_GLAIVE = 4,
    SAY_KILLED = 5,
    SAY_TURN_KICK = 6,
    SAY_DEATH = 7,
    SAY_EVADE = 8,
    SAY_EMOTE_CREEPING = 10,
};

enum Spells
{
    SPELL_KNOCKDOWN_KICK = 197251,
    SPELL_TURN_KICK = 197250,
    SPELL_DEEPENING_SHADOWS = 213583,  // 213397
    SPELL_DEEPENING_SHADOWS_TWO = 213576,  // two targets
    SPELL_DEEPENING_SHADOWS_RAND = 197551,
    SPELL_DETONATING_MOONGLAIVE = 197513,
    SPELL_SHADOWSTEP = 203416,
    SPELL_SHADOWSTEP_2 = 204635,
    SPELL_VENGEANCE = 204984,   //! To-Do invis for npc and cast from invis with visual
    SPELL_VENGEANCE_SHIELD = 205004,

    // while invis
    SPELL_INVISE_BY_SHADOWSTEP = 204461,
    SPELL_INVISE_BY_SHADOWSTEP_1 = 192750,
    SPELL_FEL_GLAIVE = 197333,
    SPELL_STEAL_LIGHT = 206387,

    // heroic and mythic
    SPELL_CREEPING_DOOM_SHIELD = 197422,
    SPELL_CREEPING_DOOM_SECOND = 213685,
    SPELL_CREEPING_DOOM_DMG = 197507,

    //Other
    SPELL_ELUNES_LIGHT = 201359,
    SPELL_ELUNES_LIGHT_VIS_1 = 197897,
    SPELL_ELUNES_LIGHT_VIS_2 = 204294,
    SPELL_ELUNES_LIGHT_CLICK = 197890,
    SPELL_ELUNES_LIGHT_OVERRIDE = 192656,
    SPELL_ELUNES_LIGHT_SUMMON = 204491,
    SPELL_ELUNES_LIGHT_DMG = 192752,


    SPELL_INTRO = 198309,
};

enum eEvents
{
    EVENT_KNOCKDOWN_KICK = 1,
    EVENT_KNOCKDOWN_KICK_TRIGGER,
    EVENT_DEEPENING_SHADOWS,
    EVENT_DETONATING_MOONGLAIVE,
    EVENT_SHADOWSTEP,
    EVENT_VENGEANCE,
    EVENT_CREEPING_FIRST,
    EVENT_CREEPING_SECOND,

    EVENT_1,

    ACTION_1,
};

Position const centrPos = { 4049.907f, -297.4254f, -281.4167f };

std::map<uint8, std::pair<Position, Position> > paths =
{
    {0, {{4074.16f, -325.45f, -281.11f}, {4028.46f, -325.07f, -281.11f}}},
    {1, {{4022.30f, -321.86f, -281.11f}, {4022.86f, -273.61f, -281.11f}}},
    {2, {{4027.00f, -269.02f, -281.11f}, {4074.33f, -269.34f, -281.11f}}}
};

Position const vengeance_position[4]
{
    {4027.47f, -273.93f, -281.11f, 0.0f},
    {4074.04f, -274.42f, -281.11f, 0.0f},
    {4074.04f, -320.78f, -281.11f, 0.0f},
    {4027.47f, -320.78f, -281.11f, 0.0f},
};

std::map<uint8, std::pair<Position, Position> > creeping_walls =
{
    {0, {{4020.11f, -267.21f, -281.11f, 6.26f}, {4020.11f, -327.91f, -281.11f, 6.26f}}},
    {1, {{4081.06f, -327.91f, -281.11f, 1.55f}, {4020.11f, -327.91f, -281.11f, 1.55f}}},
    {2, {{4081.06f, -267.21f, -281.11f, 3.17f}, {4081.06f, -327.91f, -281.11f, 3.17f}}},
    {3, {{4081.06f, -267.21f, -281.11f, 4.71f}, {4020.11f, -267.21f, -281.11f, 4.71f}}}
};

//95888
class boss_cordana : public CreatureScript
{
public:
    boss_cordana() : CreatureScript("boss_cordana") {}

    struct boss_cordanaAI : public BossAI
    {
        boss_cordanaAI(Creature* creature) : BossAI(creature, DATA_CORDANA)
        {
            intro = false;
            SetCanSeeEvenInPassiveMode(true);
            //me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNINTERACTIBLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_ATTACKABLE_1);
        }

        bool intro, vengeancePhase, creeping_phase;
        uint8 phase, vengeance_health, creeping_health;
        uint8 count_vengeance;
        uint16 lightdetect = 1000;
        bool achievement = true;

        void Reset() override
        {
            vengeancePhase = false;
            vengeance_health = 50;
            count_vengeance = 0;

            achievement = true;

            creeping_phase = true;
            creeping_health = 0;

            //! Needed fixes for invise =C
            if (me->GetMap()->GetDifficultyID() != DIFFICULTY_NORMAL)
            {
                vengeance_health = 25;
                creeping_phase = false;
                creeping_health = 40;
            }


            _Reset();
            me->SetWalk(false);
            me->SetCanFly(true);

            me->SetReactState(REACT_DEFENSIVE);
            me->RemoveAurasDueToSpell(SPELL_VENGEANCE_SHIELD);
            me->RemoveAurasDueToSpell(SPELL_FEL_GLAIVE);

            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ELUNES_LIGHT_OVERRIDE);
            instance->SetData(DATA_CORDANA_ACTIONS, 1); // delete elunes npc
            instance->SetData(DATA_CORDANA_ACTIONS, 2); // clear centry
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(SAY_AGGRO);

            Reset();
            _JustEngagedWith(who);

            me->CastSpell(me, SPELL_ELUNES_LIGHT, true);

            ScheduleEvents();
            events.RescheduleEvent(EVENT_SHADOWSTEP, 20s);  // only one time

            me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveUnitFlag(UNIT_FLAG_NOT_ATTACKABLE_1);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(SAY_DEATH);
            _JustDied();
            me->SummonCreature(103860, 4064.80f, -281.94f, -281.42f, 3.9f);

            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ELUNES_LIGHT_OVERRIDE);

            if (achievement && (me->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC || me->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC_KEYSTONE))
                instance->DoUpdateCriteria(CriteriaType::BeSpellTarget, 215485);

            DoCast(me, SPELL_ELUNES_LIGHT_SUMMON, true);
        }

        void DoAction(int32 const action) override
        {
            if (action == ACTION_1)
            {
                achievement = false;
            }
        }

        void KilledUnit(Unit* /*killer*/) override
        {
            Talk(SAY_KILLED);
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() == 104293 && summons.HasEntry(104293)) //! it's hack =C becaouse sometimes i have doubles
                summon->DespawnOrUnsummon();

            summons.Summon(summon);
            DoZoneInCombat(summon);

            if (summon->GetEntry() != 100336)
                summon->SetControlled(true, UNIT_STATE_ROOT);

            if (summon->GetEntry() == 104293) // dark
            {
                std::list<Player*> playerList;
                me->GetPlayerListInGrid(playerList, 100.0f);
                if (!playerList.empty())
                    for (auto player : playerList)
                        if (player->HasAura(SPELL_ELUNES_LIGHT_OVERRIDE))
                        {
                            player->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_OVERRIDE);
                            summon->CastSpell(summon, 206567);
                            summon->CastSpell(player, 206387);
                        }

                instance->SetData(DATA_CORDANA_ACTIONS, 1); // delete elunes npc
                instance->SetData(DATA_CORDANA_ACTIONS, 2); // clear centry
            }
            else if (summon->GetEntry() == 100351) // vengeance
            {
                // summon->CastSpell(summon, SPELL_INVISE_BY_SHADOWSTEP);
                me->AddAura(SPELL_VENGEANCE_SHIELD, me);
                count_vengeance++;
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (summon->GetEntry() == 104293) // dark
                summon->CastSpell(summon, SPELL_ELUNES_LIGHT_SUMMON, true);

            if (summon->GetEntry() == 100351 && (--count_vengeance) == 0) // hack again =C
                me->RemoveAurasDueToSpell(SPELL_VENGEANCE_SHIELD);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER || who->ToPlayer()->IsGameMaster())
                return;

            if (who->GetPositionX() >= 4093.63f && who->GetPositionX() <= 4168.71f && who->HasAura(SPELL_ELUNES_LIGHT_OVERRIDE))
                who->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_OVERRIDE);

            if (me->GetDistance(who) < 80.0f && !intro)
            {
                intro = true;
                me->GetMotionMaster()->MovePoint(1, centrPos);
                me->SetHomePosition(centrPos);
                DoCast(SPELL_INTRO);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
            case 1:
                me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                me->RemoveUnitFlag(UNIT_FLAG_NOT_ATTACKABLE_1);
                break;
            case 2:
                StartMove(urand(0, 2));
                break;
            case 3:
                SummonCreepings(1);
                break;
            }
        }

        void SpellHitTarget(WorldObject* target, const SpellInfo* spell) override
        {
            switch (spell->Id)
            {
            case SPELL_KNOCKDOWN_KICK:
                events.RescheduleEvent(EVENT_KNOCKDOWN_KICK_TRIGGER, 5s);
                Talk(SAY_TURN_KICK);
                break;
            }
        }
        void OnSpellCast(SpellInfo const* spell) override
        {

            switch (spell->Id)
            {
            case SPELL_CREEPING_DOOM_SECOND:
                SummonCreepings(2);
                break;
            }
        }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPct(creeping_health) && !creeping_phase)
            {
                creeping_phase = true;
                events.RescheduleEvent(EVENT_CREEPING_FIRST, 1s);
            }
            if (me->HealthBelowPct(vengeance_health) && !vengeancePhase && !me->HasAura(SPELL_CREEPING_DOOM_SHIELD))
            {
                vengeancePhase = true;
                events.RescheduleEvent(EVENT_VENGEANCE, 1s);
            }
        }

        void ScheduleEvents()
        {
            events.RescheduleEvent(EVENT_KNOCKDOWN_KICK, 8s);
            events.RescheduleEvent(EVENT_DEEPENING_SHADOWS, 11s);
            events.RescheduleEvent(EVENT_DETONATING_MOONGLAIVE, 11s);
            if (me->HealthBelowPct(vengeance_health) && vengeancePhase)
                events.RescheduleEvent(EVENT_VENGEANCE, 35s);
            if (me->HealthBelowPct(creeping_health) && creeping_phase)
                events.RescheduleEvent(EVENT_CREEPING_SECOND, 35s);
        }

        bool checkaura()
        {
            auto const& players = me->GetMap()->GetPlayers();
            if (!players.isEmpty())
            {
                for (auto const& itr : players)
                {
                    if (auto player = itr.GetSource())
                    {
                        if (player->HasAura(192656))
                            return false;
                    }
                }
            }
            return true;
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (vengeancePhase)
            {
                if (lightdetect <= diff)
                {
                    lightdetect = 1000;

                    if (!checkaura())
                        achievement = false;
                }
                else
                    lightdetect -= diff;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_KNOCKDOWN_KICK:
                    DoCastVictim(SPELL_KNOCKDOWN_KICK);
                    events.RescheduleEvent(EVENT_KNOCKDOWN_KICK, 16s);
                    break;
                case EVENT_KNOCKDOWN_KICK_TRIGGER:
                    if (Unit* vict = me->GetVictim())
                        if (vict->HasAura(SPELL_ELUNES_LIGHT_OVERRIDE))
                            DoCast(vict, SPELL_TURN_KICK);
                    break;
                case EVENT_DEEPENING_SHADOWS:
                    if (me->GetMap()->GetDifficultyID() == DIFFICULTY_NORMAL)
                        DoCast(SPELL_DEEPENING_SHADOWS);
                    else
                        DoCast(SPELL_DEEPENING_SHADOWS_RAND);
                    events.RescheduleEvent(EVENT_DEEPENING_SHADOWS, 3s);
                    break;
                case EVENT_DETONATING_MOONGLAIVE:
                    Talk(SAY_GLAIVE);
                    DoCast(SPELL_DETONATING_MOONGLAIVE);
                    events.RescheduleEvent(EVENT_DETONATING_MOONGLAIVE, 9s);
                    break;
                case EVENT_SHADOWSTEP:
                    Talk(SAY_SHADOWSTEP);
                    DoCast(SPELL_SHADOWSTEP);

                    DoCast(me, SPELL_SHADOWSTEP_2, true);
                    Talk(SAY_SHADOWSTEP_EMOTE);

                    me->AttackStop();

                    events.Reset();
                    DoCast(SPELL_FEL_GLAIVE);

                    StartMove(urand(0, 2));
                    break;
                case EVENT_VENGEANCE:
                    me->CastSpell(vengeance_position[urand(0, 3)], SPELL_VENGEANCE, false);
                    events.RescheduleEvent(EVENT_VENGEANCE, 35s);
                    break;
                case EVENT_CREEPING_FIRST:
                    me->AttackStop();

                    events.Reset();
                    DoCast(SPELL_CREEPING_DOOM_SHIELD);
                    break;
                case EVENT_CREEPING_SECOND:
                    DoCast(SPELL_CREEPING_DOOM_SECOND);
                    events.RescheduleEvent(EVENT_CREEPING_SECOND, 70s);  // second time
                    break;

                }
            }
            DoMeleeAttackIfReady();
        }

        void StartMove(uint8 point)
        {
            if (urand(0, 1) == 1)
                std::swap(paths[point].first, paths[point].second);

            me->SetWalk(true);
            me->NearTeleportTo(paths[point].first);
            me->GetMotionMaster()->MovePoint(2, paths[point].second);

        }

        void SummonCreepings(uint8 wave)
        {
            Talk(SAY_EMOTE_CREEPING);
            uint8 waves = 0;
            uint32 timer = 0;
            switch (wave)
            {
            case 1:
            {
                if (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC) // 2 walls
                {
                    waves = 2;
                    timer = 17000;
                }
                else // 4 walls
                {
                    waves = 4;
                    timer = 8000;
                }
                break;
            }
            case 2:
            {
                if (me->GetMap()->GetDifficultyID() == DIFFICULTY_HEROIC) // 1 walls
                {
                    waves = 1;
                    timer = 1;
                }
                else  // 2 walls
                {
                    waves = 2;
                    timer = 1;
                }
                break;
            }
            default:
                return;
            }

            int8 current_wall = urand(0, 3);
            for (uint8 i = 0; i < waves; ++i)
            {
                float x = creeping_walls[current_wall].first.GetPositionX();
                float y = creeping_walls[current_wall].first.GetPositionY();
                float z = creeping_walls[current_wall].first.GetPositionZ();
                float o = creeping_walls[current_wall].first.GetOrientation();
                uint8 number_of_npc = 0;
                uint8 position_of_passage = urand(6, 18);
                if (creeping_walls[current_wall].first.GetPositionX() == creeping_walls[current_wall].second.GetPositionX())
                {
                    for (y = creeping_walls[current_wall].first.GetPositionY(); y > creeping_walls[current_wall].second.GetPositionY(); y -= 2.5f)
                    {
                        if (++number_of_npc >= position_of_passage - 2 && number_of_npc <= position_of_passage + 2)
                            continue;

                        me->SummonCreature(100336, x, y, z, o, TEMPSUMMON_TIMED_DESPAWN, 18s);
                    }
                }
                else
                {
                    for (x = creeping_walls[current_wall].first.GetPositionX(); y > creeping_walls[current_wall].second.GetPositionX(); x -= 2.5f)
                    {
                        if (++number_of_npc >= position_of_passage - 2 && number_of_npc <= position_of_passage + 2)
                            continue;

                        me->SummonCreature(100336, x, y, z, o, TEMPSUMMON_TIMED_DESPAWN, 18s);
                    }
                }
                if (urand(0, 2) == 1)
                    current_wall = ((current_wall + 1) > 3 ? 0 : current_wall + 1);
                else
                    current_wall = ((current_wall - 1) < 0 ? 3 : current_wall - 1);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_cordanaAI(creature);
    }
};

//100525
class npc_cordana_glowing_sentry : public CreatureScript
{
public:
    npc_cordana_glowing_sentry() : CreatureScript("npc_cordana_glowing_sentry") { }

    bool OnGossipHello(Player* clicker, Creature* me)
    {
        if (me->HasAura(SPELL_ELUNES_LIGHT_VIS_1) && me->HasAura(SPELL_ELUNES_LIGHT_VIS_2))  //active?
        {
            clicker->CastSpell(clicker, SPELL_ELUNES_LIGHT_CLICK);
            clicker->CastSpell(clicker, SPELL_ELUNES_LIGHT_OVERRIDE);
            me->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_VIS_1);
            me->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_VIS_2);
        }
        return true;
    }


    struct npc_cordana_glowing_sentryAI : public ScriptedAI
    {
        npc_cordana_glowing_sentryAI(Creature* creature) : ScriptedAI(creature)
        {
            if (me->GetPositionZ() > -270.0f) // not boss
            {
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_1, true);
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_2, true);
            }
        }

        void Reset() override {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (me->GetDistance(summoner) > 80.0f)
            {
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_1, true);
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_2, true);
            }
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_ELUNES_LIGHT)
            {
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_1, true);
                DoCast(me, SPELL_ELUNES_LIGHT_VIS_2, true);

                //! hack =C
                caster->ToUnit()->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_VIS_1);
                caster->ToUnit()->RemoveAurasDueToSpell(SPELL_ELUNES_LIGHT_VIS_2);
            }
        }

        void UpdateAI(uint32 diff) override {}
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_cordana_glowing_sentryAI(creature);
    }
};

//100364
class npc_cordana_spirit_of_vengeance : public CreatureScript
{
public:
    npc_cordana_spirit_of_vengeance() : CreatureScript("npc_cordana_spirit_of_vengeance") {}

    struct npc_cordana_spirit_of_vengeanceAI : public ScriptedAI
    {
        npc_cordana_spirit_of_vengeanceAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
            doorSpirit = me->GetDistance(4234.50f, -297.59f, -281.39f) < 5.0f;
        }

        InstanceScript* instance;
        EventMap events;
        bool doorSpirit = false;

        void Reset() override
        {
            if (doorSpirit)
                instance->SetBossState(DATA_CORDANA_SPIRIT, NOT_STARTED);
            events.Reset();
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (doorSpirit)
                instance->SetBossState(DATA_CORDANA_SPIRIT, DONE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_1:
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_cordana_spirit_of_vengeanceAI(creature);
    }
};

// 100336
class npc_cordana_creeping : public CreatureScript
{
public:
    npc_cordana_creeping() : CreatureScript("npc_cordana_creeping") {}

    struct npc_cordana_creepingAI : public ScriptedAI
    {
        npc_cordana_creepingAI(Creature* creature) : ScriptedAI(creature) {}


        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            DoCast(213628);

            float x, y, z;
            me->GetClosePoint(x, y, z, me->GetCombatReach() / 3, 55.0f);
            me->GetMotionMaster()->MovePoint(1, x, y, z);

            DoCast(197415);
        }

        void SpellHitTarget(WorldObject* target, const SpellInfo* spell) override
        {
            if (spell->Id == 197506)
            {
                if (auto owner = me->GetOwner())
                    if (owner && owner->IsAIEnabled())
                        if (auto cordana = owner->ToCreature())
                            cordana->AI()->DoAction(ACTION_1);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE || id != 1)
                return;

            me->DespawnOrUnsummon();
        }

        void UpdateAI(uint32 diff) override {}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_cordana_creepingAI(creature);
    }
};


void AddSC_boss_cordana_felsong()
{
    new boss_cordana();
    new npc_cordana_glowing_sentry();
    new npc_cordana_spirit_of_vengeance();
    new npc_cordana_creeping();
}
