class HitMarkers extends AdminService native config(ModMPGame);

event SendHitMessage(PlayerController target, string victim, string part, FLOAT damage){
	target.ClientMessage("You hit " $ victim $ " in the " $ part $ " for " $ damage $ "!");
}

cpptext
{
	struct FHitEntry{
		FLOAT LastHit;
		FLOAT LastHealth;
		FLOAT LastShield;
	};
	static TMap<FString, FHitEntry> LastHitByPlayerID;

	// Overrides
	virtual INT Tick(FLOAT DeltaTime, ELevelTick TickType);
	virtual void Spawned();
}

defaultproperties
{
}
