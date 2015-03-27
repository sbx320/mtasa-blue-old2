/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*               (Shared logic for modifications)
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/shared_logic/CClientProjectileManager.cpp
*  PURPOSE:     Projectile entity manager class
*  DEVELOPERS:  Jax <>
*               Cecill Etheredge <ijsf@gmx.net>
*
*****************************************************************************/

#include "StdInc.h"

using std::list;

CClientProjectileManager * g_pProjectileManager = NULL;

/* This class is used to manage/control projectiles created from game-layer hooks.
   Projectile creation should eventually be server-requested.
*/
CClientProjectileManager::CClientProjectileManager ( CClientManager * pManager )
{
    CClientEntityRefManager::AddEntityRefs ( ENTITY_REF_DEBUG ( this, "CClientProjectileManager" ), &m_pCreator, &m_pLastCreated, NULL );

    g_pProjectileManager = this;
    m_pManager = pManager;

    m_bIsLocal = false;
    m_pCreator = NULL;
    m_pStreamingIn = NULL;

    m_bCreating = false;
    m_pLastCreated = NULL;
}


CClientProjectileManager::~CClientProjectileManager ( void )
{
    RemoveAll ();
    if ( g_pProjectileManager == this ) g_pProjectileManager = NULL;
    CClientEntityRefManager::RemoveEntityRefs ( 0, &m_pCreator, &m_pLastCreated, NULL );
}


void CClientProjectileManager::DoPulse ( void )
{
    CElementDeleter* pElementDeleter = g_pClientGame->GetElementDeleter();
    CClientProjectile* pProjectile = NULL;
    list < CClientProjectile* > cloneList = m_List;

    for ( auto pProjectile : cloneList )
    {
        pProjectile->DoPulse ( );

        // Streamed out and supposed to be gone? 
        if ( pProjectile->GetCounter ( ) <= 0 && !pProjectile->IsStreamedIn ( ) )
            pElementDeleter->Delete ( pProjectile );
    }
}


void CClientProjectileManager::RemoveAll ( void )
{
    list < CClientProjectile * > cloneList = m_List;

    for ( auto pProjectile : cloneList )
    {
        delete pProjectile;
    }

    m_List.clear ();
}


bool CClientProjectileManager::Exists ( CClientProjectile * pProjectile )
{
    for ( auto pIter : m_List )
    {
        if ( pIter == pProjectile )
        {
            return true;
        }
    }
    return false;
}

CClientProjectile* CClientProjectileManager::Get ( CEntitySAInterface * pProjectile )
{
    for ( auto pIter : m_List )
    {
        if ( pIter->GetGameEntity ( ) != NULL && 
             pIter->GetGameEntity ( )->GetInterface ( ) == pProjectile )
        {
            return pIter;
        }
    }
    return NULL;
}


unsigned int CClientProjectileManager::CountStreamedIn ( )
{
    unsigned int count = 0;
    for (auto pProjectile : m_List )
    {
        if ( pProjectile->IsStreamedIn ( ) )
            count++;
    }
    return count;
}


bool CClientProjectileManager::IsProjectileLimitReached ( )
{
    // We use 31 here rather than 32 (the actual limit) to allow GTA to create a projectile at any time
    return g_pProjectileManager->CountStreamedIn ( ) >= 31;
}


void CClientProjectileManager::RemoveFromList ( CClientProjectile* pProjectile )
{
    m_List.remove ( pProjectile );
}


bool CClientProjectileManager::Hook_StaticProjectileAllow ( CEntity * pGameCreator )
{
    return g_pProjectileManager->Hook_ProjectileAllow ( pGameCreator );
}


bool CClientProjectileManager::Hook_ProjectileAllow ( CEntity * pGameCreator )
{
    // Called before projectile creation, we need to decide to allow or cancel it here

    // Is this a local projectile?
    CClientPlayer * pLocalPlayer = m_pManager->GetPlayerManager ()->GetLocalPlayer ();

    // Store the creator/local variables for the next stage
    m_pCreator = m_pManager->FindEntity ( pGameCreator, true );
    m_bIsLocal = ( m_pCreator == pLocalPlayer || ( pLocalPlayer->GetOccupiedVehicleSeat () == 0 && m_pCreator == pLocalPlayer->GetOccupiedVehicle () ) ); 

    return ( m_bCreating || m_bIsLocal || m_pStreamingIn);
}


void CClientProjectileManager::Hook_StaticProjectileCreation ( CProjectile* pGameProjectile, CProjectileInfo* pProjectileInfo, eWeaponType weaponType, CVector * origin, float fForce, CVector * pvecTargetPos, CEntity * pGameTarget )
{
    g_pProjectileManager->Hook_ProjectileCreation ( pGameProjectile, pProjectileInfo, weaponType, origin, fForce, pvecTargetPos, pGameTarget );
}


void CClientProjectileManager::Hook_ProjectileCreation ( CProjectile* pGameProjectile, CProjectileInfo* pProjectileInfo, eWeaponType weaponType, CVector * origin, float fForce, CVector * pvecTargetPos, CEntity * pGameTarget )
{
    // Called on projectile construction 
    if ( m_pStreamingIn != NULL )
    {
        // If we are creating or streaming in, do not create a new projectile element
        m_pStreamingIn->m_pProjectile = pGameProjectile;
        m_pStreamingIn->m_pProjectileInfo = pProjectileInfo;
        return;
    }
    else
    {
        // SA created a projectile, so let's create an element for it
        CClientEntity * pTarget = m_pManager->FindEntity ( pGameTarget, true );
        auto pProjectile = new CClientProjectile ( m_pManager, pGameProjectile, pProjectileInfo, m_pCreator, pTarget, weaponType, origin, pvecTargetPos, fForce, m_bIsLocal );
        g_pClientGame->ProjectileInitiateHandler ( pProjectile );
    }
}

CClientProjectile * CClientProjectileManager::Create ( CClientEntity* pCreator, 
                                                       eWeaponType eWeapon,
                                                       CVector & vecOrigin,
                                                       CVector& vecVelocity,
                                                       CVector& vecRotation,
                                                       float fForce, 
                                                       CClientEntity * pTargetEntity, 
                                                       unsigned short usModel )
{
    CClientPlayer * pLocalPlayer = m_pManager->GetPlayerManager ( )->GetLocalPlayer ( );
    bool bLocal = ( pCreator == pLocalPlayer || ( pLocalPlayer->GetOccupiedVehicleSeat ( ) == 0 && m_pCreator == pLocalPlayer->GetOccupiedVehicle ( ) ) );
    CClientProjectile *pProjectile = new CClientProjectile ( m_pManager, pCreator, pTargetEntity, eWeapon, vecOrigin, vecVelocity, vecRotation, fForce, usModel, bLocal );
    return pProjectile;
}

bool CClientProjectileManager::Create ( CClientProjectile * pProjectile )
{
    m_pStreamingIn = pProjectile;
    auto pCreator = pProjectile->GetCreator ( );
    // Peds and players
    if ( pCreator->GetType ( ) == CCLIENTPED || pCreator->GetType ( ) == CCLIENTPLAYER )
    {
        CPed * pPed = dynamic_cast <CPed *> ( pCreator->GetGameEntity ( ) );
        if ( pPed ) pPed->AddProjectile ( pProjectile->GetWeaponType(), CVector(), 0.0f, NULL, NULL );
    }
    // Vehicles
    else if ( pCreator->GetType ( ) == CCLIENTVEHICLE )
    {
        CVehicle * pVehicle = dynamic_cast <CVehicle *> ( pCreator->GetGameEntity() );
        if ( pVehicle ) pVehicle->AddProjectile ( pProjectile->GetWeaponType ( ), CVector ( ), 0.0f, NULL, NULL );
    }
    m_pStreamingIn = NULL;

    return true;
}
