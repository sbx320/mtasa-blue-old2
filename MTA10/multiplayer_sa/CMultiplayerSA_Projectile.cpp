#include "StdInc.h"

void HOOK_CProjectileInfo_Update_FindLocalPlayer_FindLocalPlayerVehicle ( );
void HOOK_CProjectile_FixTearGasCrash ( );
void HOOK_CProjectile_FixExplosionLocation ( );
void HOOK_CProjectileInfo__AddProjectile ( );
void HOOK_CProjectile__CProjectile ( );

ProjectileHandler* m_pProjectileHandler = NULL;
ProjectileStopHandler* m_pProjectileStopHandler = NULL;

void CMultiplayerSA::InitHooks_Projectile ( )
{
    m_pProjectileHandler = NULL;
    m_pProjectileStopHandler = NULL;

    const DWORD HOOKPOS_CProjectileInfo_FindPlayerPed = 0x739321;
    HookInstallCall ( HOOKPOS_CProjectileInfo_FindPlayerPed, (DWORD) HOOK_CProjectileInfo_Update_FindLocalPlayer_FindLocalPlayerVehicle );
    const DWORD HOOKPOS_CProjectileInfo_FindPlayerVehicle = 0x739570;
    HookInstallCall ( HOOKPOS_CProjectileInfo_FindPlayerVehicle, (DWORD) HOOK_CProjectileInfo_Update_FindLocalPlayer_FindLocalPlayerVehicle );

    // Fix for a crash when teargas is created by a vehicle
    const DWORD HOOKPOS_CProjectile_FixTearGasCrash = 0x4C0403;
    HookInstall ( HOOKPOS_CProjectile_FixTearGasCrash, (DWORD) HOOK_CProjectile_FixTearGasCrash, 6 );

    // Fix for projectiles going too far "into" a wall due to collisions being processed too slow
    const DWORD HOOKPOS_CProjectile_FixExplosionLocation = 0x738A77;
    HookInstall ( HOOKPOS_CProjectile_FixExplosionLocation, (DWORD) HOOK_CProjectile_FixExplosionLocation, 12 );

    // Hook for CProjectileInfo::AddProjectile (Pre-Creation Handler)
    const DWORD HOOKPOS_CProjectileInfo__AddProjectile = 0x737C80;
    HookInstall ( HOOKPOS_CProjectileInfo__AddProjectile, (DWORD) HOOK_CProjectileInfo__AddProjectile, 7 );

    // Hook for CProjectile::CProjectile (Post-Creation for above hook, but before application of velocity etc)
    const DWORD HOOKPOS_CProjectile__CProjectile = 0x5A40D1;
    HookInstall ( HOOKPOS_CProjectile__CProjectile, (DWORD) HOOK_CProjectile__CProjectile, 6 );
}

void CMultiplayerSA::SetProjectileHandler ( ProjectileHandler * pProjectileHandler )
{
    m_pProjectileHandler = pProjectileHandler;
}

void CMultiplayerSA::SetProjectileStopHandler ( ProjectileStopHandler * pProjectileHandler )
{
    m_pProjectileStopHandler = pProjectileHandler;
}

//
//  Fix for projectiles firing too fast locally when created by a remote player
//    This is caused by SA increasing the velocity of non-player created projectiles compared to the default
//    We fix this by making SA think the creator is always the local player
//
void _declspec( naked ) HOOK_CProjectileInfo_Update_FindLocalPlayer_FindLocalPlayerVehicle ( )
{
    // 00739559 E8 B2 4C E3 FF                          call    FindPlayerPed < HOOK >
    // 00739570 E8 5B 4B E3 FF                          call    FindPlayerVehicle < HOOK >
    // Checks if the creator is the local player ped or the creator is the local player peds vehicle else decreases the velocity substantially.
    // We are forcing it to think the creator is not the local player ped or his vehicle for this specific check
    _asm
    {
        xor eax, eax
            retn
    }
}

//
//  Fix for a crash when teargas is created by a vehicle
//
DWORD RETURN_CProjectile_FixTearGasCrash_Fix = 0x4C05B9;
DWORD RETURN_CProjectile_FixTearGasCrash_Cont = 0x4C0409;


void _declspec( naked ) HOOK_CProjectile_FixTearGasCrash ( )
{
    _asm
    {
        cmp ebp, 0h
        je cont
        mov ecx, [ebp + 47Ch]
        // no terminators in this time period
        jmp RETURN_CProjectile_FixTearGasCrash_Cont
cont:
        // come with me if you want to live
        jmp RETURN_CProjectile_FixTearGasCrash_Fix
        // dundundundundun
        // dundundundundun
    }
}


//
//  Fix for projectiles going too far "into" a wall due to collisions being processed too slow
// 
CPhysicalSAInterface * pExplosionEntity;
DWORD   RETURN_CProjectile_FixExplosionLocation = 0x738A86;
void UpdateExplosionLocation ( )
{
    if ( pExplosionEntity )
    {
        // project backwards 20% of our velocity just to catch us going too far
        CVector vecStart = pExplosionEntity->Placeable.matrix->vPos + ( pExplosionEntity->m_vecLinearVelocity * 0.20f );
        // project forwards 120% to look for collisions forwards
        CVector vecEnd = vecStart - ( pExplosionEntity->m_vecLinearVelocity * 1.20f );
        // calculate our actual impact position
        if ( pGameInterface->GetWorld ( )->CalculateImpactPosition ( vecStart, vecEnd ) )
        {
            // Apply it
            if ( pExplosionEntity->Placeable.matrix )
            {
                pExplosionEntity->Placeable.matrix->vPos = vecEnd;
            }
            else
            {
                pExplosionEntity->Placeable.m_transform.m_translate = vecEnd;
            }
        }
    }
}

void _declspec( naked ) HOOK_CProjectile_FixExplosionLocation ( )
{
    _asm
    {
        mov pExplosionEntity, esi
            pushad
    }
    UpdateExplosionLocation ( );
    _asm
    {
        popad
            mov eax, [esi + 14h]
            test eax, eax
            jz skip
            add eax, 30h
            jmp RETURN_CProjectile_FixExplosionLocation
skip :
        lea eax, [esi + 4]
            jmp RETURN_CProjectile_FixExplosionLocation
    }
}


// 
// Hook for CProjectileInfo::AddProjectile
//  Used to allow Core to abort cancel a projectile
// 
DWORD RETURN_CProjectile__AddProjectile = 0x401C3D;

CVector * projectileOrigin = NULL;
eWeaponType projectileWeaponType = WEAPONTYPE_UNARMED;
float projectileForce = 0.0f;
CVector * projectileTarget = NULL;
CEntity* projectileTargetEntity = NULL;
CEntitySAInterface * pProjectileOwner = NULL;
CEntitySAInterface * projectileTargetEntityInterface = NULL;

bool ProcessProjectileAdd ( )
{
    if ( m_pProjectileStopHandler )
    {
        CPools * pPools = pGameInterface->GetPools ( );

        CEntity * pOwner = pPools->GetEntity ( (DWORD*) pProjectileOwner );
        projectileTargetEntity = pPools->GetEntity ( (DWORD*) projectileTargetEntityInterface );

        return m_pProjectileStopHandler ( pOwner, projectileWeaponType, projectileOrigin, projectileForce, projectileTarget, projectileTargetEntity );
    }
    return true;
}

// CProjectileInfo::AddProjectile(class CEntity * owner,enum eWeaponType weaponType,
//                                class CVector origin?,float fForce?,class CVector * targetPos,class CEntity * target)
void _declspec( naked ) HOOK_CProjectileInfo__AddProjectile ( )
{
    _asm
    {
        mov     edx, [esp + 4]
        mov     pProjectileOwner, edx

        mov     edx, [esp + 8]
        mov     projectileWeaponType, edx

        lea     edx, [esp + 12]
        mov     projectileOrigin, edx

        mov     edx, [esp + 24]
        mov     projectileForce, edx

        mov     edx, [esp + 28]
        mov     projectileTarget, edx

        mov     edx, [esp + 32]
        mov     projectileTargetEntityInterface, edx

        pushad
    }
    if ( ProcessProjectileAdd ( ) )
    { 
        // Create the projectile
        _asm
        {
            popad
            push    0xFFFFFFFF
            mov     edx, RETURN_CProjectile__AddProjectile
            jmp     edx
        }
    }
    else
    {
        // Do not create the projectile
        _asm
        {
            popad
            xor al, al
            retn
        }
    }
}


// 
// Hook for CProjectileInfo::AddProjectile
//  Used to allow Core to be notified about a projectile creation
//  At this point (default) velocity, origin, target etc. are not yet applied
// 
DWORD RETURN_CProjectile__CProjectile = 0x4037B3;
class CProjectileSAInterface * pProjectile = NULL;
DWORD dwProjectileInfoIndex;

void ProcessProjectile ( )
{
    if ( m_pProjectileHandler != NULL )
    {
        CPoolsSA * pPools = (CPoolsSA*) pGameInterface->GetPools ( );
        CEntity * pOwner = pPools->GetEntity ( (DWORD*) pProjectileOwner );
        CProjectileInfo * projectileInfo = pGameInterface->GetProjectileInfo ( )->GetProjectileInfo ( dwProjectileInfoIndex );
        CProjectile* projectile = pGameInterface->GetProjectileInfo ( )->GetProjectile ( pProjectile );
        projectile->SetProjectileInfo ( projectileInfo );
        m_pProjectileHandler ( pOwner, projectile, projectileInfo, projectileWeaponType, projectileOrigin, projectileForce, projectileTarget, projectileTargetEntity );
        projectileTargetEntity = NULL;
    }
}

void _declspec( naked ) HOOK_CProjectile__CProjectile ( )
{
    _asm
    {
        mov     dwProjectileInfoIndex, ebx // it happens to be in here, luckily
        mov     pProjectile, eax
        pushad
    }

    ProcessProjectile ( );

    _asm
    {
        popad
        add esp, 0x10
        retn 4
    }
}
