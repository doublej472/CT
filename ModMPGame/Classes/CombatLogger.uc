class CombatLogger extends AdminService native;

var() config string CombatLogFile;
var() config bool   AppendCombatLog;
var() config bool   CombatLogTimeStamp;

var bool bLogging;

native final function LogEvent(coerce string Msg, name Tag);

function StartLogging() {
    bLogging = true;

    LogEvent(AdminControl.GetMapFilename(), 'Map');
    LogEvent(Level.Game.Class, 'GameMode');

    if (TAGCTFGame(Level.Game) == None) {
        LogEvent("Non TAG gamemode will lead to less detailed reports.", 'Warn');
    }

    LogTeams();

    LogEvent("Match is live", 'StartGame');
}

function LogTeams() {
    local String repOut, trandoOut;
    local GameReplicationInfo GRI;
    local array<PlayerReplicationInfo> aPRI;
    local int i;

    repOut = "";
    trandoOut = "";
    GRI = Level.Game.GameReplicationInfo;
    GRI.GetPRIArray(aPRI);
    for (i=0; i<aPRI.Length; i++){
        if (aPRI[i].Team.TeamIndex == 0) {
            repOut = repOut@"("$aPRI[i].PlayerID$","$aPRI[i].GetPlayerName()$")";
        } else if (aPRI[i].Team.TeamIndex == 1) {
            trandoOut = trandoOut@"("$aPRI[i].PlayerID$","$aPRI[i].GetPlayerName()$")";
        }
    }

    LogEvent("team_list 0 :"@repOut, 'Team');
    LogEvent("team_list 1 :"@trandoOut, 'Team');
}

cpptext 
{
    virtual void Spawned();
}

defaultproperties 
{
    CombatLogFile = "CombatEvents.log"
    AppendCombatLog=true
    CombatLogTimeStamp=true
}