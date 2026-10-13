#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace InjuryRules
{
enum class Part { Unknown, Head, Torso, LeftUpperArm, RightUpperArm, LeftForearm, RightForearm, LeftHand, RightHand, LeftThigh, RightThigh, LeftCalf, RightCalf, LeftFoot, RightFoot, Count };
struct State
{
    float Health = 100, MaxHealth = 100;
    float ArmDamage = 0, LegDamage = 0, BleedPerTick = 1, BleedMinimumHealth = 1;
    float ArmReloadDuration = 1.5f, LegMoveSpeed = 0.6f;
    bool StackArmSides = false, StackLegSides = false;
    bool Bleeding = false;
    std::array<bool, static_cast<int>(Part::Count)> Wounds{};
    static bool Valid(Part p) { return p > Part::Unknown && p < Part::Count; }
    static bool Arm(Part p) { return p >= Part::LeftUpperArm && p <= Part::RightHand; }
    static bool Leg(Part p) { return p >= Part::LeftThigh && p <= Part::RightFoot; }
    int Tier() const { return std::clamp(static_cast<int>(std::floor((MaxHealth-Health)/25.0f)), 0, 4); }
    bool IsInjured(Part p) const { return Valid(p) && p!=Part::Head && Wounds[static_cast<int>(p)]; }
    bool Any(Part first, Part last) const
    {
        for (int i=static_cast<int>(first); i<=static_cast<int>(last); ++i) if (Wounds[i]) return true;
        return false;
    }
    void Hit(Part p)
    {
        if (!Valid(p) || p==Part::Head || Health<=0) return;
        Wounds[static_cast<int>(p)]=true;
        if (p==Part::Torso) Bleeding=true;
        else if (Arm(p)) Damage(ArmDamage);
        else if (Leg(p)) Damage(LegDamage);
    }
    void Damage(float amount)
    {
        if (!std::isfinite(amount) || amount<=0 || Health<=0) return;
        Health=std::clamp(Health-amount, 0.0f, MaxHealth);
        if (Health<=0) Bleeding=false;
    }
    void BleedTick()
    {
        // 仅流血受下限约束；直接伤害仍可致死。低于下限时也不能凭空回血。
        const float Floor=std::clamp(BleedMinimumHealth,1.f,MaxHealth);
        if (Bleeding && Health>Floor && std::isfinite(BleedPerTick) && BleedPerTick>0)
            Health=std::max(Floor,Health-BleedPerTick);
    }
    void Reset() { Health=MaxHealth; Bleeding=false; Wounds.fill(false); }
    bool LeftArm() const { return IsInjured(Part::LeftUpperArm) || IsInjured(Part::LeftForearm) || IsInjured(Part::LeftHand); }
    bool RightArm() const { return IsInjured(Part::RightUpperArm) || IsInjured(Part::RightForearm) || IsInjured(Part::RightHand); }
    bool LeftLeg() const { return IsInjured(Part::LeftThigh) || IsInjured(Part::LeftCalf) || IsInjured(Part::LeftFoot); }
    bool RightLeg() const { return IsInjured(Part::RightThigh) || IsInjured(Part::RightCalf) || IsInjured(Part::RightFoot); }
    float Recoil() const { return 1; } // 兼容旧查询；新规格只计算换弹与移速。
    float ReloadDuration() const { return LeftArm() || RightArm() ? (StackArmSides && LeftArm() && RightArm() ? ArmReloadDuration*ArmReloadDuration : ArmReloadDuration) : 1; }
    float ReloadSpeed() const { return 1.f/ReloadDuration(); }
    float MoveSpeed() const { return LeftLeg() || RightLeg() ? (StackLegSides && LeftLeg() && RightLeg() ? LegMoveSpeed*LegMoveSpeed : LegMoveSpeed) : 1; }
};
}
