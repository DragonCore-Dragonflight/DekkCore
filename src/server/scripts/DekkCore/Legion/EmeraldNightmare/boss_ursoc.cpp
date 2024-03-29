#include "emerald_nightmare.h"

enum Says
{
    SAY_AGGRO = 0,
    SAY_FOCUSED_GAZE_EMOTE = 1,
    SAY_FOCUSED_GAZE = 2,
    SAY_CACOPHONY = 3,
    SAY_DEATH = 4,
    SAY_BLOOD_FRENZY = 5,
};

enum Spells
{
    SPELL_EMPTY_ENERGY = 205270,
    SPELL_FEROCIOUS_YELL = 204990,
    SPELL_ENERGIZE = 205272, //2.941176470588235
    SPELL_BERSERK = 26662,
    SPELL_OVERWHELM_AURA = 197946,
    SPELL_OVERWHELM_DUMMY = 197947,
    SPELL_OVERWHELM_DMG = 197943,
    SPELL_REND_FLESH_AURA = 197948,
    SPELL_REND_FLESH_DUMMY = 197949,
    SPELL_REND_FLESH_DMG = 197942,
    SPELL_FOCUSED_GAZE = 198006,
    SPELL_BARRELING_MOMENTUM = 198002,
    SPELL_B_MOMENTUM_FILTER = 198004,
    SPELL_B_MOMENTUM_PING = 198007,
    SPELL_B_MOMENTUM_CHARGE = 198009,
    SPELL_B_MOMENTUM_AT = 198093,
    SPELL_B_MOMENTUM_DMG = 198099,
    SPELL_B_MOMENTUM_AOE = 198109,
    SPELL_B_MOMENTUM_DUMMY = 198102,
    SPELL_MOMENTUM_DEBUFF = 198108,
    SPELL_TRAMPLING_SLAM = 199237,
    SPELL_BLOOD_FRENZY = 198388,
    SPELL_ROARING_CACOPHONY_AURA = 197986,
    SPELL_ROARING_CACOPHONY_DUMMY_1 = 197988,
    SPELL_ROARING_CACOPHONY_DUMMY_2 = 197989,
    SPELL_ROARING_CACOPHONY_DMG = 197969,
    SPELL_ECHOING_DISCHORD = 198392,

    //Heroic+
    SPELL_ROARING_CACOPHONY_SUM = 197982, //Sum NPC: 100576
    SPELL_NIGHTMARISH_CACOPHONY = 197980,
    SPELL_NIGHTMARE_IMAGE_AT = 197960, //DMG: 205611
    SPELL_NIGHTMARE_IMAGE_MORPH = 204969,
};

enum eEvents
{
    EVENT_FOCUSED_GAZE = 1,
    EVENT_REND_FLESH = 2,
    EVENT_OVERWHELM = 3,
    EVENT_ROARING_CACOPHONY = 4,
    EVENT_ROARING_CACOPHONY_2 = 5,

    EVENT_ACTIVATE_IMAGE, //Mythic
    ACTION_BARRELING_MOMENTUM,
};

enum eOther
{
    DATA_PLAYER_HITCOUNT = 1,
    DATA_MOMENTUM_TARGET = 2,

    ACTION_1,
    ACTION_2,
    ACTION_3,

    EVENT_1,
    EVENT_2,
    EVENT_3,
};

//100497
struct boss_ursoc : public BossAI
{
    boss_ursoc(Creature* creature) : BossAI(creature, DATA_URSOC) {}

    bool introDone = false;
    bool focused = false;
    bool lowHp = false;
    uint8 playerHitCount = 0;
    uint32 berserkTimer = 0;
    ObjectGuid m_TargetGUID;

    void Reset() override
    {
        _Reset();
        summons.DespawnAll();
        me->SetPower(POWER_ENERGY, 100);
        DoCast(me, SPELL_EMPTY_ENERGY, true);
        focused = false;
        lowHp = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        _JustEngagedWith(who);
        DoCast(me, SPELL_FEROCIOUS_YELL, true);
        DoCast(me, SPELL_ENERGIZE, true);

        berserkTimer = 5 * MINUTE * IN_MILLISECONDS;
        events.RescheduleEvent(EVENT_OVERWHELM, 10s);
        events.RescheduleEvent(EVENT_REND_FLESH, 13s);
        events.RescheduleEvent(EVENT_ROARING_CACOPHONY, 18s);
        Talk(SAY_AGGRO);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
    }

    void SpellHit(WorldObject* caster, const SpellInfo* spell) override
    {
        if (!me->IsInCombat())
            return;

        switch (spell->Id)
        {
        case SPELL_OVERWHELM_DUMMY:
            DoCastVictim(SPELL_OVERWHELM_DMG, true);
            break;
        case SPELL_REND_FLESH_DUMMY:
            DoCastVictim(SPELL_REND_FLESH_DMG);
            break;
        case SPELL_ROARING_CACOPHONY_DUMMY_2:
            Talk(SAY_CACOPHONY);
            me->CastSpell(me, SPELL_ROARING_CACOPHONY_DMG);
            if (IsHeroicPlusRaid())
            {
                Position pos;
                me->GetFirstCollisionPosition(0.0f, 0.0f);
                me->CastSpell(pos, SPELL_ROARING_CACOPHONY_SUM, true);
            }
            break;
        case SPELL_B_MOMENTUM_PING:
            playerHitCount = 0;
            DoCast(me, SPELL_B_MOMENTUM_AT, true);
            DoCast(caster->ToUnit(), SPELL_TRAMPLING_SLAM, true);
            //DoCast(caster, SPELL_B_MOMENTUM_CHARGE);
            //>Hack
            me->SetPower(POWER_ENERGY, 0);
            //me->GetMotionMaster()->MoveCharge(caster->GetPosition(),40.0f, SPELL_B_MOMENTUM_CHARGE);
            //<
            focused = false;
            break;
        }
    }

    void SpellHitTarget(WorldObject* target, const SpellInfo* spell) override
    {
        if (!me->IsInCombat())
            return;

        switch (spell->Id)
        {
        case SPELL_B_MOMENTUM_FILTER:
            m_TargetGUID = target->GetGUID();
            DoCast(target->ToUnit(), SPELL_FOCUSED_GAZE, true);
            DoCast(target->ToUnit(), SPELL_B_MOMENTUM_DUMMY, true);
            Talk(SAY_FOCUSED_GAZE_EMOTE, target);
            Talk(SAY_FOCUSED_GAZE);
            break;
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE)
            if (id == SPELL_B_MOMENTUM_CHARGE)
                DoCast(me, SPELL_B_MOMENTUM_AOE, true);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
    {
        if (me->HealthBelowPct(31) && !lowHp)
        {
            lowHp = true;
            DoCast(me, SPELL_BLOOD_FRENZY, true);
            Talk(SAY_BLOOD_FRENZY);
        }
    }

    void DoAction(int32 const action) override
    {
        if (action == ACTION_BARRELING_MOMENTUM && !focused)
        {
            focused = true;
            events.RescheduleEvent(EVENT_FOCUSED_GAZE, 5s);
        }
    }

    uint32 GetData(uint32 type) const override
    {
        switch (type)
        {
        case DATA_PLAYER_HITCOUNT:
            return playerHitCount;
        }
        return 0;
    }

    void SetData(uint32 type, uint32 data) override
    {
        switch (type)
        {
        case DATA_PLAYER_HITCOUNT:
            ++playerHitCount;
            break;
        default:
            break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (berserkTimer)
        {
            if (berserkTimer <= diff)
            {
                berserkTimer = 0;
                DoCast(me, SPELL_BERSERK, true);
            }
            else
                berserkTimer -= diff;
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_REND_FLESH:
                DoCast(me, SPELL_REND_FLESH_AURA, true);
                break;
            case EVENT_FOCUSED_GAZE:
                m_TargetGUID.Clear();
                DoCast(me, SPELL_B_MOMENTUM_FILTER, true);
                break;
            case EVENT_OVERWHELM:
                DoCast(me, SPELL_OVERWHELM_AURA, true);
                break;
            case EVENT_ROARING_CACOPHONY:
                DoCast(me, SPELL_ROARING_CACOPHONY_DUMMY_2, true);
                events.RescheduleEvent(EVENT_ROARING_CACOPHONY_2, 20s);
                if (IsMythicRaid())
                    events.RescheduleEvent(EVENT_ACTIVATE_IMAGE, 40s);
                break;
            case EVENT_ROARING_CACOPHONY_2:
                DoCast(me, SPELL_ROARING_CACOPHONY_AURA, true);
                break;
            case EVENT_ACTIVATE_IMAGE:
            {
                events.RescheduleEvent(EVENT_ACTIVATE_IMAGE, 40s);
                break;
            }
            }
        }
        DoMeleeAttackIfReady();
    }
};

//100576
struct npc_ursoc_nightmare_image : public ScriptedAI
{
    npc_ursoc_nightmare_image(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
        me->SetUnitFlag(UNIT_FLAG_NOT_ATTACKABLE_1);
    }

    EventMap events;

    void IsSummonedBy(WorldObject* summoner) override
    {
        DoZoneInCombat();
        DoCast(me, SPELL_NIGHTMARE_IMAGE_MORPH, true);
        events.RescheduleEvent(EVENT_3, 3s);
    }

    void Reset() override {}

    void DoAction(int32 const action) override
    {
        switch (action)
        {
        case ACTION_1:
            me->RemoveAurasDueToSpell(SPELL_NIGHTMARE_IMAGE_MORPH);
            DoCast(SPELL_NIGHTMARISH_CACOPHONY);
            events.RescheduleEvent(EVENT_1, 4s);
            break;
        case ACTION_2:
            events.RescheduleEvent(EVENT_2, 5s);
            break;
        }
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
                DoCast(me, SPELL_NIGHTMARE_IMAGE_MORPH, true);
                break;
            case EVENT_2:
                me->RemoveDynObject(SPELL_NIGHTMARE_IMAGE_AT);
                me->RemoveAurasDueToSpell(SPELL_NIGHTMARE_IMAGE_MORPH);
                me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
                me->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                me->RemoveUnitFlag(UNIT_FLAG_NOT_ATTACKABLE_1);
                me->SetReactState(REACT_AGGRESSIVE);
                break;
            case EVENT_3:
                DoCast(me, SPELL_NIGHTMARE_IMAGE_AT, true);
                break;
            }
        }
        DoMeleeAttackIfReady();
    }
};

//205272
class spell_ursoc_periodic_energize : public AuraScript
{
    PrepareAuraScript(spell_ursoc_periodic_energize);

    uint8 PowerCount;

    void OnTick(AuraEffect const* aurEff)
    {
        Creature* caster = GetCaster()->ToCreature();
        if (!caster || !caster->IsInCombat())
            return;

        PowerCount = caster->GetPower(POWER_ENERGY);

        if (PowerCount < 100)
            caster->SetPower(POWER_ENERGY, PowerCount + 3);
        else
            caster->AI()->DoAction(ACTION_BARRELING_MOMENTUM);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_ursoc_periodic_energize::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

//199237
class spell_ursoc_trampling_slam : public SpellScript
{
    PrepareSpellScript(spell_ursoc_trampling_slam);

    void DealDamage()
    {
        if (!GetCaster() || !GetHitUnit())
            return;

        float distance = GetCaster()->GetExactDist2d(GetHitUnit());

        if (distance < 100.0f)
            SetHitDamage(GetHitDamage() * (1.0f - (distance / 100.0f)));
        else
            SetHitDamage(GetHitDamage() * 0.1f);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_ursoc_trampling_slam::DealDamage);
    }
};

//198109
class spell_ursoc_barreling_impact : public SpellScript
{
    PrepareSpellScript(spell_ursoc_barreling_impact);

    void DealDamage()
    {
        if (!GetCaster() || !GetCaster()->IsAIEnabled())
            return;

        uint8 plrCount = GetCaster()->GetMap()->GetPlayersCountExceptGMs();

        if (uint8 hitCount = GetCaster()->GetAI()->GetData(DATA_PLAYER_HITCOUNT))
        {
            uint32 perc = float(float(hitCount) / float(plrCount) * 100.0f);
            if (perc >= 100)
                perc = 90;
            int32 damage = CalculatePct(GetHitDamage(), 100 - perc);
            SetHitDamage(damage);
        }
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_ursoc_barreling_impact::DealDamage);
    }
};

//198099
class spell_ursoc_momentum_filter : public SpellScript
{
    PrepareSpellScript(spell_ursoc_momentum_filter);

    void FilterTargets(WorldObject*& target)
    {
        if (Unit* caster = GetCaster())
        {
            ObjectGuid guid = caster->GetAI()->GetGUID(DATA_MOMENTUM_TARGET);
            if (Player* pTarget = ObjectAccessor::GetPlayer(*caster, guid))
            {
                if (!(Player*)target->IsInBetween(caster, pTarget, 10.0f))
                {
                    target = nullptr;
                }
            }
        }
    }

    void HandleOnHit()
    {
        if (GetCaster() && GetHitUnit())
        {
            GetCaster()->CastSpell(GetHitUnit(), SPELL_MOMENTUM_DEBUFF, true);
            GetCaster()->GetAI()->SetData(DATA_PLAYER_HITCOUNT, 1);
        }
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ursoc_momentum_filter::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_ursoc_momentum_filter::FilterTargets, EFFECT_1, TARGET_UNIT_TARGET_ENEMY);
        OnHit += SpellHitFn(spell_ursoc_momentum_filter::HandleOnHit);
    }
};

void AddSC_ursoc()
{
    RegisterCreatureAI(boss_ursoc);
    RegisterCreatureAI(npc_ursoc_nightmare_image);
    RegisterSpellScript(spell_ursoc_periodic_energize);
    RegisterSpellScript(spell_ursoc_trampling_slam);
    RegisterSpellScript(spell_ursoc_barreling_impact);
    RegisterSpellScript(spell_ursoc_momentum_filter);
}
