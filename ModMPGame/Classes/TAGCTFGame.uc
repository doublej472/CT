class TAGCTFGame extends CTFGame;

var AdminControl AdminControl;

event PostLogin( PlayerController NewPlayer ) {
    Super.PostLogin(NewPlayer);

	
    if (AdminControl == None) {
        AdminControl = AdminControl(Level.Game.GameStats);
    }

	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        AdminControl.CombatLogger.LogEvent(NewPlayer.PlayerReplicationInfo.PlayerID@NewPlayer.PlayerReplicationInfo.PlayerName, 'Join');
    }
}

function PostBeginPlay() {
	Super.PostBeginPlay();
}

function ChangeName( Controller Other, coerce string S, bool bNameChange ) {
    local PlayerReplicationInfo PRI;
    Super.ChangeName(Other, S, bNameChange);

    if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        PRI = Other.PlayerReplicationInfo;
        AdminControl.CombatLogger.LogEvent(PRI.PlayerID@PRI.OldName@PRI.PlayerName, 'Name');
    }
}

function KillEvent(string Killtype, PlayerReplicationInfo Killer, PlayerReplicationInfo Victim, class<DamageType> Damage) {
    // local Pawn VictimPawn;
    // local MPPlayerReplicationInfo KillerPRI;
    // local PlayerController KillerPC;
    // local Controller VictimPC;

    Super.KillEvent(Killtype, Killer, Victim, Damage);

    // KillerPRI = MPPlayerReplicationInfo(Killer);
    // KillerPC = PlayerController(Killer.Owner);
    // VictimPC = Controller(Victim.Owner);
    // VictimPawn = VictimPC.Pawn;
    // if (KillerPRI != None) {
    //     if (VictimPawn.LastHitBone == 'head') {
    //         KillerPRI.headcount++;
    //     }

    //     if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
    //             AdminControl.CombatLogger.LogEvent(Killer.PlayerID@Victim.PlayerID@VictimPawn.LastHitBone@Killer.Score, 'Kill');
    //         }
    //     }

    // KillerPC.ClientMessage('You now have'@KillerPRI.headcount@"headshots out of"@KillerPRI.Kills + 1@"kills.");
}

function ScoreKill(Controller Killer, Controller Other) {
	local Pawn VictimPawn;
	local MPPlayerReplicationInfo KillerMPPRI;
	local PlayerReplicationInfo KillerPRI, VictimPRI;
	local PlayerController KillerPC;
	
	Super.ScoreKill(Killer, Other);

    KillerPC = PlayerController(Killer);
	KillerPRI = KillerPC.PlayerReplicationInfo;
	VictimPRI = Other.PlayerReplicationInfo;
	KillerMPPRI = MPPlayerReplicationInfo(KillerPRI);
    VictimPawn = Other.Pawn;
    if (KillerPRI != None) {
        if (VictimPawn.LastHitBone == 'head') {
            KillerMPPRI.headcount++;
        }

        if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
                AdminControl.CombatLogger.LogEvent(KillerPRI.PlayerID@VictimPRI.PlayerID@VictimPawn.LastHitBone@KillerPRI.Score, 'Kill');
            }
        }

    KillerPC.ClientMessage('You now have'@KillerMPPRI.headcount@"headshots out of"@KillerMPPRI.Kills@"kills.");
}

function ScoreFlag(Controller Scorer, GameObject theFlag)
{
	//local float Dist,oppDist;
	//local int i;
	//local float ppp,numtouch;
	//local vector FlagLoc;

	if ( Scorer.PlayerReplicationInfo.Team == Teams[theFlag.TeamNum] )
	{
		Scorer.PlayerReplicationInfo.Score += 2;
		ScoreEvent(Scorer.PlayerReplicationInfo,2,"flag_ret_friendly");
        if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
            AdminControl.CombatLogger.LogEvent("flag_ret_friendly_2"@Scorer.PlayerReplicationInfo.PlayerID, 'Flag');
        }

		/*
		FlagLoc = TheFlag.Position().Location;
		Dist = vsize(FlagLoc - TheFlag.HomeBase.Location);

		if (TheFlag.TeamNum==0)
			oppDist = vsize(FlagLoc - Teams[1].HomeBase.Location);
		else
			oppDist = vsize(FlagLoc - Teams[0].HomeBase.Location); 

		GameEvent("flag_returned",""$theFlag.TeamNum,Scorer.PlayerReplicationInfo);
		BroadcastLocalizedMessage( class'CTFMessage', 1, Scorer.PlayerReplicationInfo, None, Teams[TheFlag.TeamNum] );

		if (Dist>1024)
		{
			// figure out who's closer

			if (Dist<=oppDist)	// In your team's zone
			{
				Scorer.PlayerReplicationInfo.Score += 3;
				ScoreEvent(Scorer.PlayerReplicationInfo,3,"flag_ret_friendly");
			}
			else
			{
				Scorer.PlayerReplicationInfo.Score += 5;
				ScoreEvent(Scorer.PlayerReplicationInfo,5,"flag_ret_enemy");

				if (oppDist<=1024)	// Denial
				{
					Scorer.PlayerReplicationInfo.Score += 7;
					ScoreEvent(Scorer.PlayerReplicationInfo,7,"flag_denial");
				}

			}					
		} 
		*/
		return;
	}

	/* TODO: Add back in First Touch Scoring
	// Figure out Team based scoring.
	if (TheFlag.FirstTouch!=None)	// Original Player to Touch it gets 5
	{
		ScoreEvent(TheFlag.FirstTouch.PlayerReplicationInfo,5,"flag_cap_1st_touch");
		TheFlag.FirstTouch.PlayerReplicationInfo.Score += 5;
	}
	*/

	// Guy who caps gets 5
	Scorer.PlayerReplicationInfo.Score += 5;
	IncrementGoalsScored(Scorer.PlayerReplicationInfo);
    if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
            AdminControl.CombatLogger.LogEvent("flag_cap_final_5"@Scorer.PlayerReplicationInfo.PlayerID, 'Flag');
	}
    

	/* TODO: Add back in Assist
	// Each player gets 20/x but it's guarenteed to be at least 1 point but no more than 5 points 
	numtouch=0;	
	for (i=0;i<TheFlag.Assists.length;i++)
	{
		if (TheFlag.Assists[i]!=None)
			numtouch = numtouch + 1.0;
	}

	ppp = FClamp(20/numtouch,1,5);

	for (i=0;i<TheFlag.Assists.length;i++)
	{
		if (TheFlag.Assists[i]!=None)
		{
			ScoreEvent(TheFlag.Assists[i].PlayerReplicationInfo,ppp,"flag_cap_assist");
			TheFlag.Assists[i].PlayerReplicationInfo.Score += int(ppp);
		}
	}
	*/

	// Apply the team score
	Scorer.PlayerReplicationInfo.Team.Score += 1.0;
	ScoreEvent(Scorer.PlayerReplicationInfo,5,"flag_cap_final");
	TeamScoreEvent(Scorer.PlayerReplicationInfo.Team.TeamIndex,1,"flag_cap");	
	GameEvent("flag_captured",""$theflag.TeamNum,Scorer.PlayerReplicationInfo);

	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
            AdminControl.CombatLogger.LogEvent("flag_captured"@1 - theflag.TeamNum@Scorer.PlayerReplicationInfo.Team.Score, 'Flag');
	}

	BroadcastLocalizedMessage( class'CTFMessage', 0, Scorer.PlayerReplicationInfo, None, Teams[TheFlag.TeamNum] );
	AnnounceScore(Scorer.PlayerReplicationInfo.Team.TeamIndex);
	CheckScore(Scorer.PlayerReplicationInfo);

	if ( bOverTime )
	{
		EndGame(Scorer.PlayerReplicationInfo,"timelimit");
	}
}

function EndGame(PlayerReplicationInfo Winner, string Reason ) {
	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        AdminControl.CombatLogger.LogEvent(Reason, 'EndGame');
    }

    Super.EndGame(Winner, Reason);
}

function Logout(Controller Exiting) {
	Super.Logout(Exiting);

	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        AdminControl.CombatLogger.LogEvent(Exiting.PlayerReplicationInfo.PlayerID, 'Leave');
    }
}

function bool ChangeTeam(Controller Other, int num, bool bNewTeam, optional bool bSwitch) {
	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        AdminControl.CombatLogger.LogEvent("team_swap"@Other.PlayerReplicationInfo.PlayerID@num, 'Team');
		AdminControl.CombatLogger.LogTeams();
    }

	return Super.ChangeTeam(Other, num, bNewTeam, bSwitch);
}

function ProcessServerTravel( string URL, bool bItems ) {
	if (AdminControl.CombatLogger != None && AdminControl.CombatLogger.bLogging) {
        AdminControl.CombatLogger.LogEvent("mapchange", 'EndGame');
    }

	Super.ProcessServerTravel(URL, bItems);
}