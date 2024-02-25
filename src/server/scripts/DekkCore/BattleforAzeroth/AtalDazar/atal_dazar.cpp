/*
 * Copyright 2021
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

#include "ScriptMgr.h"
#include "atal_dazar.h"
#include "ScriptedCreature.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "InstanceScript.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"
#include "MotionMaster.h"

enum eSpells
{
    SPELL_TIKI_BLAZE = 257689,
};

//Npc 127315
struct npc_reanimation_totem : public ScriptedAI
{
    npc_reanimation_totem(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        me->SetReactState(REACT_PASSIVE);
        SetCombatMovement(false);
        ScriptedAI::InitializeAI();
    }

    void Reset() override
    {
        ScriptedAI::Reset();
    }
    void DamageTaken(Unit* /*who*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (damage >= me->GetHealth())
        {
            damage = me->GetHealth() - 1;
            std::list<Creature*> zombies;
            zombies = me->FindNearestCreatures(127757, 20.0f);
            for (auto npc : zombies)
            {
                npc->RemoveAura(255968);
                npc->RemoveAura(256956);
                npc->AddAura(256960, npc);
            }
            me->KillSelf();
        }
    }

    void UpdateAI(uint32 diff) override
    {
        ScriptedAI::UpdateAI(diff);
    }

};
//129985
struct npc_tiki_mask : public ScriptedAI
{
    npc_tiki_mask(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        ScriptedAI::InitializeAI();
    }

    void Reset() override
    {
        ScriptedAI::Reset();
    }

    void UpdateAI(uint32 diff) override
    {
        ScriptedAI::UpdateAI(diff);
    }

};

//129985
struct npc_zandalar_witch : public ScriptedAI
{
    npc_zandalar_witch(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        ScriptedAI::InitializeAI();
    }

    void Reset() override
    {
        ScriptedAI::Reset();
    }

    void UpdateAI(uint32 diff) override
    {
        ScriptedAI::UpdateAI(diff);
    }
};

//npc 126734
struct npc_mob_echo_of_shadra : public ScriptedAI
{
    npc_mob_echo_of_shadra(Creature* pCreature) : ScriptedAI(pCreature) { }

    void Reset() override
    {
        events.Reset();
        me->setActive(false);
        me->SetWalk(false);
        me->GetMotionMaster()->MoveRandom(urand(0, 15));

        //need to check the chance
        int chance = urand(0, 3);
        if (chance == 2)
            me->AddAura(250013, me);
    }

    void InitializeAI() override
    {
        me->SetReactState(REACT_PASSIVE);
        SetCombatMovement(false);
        ScriptedAI::InitializeAI();
    }

    void DamageTaken(Unit* /*who*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
    }

    void MovementInform(uint32 /*type*/, uint32 /*id*/) override
    {
        me->GetMotionMaster()->MoveRandom(urand(5, 15));
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (Player* closestplayer = me->SelectNearestPlayer(50))
            if (me->GetDistance(closestplayer) < 1)
            {
                me->CastSpell(me, 258611);
                me->KillSelf();
            }
    }
};

void OpenGate(InstanceScript* instance)
{
    if (GameObject* go = instance->instance->GetGameObject(instance->GetGuidData(GO_GATE_001)))
        go->SetGoState(GO_STATE_ACTIVE);
    if (GameObject* go = instance->instance->GetGameObject(instance->GetGuidData(GO_GATE_002)))
        go->SetGoState(GO_STATE_ACTIVE);
    if (GameObject* go = instance->instance->GetGameObject(instance->GetGuidData(GO_GATE_003)))
        go->SetGoState(GO_STATE_ACTIVE);
    if (GameObject* go = instance->instance->GetGameObject(instance->GetGuidData(GO_GATE_004)))
        go->SetGoState(GO_STATE_ACTIVE);
};

class go_ad_switch : public GameObjectScript {
public:
    go_ad_switch() : GameObjectScript("go_ad_switch") { }

    struct go_ad_switchAI : public GameObjectAI
    {
        go_ad_switchAI(GameObject* go) : GameObjectAI(go) { }
    };
    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_ad_switchAI(go);
    }

    void OnGameObjectStateChanged(GameObject* go, uint32 state) override
    {
        if (state == GO_STATE_ACTIVE)
            OpenGate(go->GetInstanceScript());
    }
};

struct go_waterfall_stairs : public GameObjectAI
{
    go_waterfall_stairs(GameObject* go) : GameObjectAI(go) { }

    void Reset() override
    {
        me->GetScheduler().CancelAll();
        me->GetScheduler().Schedule(1s, [this] (TaskContext context)
        {
            if (InstanceScript* instance = me->GetInstanceScript())
            {
                if (instance->GetBossState(DATA_PRIESTESS_ALUNZA) == DONE && instance->GetBossState(DATA_VOLKAAL) == DONE && instance->GetBossState(DATA_REZAN) == DONE)
                {
                    me->SetGoState(GO_STATE_ACTIVE);
                    if (GameObject* colision = me->GetInstanceScript()->instance->GetGameObject(me->GetInstanceScript()->GetGuidData(GO_COLLISION_WALL)))
                colision->Delete();
                }
                context.Repeat(1s);
            }
        });
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
    }

private:
    TaskScheduler scheduler;
};

//256960
class spell_rooting_decay : public AuraScript
{
    PrepareAuraScript(spell_rooting_decay);

    void OnPeriodic(AuraEffect const* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
            caster->CastSpell(caster, 256959);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rooting_decay::OnPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
    }
};

void AddSC_atal_dazar()
{
    new go_ad_switch();
    RegisterGameObjectAI(go_waterfall_stairs);
    RegisterCreatureAI(npc_mob_echo_of_shadra);
    RegisterCreatureAI(npc_reanimation_totem);
    RegisterSpellScript(spell_rooting_decay);
}
