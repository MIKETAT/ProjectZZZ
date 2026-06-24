#include "Utility/ZZZGameplayTag.h"

namespace Combat::ActionTag
{
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Attack, FName(TEXTVIEW("Combat.ActionTag.Enemy_Attack")))
	
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_1, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_1")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_2, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_2")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_3, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_3")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_4, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_4")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_5, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_5")))
	UE_DEFINE_GAMEPLAY_TAG(Basic_Attack_6, FName(TEXTVIEW("Combat.ActionTag.Basic_Attack_6")))

	UE_DEFINE_GAMEPLAY_TAG(Special_Attack, FName(TEXTVIEW("Combat.ActionTag.Special_Attack")))
	UE_DEFINE_GAMEPLAY_TAG(Special_Attack_EX, FName(TEXTVIEW("Combat.ActionTag.Special_Attack_EX")))
	
	UE_DEFINE_GAMEPLAY_TAG(Ultimate, FName(TEXTVIEW("Combat.ActionTag.Ultimate")))
	
	UE_DEFINE_GAMEPLAY_TAG(Switch_In, FName(TEXTVIEW("Combat.ActionTag.Switch_In")))
	UE_DEFINE_GAMEPLAY_TAG(Switch_Out, FName(TEXTVIEW("Combat.ActionTag.Switch_Out")))
	UE_DEFINE_GAMEPLAY_TAG(Chain_Attack, FName(TEXTVIEW("Combat.ActionTag.Chain_Attack")))
	
	UE_DEFINE_GAMEPLAY_TAG(Dodge, FName(TEXTVIEW("Combat.ActionTag.Dodge")))
	UE_DEFINE_GAMEPLAY_TAG(Rush_Attack, FName(TEXTVIEW("Combat.ActionTag.Rush_Attack")))

	UE_DEFINE_GAMEPLAY_TAG(Quick_Assist, FName(TEXTVIEW("Combat.ActionTag.Quick_Assist")))
	UE_DEFINE_GAMEPLAY_TAG(Defensive_Assist, FName(TEXTVIEW("Combat.ActionTag.Defensive_Assist")))

	UE_DEFINE_GAMEPLAY_TAG(HitReaction_Front, FName(TEXTVIEW("Combat.ActionTag.HitReaction_Front")))
	UE_DEFINE_GAMEPLAY_TAG(HitReaction_Back, FName(TEXTVIEW("Combat.ActionTag.HitReaction_Back")))
}

namespace Team
{
	UE_DEFINE_GAMEPLAY_TAG(Agent, FName(TEXTVIEW("Team.Agent")))
	UE_DEFINE_GAMEPLAY_TAG(Enemy, FName(TEXTVIEW("Team.Enemy")))
}

namespace Combat::Gait
{
	UE_DEFINE_GAMEPLAY_TAG(Idle, FName(TEXTVIEW("Combat.Gait.Idle")))
	UE_DEFINE_GAMEPLAY_TAG(Run, FName(TEXTVIEW("Combat.Gait.Run")))
	UE_DEFINE_GAMEPLAY_TAG(Sprint, FName(TEXTVIEW("Combat.Gait.Sprint")))
	UE_DEFINE_GAMEPLAY_TAG(Dodge, FName(TEXTVIEW("Combat.Gait.Dodge")))
}

namespace Combat::Status::Agent
{
	UE_DEFINE_GAMEPLAY_TAG(Dodge, FName(TEXTVIEW("Combat.Status.Agent.Dodge")))
	UE_DEFINE_GAMEPLAY_TAG(Parry, FName(TEXTVIEW("Combat.Status.Agent.Parry")))
}

namespace Combat::Status::Enemy
{
	UE_DEFINE_GAMEPLAY_TAG(Stunned, FName(TEXTVIEW("Combat.Status.Enemy.Stunned")))
}

namespace Combat::CombatWindows
{
	UE_DEFINE_GAMEPLAY_TAG(InputBufferWindow, FName(TEXTVIEW("Combat.Window.InputBuffer")))
	UE_DEFINE_GAMEPLAY_TAG(ProceedWindow, FName(TEXTVIEW("Combat.Window.ProceedWindow")))
	UE_DEFINE_GAMEPLAY_TAG(IsRecoveryWindow, FName(TEXTVIEW("Combat.Window.IsRecoveryWindow")))
	UE_DEFINE_GAMEPLAY_TAG(ParryWindow, FName(TEXTVIEW("Combat.Window.ParryWindow")))
	UE_DEFINE_GAMEPLAY_TAG(MovementInterruptWindow, FName(TEXTVIEW("Combat.Window.MovementInterruptWindow")))
}

namespace Combat::Data
{
	UE_DEFINE_GAMEPLAY_TAG(DamageMultiplier, FName(TEXTVIEW("Combat.Data.DamageMultiplier")))
	UE_DEFINE_GAMEPLAY_TAG(DazeMultiplier, FName(TEXTVIEW("Combat.Data.DazeMultiplier")))
	UE_DEFINE_GAMEPLAY_TAG(StunDMGMultiplier, FName(TEXTVIEW("Combat.Data.StunDMGMultiplier")))
}

namespace Combat::Event
{
	UE_DEFINE_GAMEPLAY_TAG(Death, FName(TEXTVIEW("Combat.Event.Death")));
	UE_DEFINE_GAMEPLAY_TAG(ParrySucceed, FName(TEXTVIEW("Combat.Event.ParrySucceed")));
	UE_DEFINE_GAMEPLAY_TAG(ChainAttack, FName(TEXTVIEW("Combat.Event.ChainAttack")));
	UE_DEFINE_GAMEPLAY_TAG(PerfectAssist, FName(TEXTVIEW("Combat.Event.PerfectAssist")));
	UE_DEFINE_GAMEPLAY_TAG(QuickAssist, FName(TEXTVIEW("Combat.Event.QuickAssist")));
}

namespace Combat::SpecialAction
{
	UE_DEFINE_GAMEPLAY_TAG(ChainAttack, FName(TEXTVIEW("Combat.SpecialAction.ChainAttack")))
	UE_DEFINE_GAMEPLAY_TAG(QuickAssist, FName(TEXTVIEW("Combat.SpecialAction.QuickAssist")))
	UE_DEFINE_GAMEPLAY_TAG(DefensiveAssist, FName(TEXTVIEW("Combat.SpecialAction.DefensiveAssist")))
	UE_DEFINE_GAMEPLAY_TAG(Ultimate, FName(TEXTVIEW("Combat.SpecialAction.Ultimate")))
}

namespace Combat::Camera::Status
{
	UE_DEFINE_GAMEPLAY_TAG(CombatFollowCamera, FName(TEXTVIEW("Combat.Camera.Status.CombatFollowCamera")))
	UE_DEFINE_GAMEPLAY_TAG(HighSpeedMovementCamera, FName(TEXTVIEW("Combat.Camera.Status.HighSpeedMovementCamera")))
	UE_DEFINE_GAMEPLAY_TAG(ChainAttackCamera, FName(TEXTVIEW("Combat.Camera.Status.ChainAttackCamera")))
	UE_DEFINE_GAMEPLAY_TAG(QuickAssistCamera, FName(TEXTVIEW("Combat.Camera.Status.QuickAssistCamera")))
	UE_DEFINE_GAMEPLAY_TAG(UltimateCamera, FName(TEXTVIEW("Combat.Camera.Status.UltimateCamera")))
	UE_DEFINE_GAMEPLAY_TAG(ParryCamera, FName(TEXTVIEW("Combat.Camera.Status.ParryCamera")))
}


namespace AI::BlackBoard
{
	UE_DEFINE_GAMEPLAY_TAG(IsStunned, FName(TEXTVIEW("AI.BlackBoard.IsStunned")))
}
