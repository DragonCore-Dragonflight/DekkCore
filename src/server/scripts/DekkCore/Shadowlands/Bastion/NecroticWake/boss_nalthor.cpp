#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "SpellAuraEffects.h"
#include "necrotic_wake.h"


enum Spells
{
    SPELL_FINAL_HARVEST = 321247,
    SPELL_LAND_OF_THE_DEAD = 321226,
    SPELL_NECROTIC_BOLT = 320170,
    SPELL_UNHOLY_FRENZY = 320012,
    SPELL_FINAL_HARVEST_DAMAGE = 321253,
    SPELL_LAND_OF_THE_DEAD_CAST = 321226,
    SPELL_LAND_OF_THE_DEAD_MISSILE = 319874,
    SPELL_LAND_OF_THE_DEAD_MISSILE_SECOND = 319902,
    SPELL_LAND_OF_THE_DEAD_MISSILE_THIRD = 333627,
    SPELL_NECROTIC_BREATH_TRIGGER = 337074,
    EVENT_LAND_OF_THE_DEAD = 1,
    EVENT_NECROTIC_BREATH
};

//166945
struct boss_nalthor : public BossAI
{
    boss_nalthor(Creature* creature) : BossAI(creature, DATA_AMARTH)
    {
        Vehicle* vehicle = me->GetVehicleKit();
    }

    void Reset() override
    {
        BossAI::Reset();
    }

    void JustEngagedWith(Unit* who) override
    {
        _JustEngagedWith(who);
        Talk(0);
        events.ScheduleEvent(SPELL_NECROTIC_BOLT, 3s);
        events.ScheduleEvent(EVENT_LAND_OF_THE_DEAD, 5s);
        events.ScheduleEvent(SPELL_FINAL_HARVEST, 8s);
        events.ScheduleEvent(EVENT_NECROTIC_BREATH, 10s);
    }

    void OnSpellFinished(SpellInfo const* spellInfo) override
    {
        switch (spellInfo->Id)
        {
        case SPELL_FINAL_HARVEST:
            DoCastAOE(SPELL_FINAL_HARVEST_DAMAGE, true);
            break;
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
        case SPELL_NECROTIC_BOLT:
            DoCastVictim(SPELL_NECROTIC_BOLT, false);
            events.Repeat(15s);
            break;

        case EVENT_LAND_OF_THE_DEAD:
            me->CastSpell(nullptr, SPELL_LAND_OF_THE_DEAD_CAST, false);
            me->CastSpell(me->GetRandomNearPosition(20.0f), SPELL_LAND_OF_THE_DEAD_MISSILE, true);
            me->CastSpell(me->GetRandomNearPosition(20.0f), SPELL_LAND_OF_THE_DEAD_MISSILE_SECOND, true);
            me->CastSpell(me->GetRandomNearPosition(20.0f), SPELL_LAND_OF_THE_DEAD_MISSILE_THIRD, true);
            events.Repeat(20s);
            break;

        case SPELL_FINAL_HARVEST:
            me->CastSpell(nullptr, SPELL_FINAL_HARVEST, false);
            events.Repeat(25s);
            break;

        case EVENT_NECROTIC_BREATH:
            SetCombatMovement(false);
            me->AddAura(SPELL_NECROTIC_BREATH_TRIGGER, me);
            events.Repeat(30s);
            break;
        }
    }

    void JustReachedHome() override
    {
        _JustReachedHome();
        me->RemoveAllAreaTriggers();
    }

    void JustDied(Unit* /*who*/) override
    {
        _JustDied();
        Talk(1);
        me->RemoveAllAreaTriggers();
    }
};


void AddSC_boss_nalthor()
{
    RegisterCreatureAI(boss_nalthor);
}
