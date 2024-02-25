/*
 * Copyright (C) 2020 DekkCore
 *
 * This SourceCode is NOT free a software. Please hold everything Private
 * and read our Terms
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_INFERNO_CHARGE           = 119405,
    SPELL_INFERNO_CHARGE_TRIGGERED = 119299,
    SPELL_FLAME_BREATH             = 119420
};

enum Events
{
    EVENT_FLAME_BREATH,
    EVENT_INFERNO
};

class boss_adarogg : public CreatureScript
{
    public:
        boss_adarogg() : CreatureScript("boss_adarogg") { }

        struct boss_adaroggAI : public ScriptedAI
        {
            boss_adaroggAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override { }

            void JustEngagedWith(Unit* /*who*/) override
            {
                events.ScheduleEvent(EVENT_FLAME_BREATH, 5s);
                events.ScheduleEvent(EVENT_INFERNO, 15s);
            }

            void SpellHitTarget(WorldObject* target, SpellInfo const* spell) override
            {
                if (spell->Id == SPELL_INFERNO_CHARGE)
                    me->CastSpell(target, SPELL_INFERNO_CHARGE_TRIGGERED, false);
            }

            void JustDied(Unit* /*killer*/) override { }

            void UpdateAI(uint32 const diff) override
            {
                if(!UpdateVictim())
                    return;

                events.Update(diff);

                if(me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if(uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_FLAME_BREATH:
                            DoCastVictim(SPELL_FLAME_BREATH);
                            events.ScheduleEvent(EVENT_FLAME_BREATH, 10s);
                            break;
                        case EVENT_INFERNO:
                            DoCastRandom(SPELL_INFERNO_CHARGE, 100);
                            events.ScheduleEvent(EVENT_INFERNO, 18s);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_adaroggAI(creature);
        }
};

void AddSC_boss_adarogg()
{
    new boss_adarogg();
}
