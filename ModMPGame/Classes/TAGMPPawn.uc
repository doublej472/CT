class TAGMPPawn extends MPClone;

function Died(Controller Killer, class<DamageType> damageType, vector HitLocation, optional name BoneName ) {

    Log("@@@@RAV@@@@ I AM DEAD");

    Super.Died(Killer, damageType, HitLocation, name);
}

function PossessedBy(Controller C) {
    Super.PossessedBy(C);

    Log("@@@@RAV@@@@ Possessed by Controller: " @C);
}