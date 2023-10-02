class MatchManager extends AdminService native;

#exec OBJ LOAD FILE="RAS_Music.uax"

var int nNumReadyPlayers;    //The number of players who are ready
var bool bReadyCheckActive;  //A ready check is currently active

native final function SetPlayerReadyState(PlayerController PC, bool IsReady);
native final function bool GetPlayerReadyState(PlayerController PC);

//Checks if a player should be considered as someone who must ready up
function bool IsActivePlayer(PlayerController PC){
    local PlayerReplicationInfo PRI;

    PRI = PC.PlayerReplicationInfo;
    if(PRI == None || PRI.bOnlySpectator || PRI.Team == None){
        return false;
    }

    return true;
}

function RecalculateReadyPlayers(){
    local PlayerController PC;

	nNumReadyPlayers = 0;

    ForEach DynamicActors(Class'PlayerController', PC){
        if(NetConnection(PC.Player) != None)
            if(IsActivePlayer(PC) && GetPlayerReadyState(PC))
				nNumReadyPlayers += 1;
    }
}

event PlayerDisconnectEvent(PlayerController PC){
    RecalculateReadyPlayers();
}

//Gets the total count of players who need to ready up to begin the match
event int GetActivePlayerCount(){
    local int nCount;
    local PlayerController PC;

    nCount = 0;
    ForEach DynamicActors(Class'PlayerController', PC){
        if(NetConnection(PC.Player) != None)
            if(IsActivePlayer(PC))
                nCount += 1;
    }

    return nCount;
}

event PrintActivePlayerReadyState(){
    local PlayerController PC;

    ForEach DynamicActors(Class'PlayerController', PC){
        if(NetConnection(PC.Player) != None)
            if(IsActivePlayer(PC) && !GetPlayerReadyState(PC))
                Level.Game.Broadcast(self, PC.PlayerReplicationInfo.PlayerName@"is not ready!");
    }

	Level.Game.Broadcast(self, nNumReadyPlayers $ "/" $ GetActivePlayerCount() $ " are ready!");
}

//Begins the ready check
function StartReadyCheck(PlayerController PC){
    local PlayerController P;
    local Sound ReadySound;

    ReadySound = Sound'RAS_Music.RAS_BattleCues.musRAS_battle02_lp';
    ForEach DynamicActors(Class'PlayerController', P){
        P.ClientPlaySoundLocally(ReadySound);
		SetPlayerReadyState(P, false);
    }

	nNumReadyPlayers = 0;
    bReadyCheckActive = true;
    SetPlayerReadyState(PC, true);
}

//Cancels an in-progress ready check
function CancelReadyCheck(PlayerController PC){
  local PlayerController P;

  if(bReadyCheckActive){
    ForEach DynamicActors(Class'PlayerController', P){
        if(NetConnection(P.Player) != None){
            SetPlayerReadyState(P, false);
        }
    }

    nNumReadyPlayers = 0;
    bReadyCheckActive = false;
  }
}

//Shuffle teams randomly (written by RAV)
function ShufflePlayers(){
    local int randomTeamNum, shuffleTeamSize, repSize, trandoSize, i;
    local PlayerController PC;
    local array<PlayerController> ActivePCList;
    local TDGame pTDGame;

    pTDGame = TDGame(Level.Game);
    repSize = 0;
    trandoSize = 0;

    if(pTDGame != None){
        ForEach DynamicActors(Class'PlayerController', PC)
            if(IsActivePlayer(PC))
                ActivePCList[ActivePCLIst.length] = PC;

        shuffleTeamSize = ActivePCList.length / 2;

        for(i = 0; i < ActivePCList.length; i++){
            PC = ActivePCList[i];
            if(repSize == shuffleTeamSize){
                pTDGame.ChangeTeam(PC, 1, true);
            }else if(trandoSize == shuffleTeamSize){
                pTDGame.ChangeTeam(PC, 0, true);
            }else{
                randomTeamNum = Rand(2);
                pTDGame.ChangeTeam(PC, randomTeamNum, true);

                if(randomTeamNum == 0)
                    repSize++;
                else
                    trandoSize++;
            }

            PC.Suicide();
        }
    }
}

//Reset state of everything to begin match without changing maps
event LiveReset(){
    local int i;
    local PlayerController PC;
    local Projectile Proj;
    local TDGame pTDGame;

    //Destroy any grenades or other projectiles in the air
    ForEach DynamicActors(Class 'Projectile', Proj){
        Proj.Destroy();
    }

    //Reset the stats on the PRIs and FPlayerStats structs
    ForEach DynamicActors(Class'PlayerController', PC){
        PC.PlayerReplicationInfo.Deaths = 0;
        PC.PlayerReplicationInfo.Score = 0;
        PC.PlayerReplicationInfo.Kills = 0;
        PC.PlayerReplicationInfo.GoalsScored = 0;
        PC.PlayerReplicationInfo.StartTime = 0;
    }
    AdminControl.ResetAllStats();

    //Reset the team scores if this is a team-based game mode
    pTDGame = TDGame(Level.Game);
    if(pTDGame != None){
        pTDGame.RestartEverybody();
        for (i = 0; i < ArrayCount(pTDGame.Teams); i++){
            pTDGame.Teams[i].Score = 0;
        }
    }

    //Play the "Go! Go! Go!" sound for each player
    ForEach DynamicActors(Class'PlayerController', PC){
        if(NetConnection(PC.Player) != None && PC.PlayerReplicationInfo != None){
            if(PC.PlayerReplicationInfo.Team.TeamIndex == 1)
                PC.ClientPlaySoundLocally(Sound'MP_Voice.Trando_Merc.TMCZZ012');
            else
                PC.ClientPlaySoundLocally(Sound'MP_Voice.Delta_40.D40ZZ025');
        }
    }

    //Reset game elapsed time, set the respawn time, and broadcast the live message
    Level.Game.GameReplicationInfo.ElapsedTime = 0;
    Level.Game.ConsoleCommand("set mpgame respawnwaittime" @ GetActivePlayerCount() + 2);
    Level.Game.Broadcast(self, "Game is Live!");
}

function bool ExecCmd(String Cmd, optional PlayerController PC){
    if(PC != None){
        if(ParseCommand(Cmd, "READY")){
            if(bReadyCheckActive){
                CommandFeedback(PC, "There is already an active ready check!");
            }
            else if(PC.PlayerReplicationInfo.bAdmin){
                StartReadyCheck(PC);
                Level.Game.Broadcast(self, PC.PlayerReplicationInfo.PlayerName@"has started a ready check! ("$nNumReadyPlayers$"/"$GetActivePlayerCount()$")");
            }else{
                CommandFeedback(PC, "You are not logged in!", true);
            }

            return true;
        }else if(ParseCommand(Cmd, "CANCEL")){
            if(!bReadyCheckActive){
                CommandFeedback(PC, "There is no active ready check!");
            }
            else if(PC.PlayerReplicationInfo.bAdmin){
                CancelReadyCheck(PC);
                Level.Game.Broadcast(self, PC.PlayerReplicationInfo.PlayerName@"has cancelled the ready check!");
            }else{
                CommandFeedback(PC, "You are not logged in!", true);
            }

            return true;
        }else if(ParseCommand(Cmd, "RESET")){
            if(PC.PlayerReplicationInfo.bAdmin){
                LiveReset();
            }else{
                CommandFeedback(PC, "You are not logged in!", true);
            }

            return true;
        }else if(ParseCommand(Cmd, "YES")){
            if(!bReadyCheckActive){
                CommandFeedback(PC, "There is no active ready check!");
            }else if(GetPlayerReadyState(PC)){
                CommandFeedback(PC, "You are already ready!");
            }else if(!IsActivePlayer(PC)){
                CommandFeedback(PC, "You need to be an active player to ready up!");
            }else{
                Level.Game.Broadcast(self, PC.PlayerReplicationInfo.PlayerName @ "is ready! (" $ nNumReadyPlayers + 1 $ "/" $ GetActivePlayerCount() $ ")");
                SetPlayerReadyState(PC, true);
            }

            return true;
        }else if(ParseCommand(Cmd, "NO")){
            if(!bReadyCheckActive){
                CommandFeedback(PC, "There is no active ready check!");
            }else if(!GetPlayerReadyState(PC)){
                CommandFeedback(PC, "You are already not ready!");
            }else{
                Level.Game.Broadcast(self, PC.PlayerReplicationInfo.PlayerName @ "is not ready! (" $ nNumReadyPlayers - 1 $ "/" $ GetActivePlayerCount() $ ")");
                SetPlayerReadyState(PC, false);
            }

            return true;
        }else if(ParseCommand(Cmd, "ISREADY")){
            if(!bReadyCheckActive){
                CommandFeedback(PC, "There is no active ready check!");
            }else{
                PrintActivePlayerReadyState();
            }

            return true;
        }else if(ParseCommand(Cmd, "SHUFFLE")){
            if(PC.PlayerReplicationInfo.bAdmin){
                ShufflePlayers();
                CommandFeedback(PC, "The teams have been shuffled!");
            }else{
                CommandFeedback(PC, "You are not logged in!", true);
            }

            return true;
        }
    }

    return false;
}

cpptext
{
    void TryFinishReadyCheck();
}

defaultproperties
{
    bRequiresAdminPermissions=false
}
