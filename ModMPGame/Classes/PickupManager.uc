class PickupManager extends AdminService config(ModMPGame);

#exec OBJ LOAD FILE="CTInventory.u"
#exec OBJ LOAD FILE="Properties.u"

var bool bFixedPickups;
var() config bool bAdjustSniperDamage;

//Fix any pickups that may be broken on the map, spawn in pickups that are 'missing', swap out 'noob' weapons for pistols
function FixMapPickups(){
    local Pickup P, Bacta;
    local String LevelName;

    //Dont go through this code more than once per map
    if(bFixedPickups){
        return;
    }

    //Fix pickups that dont have a respawn time set (red shotgun on Garrison), remove noob weapons
    ForEach AllActors(Class'Pickup', P){
        if(P.RespawnTime == 0)
            P.RespawnTime = 60;

        if(P.IsA('ConcussionRiflePickupMP') || P.IsA('DC17mAntiArmorWeaponPickupMP') || P.IsA('RocketLauncherPickupMP')
        || P.IsA('TrandoshanRifleMPPickup') || P.IsA('EMPGrenadeMultiPlay') || P.IsA('SonicDetonatorMultiPlay')){
            P.Transmogrify(Class'Properties.DC15PickupMP');
        }
    }
    
    //Spawn the missing red bacta in Trando base on Arena A17
    LevelName = Left(string(Level), InStr(string(Level), "."));
    if(LevelName ~= "ctf_siege" || LevelName ~= "dm_siege"){
        Bacta = Spawn(Class'CTInventory.PickupHealth', self,, vect(3975, 2139, -561));
        Bacta.RespawnTime = 45;
    }

    //Adjust sniper damage if needed
    if(bAdjustSniperDamage){
        Class'DC17mSniperAmmo'.Default.Damage = 225.0f;
        Class'DC17mSniperMP'.Default.ZoomDamageMultiplier = 1.0f;
    }

    bFixedPickups = true;
}

function PostBeginPlay(){
    Super.PostBeginPlay();

    FixMapPickups();
}

function ToggleSniperDamage(){
    local float fBaseDamage;
    local float fZoomMultiplier;

    bAdjustSniperDamage = !bAdjustSniperDamage;
    SaveConfig();

    if(bAdjustSniperDamage){
        fBaseDamage = 225.0f;
        fZoomMultiplier = 1.0f;
    }else{
        fBaseDamage = 175.0f;
        fZoomMultiplier = 1.72f;
    }

    Class'DC17mSniperAmmo'.Default.Damage = fBaseDamage;
    Class'DC17mSniperMP'.Default.ZoomDamageMultiplier = fZoomMultiplier;
    Level.Game.Broadcast(self, "Changed sniper base damage to:"@fBaseDamage@"and zoom multiplier to:"@fZoomMultiplier);
}

function bool ExecCmd(String Cmd, optional PlayerController PC){
    if(PC != None){
        if(ParseCommand(Cmd, "TOGGLESNIPERDAMAGE")){
            ToggleSniperDamage();

            return true;
        }
    }

    return false;
}

defaultproperties
{
    bFixedPickups=false
    bAdjustSniperDamage=false
    bRequiresAdminPermissions=true
}