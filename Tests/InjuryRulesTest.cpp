#include "InjuryRules.h"
#include <cstdio>
#include <cmath>
int failures = 0;
void Check(bool ok, const char* name) { if (!ok) { ++failures; std::printf("FAIL: %s\n", name); } }
int main()
{
    using namespace InjuryRules;
    State s;
    Check(s.Health == 100 && s.Tier() == 0, "healthy start");
    s.Hit(Part::Head);
    Check(s.Health == 100 && !s.IsInjured(Part::Head), "head has no damage or wound state");
    s.Reset();
    for (int i=1; i<=4; ++i) { s.Damage(25); Check(s.Tier()==i && s.Health==100-25*i, "four exact 25 boundaries"); }
    s.Reset(); s.Damage(24.99f); Check(s.Tier()==0, "no tier before full 25 loss");
    s.Reset(); s.Hit(Part::LeftHand);
    Check(s.Health==100 && s.Recoil()==1 && s.ReloadDuration()==1.5f && s.MoveSpeed()==1, "hand injury exposes reload interface only");
    s.Hit(Part::RightUpperArm); Check(s.ReloadDuration()==1.5f, "both arms use unified multiplier by default");
    s.Reset(); s.Hit(Part::RightFoot); Check(s.MoveSpeed()==0.6f && s.Recoil()==1, "foot injury uses leg penalty");
    s.Reset(); s.Hit(Part::Torso); s.Hit(Part::Torso);
    Check(s.Health==100 && s.Bleeding, "torso starts single bleed without immediate loss");
    s.BleedTick(); Check(s.Health==99, "bleed tick subtracts one point");
    for (int I=0;I<200;++I) s.BleedTick();
    Check(s.Health==1 && s.Bleeding, "bleeding has a nonlethal floor until treatment");
    s.Damage(500); Check(s.Health==0 && !s.Bleeding, "death clamps health and stops bleeding");
    s.Hit(Part::LeftFoot); Check(!s.IsInjured(Part::LeftFoot), "dead actor rejects new wounds");
    s.Reset(); s.Damage(-25); Check(s.Health==100, "negative damage is rejected");
    s.Damage(NAN); Check(s.Health==100, "nonfinite damage is rejected");
    s.Hit(Part::Unknown); Check(!s.IsInjured(Part::Unknown) && s.Health==100, "unknown region is rejected");
    s.Hit(Part::LeftCalf); s.Hit(Part::Torso); s.Reset();
    Check(s.Health==100 && s.MoveSpeed()==1 && !s.Bleeding && !s.IsInjured(Part::LeftCalf), "reset clears every state");
    std::printf("InjuryRules: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
