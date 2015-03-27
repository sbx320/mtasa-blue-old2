/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*               (Shared logic for modifications)
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/shared_logic/CClientProjectile.cpp
*  PURPOSE:     Projectile entity class
*  DEVELOPERS:  Jax <>
*               Ed Lyons <eai@opencoding.net>
*
*****************************************************************************/

#include <StdInc.h>

#define AIRBOMB_MAX_LIFETIME 60000



CClientProjectile::CClientProjectile ( CClientManager* pManager,
                    CClientEntity * pCreator,
                    CClientEntity * pTarget,
                    eWeaponType weaponType,
                    const CVector& vecOrigin,
                    const CVector& vecVelocity,
                    const CVector& vecRotation,
                    float fForce,
                    unsigned short usModel,
                    bool bLocal )
    : ClassInit ( this ), CClientStreamElement ( pManager->GetProjectileStreamer ( ), INVALID_ELEMENT_ID )
{
    CClientEntityRefManager::AddEntityRefs ( ENTITY_REF_DEBUG ( this, "CClientProjectile" ), &m_pCreator, &m_pTarget, NULL );

    m_pManager = pManager;
    m_pProjectileManager = pManager->GetProjectileManager ( );
    m_pProjectile = nullptr;
    m_pProjectileInfo = nullptr;

    SetTypeName ( "projectile" );

    m_pCreator = pCreator;
    m_pTarget = pTarget;
    m_weaponType = weaponType;

    m_vecRotation = vecRotation;
    m_vecPosition = vecOrigin;
    m_vecVelocity = vecVelocity;

    m_dwCounter = 5000;

    if ( usModel == 0 )
        usModel = CClientPickupManager::GetWeaponModel ( m_weaponType );

    m_usModel = usModel;

    m_fForce = fForce;
    m_bLocal = bLocal;
    m_llCreationTime = GetTickCount64_ ( );

    m_pProjectileManager->AddToList ( this );
    m_bLinked = true;

    UpdateStreamPosition ( m_vecPosition );

    if ( pCreator )
    {
        switch ( pCreator->GetType ( ) )
        {
            case CCLIENTPLAYER:
            case CCLIENTPED:
                static_cast <CClientPed *> ( pCreator )->AddProjectile ( this );
                break;
            case CCLIENTVEHICLE:
                static_cast <CClientVehicle *> ( pCreator )->AddProjectile ( this );
                break;
            default: break;
        }
    }
}

CClientProjectile::CClientProjectile ( class CClientManager* pManager, CProjectile* pProjectile, CProjectileInfo* pProjectileInfo, CClientEntity * pCreator, CClientEntity * pTarget, eWeaponType weaponType, CVector * pvecOrigin, CVector * pvecTarget, float fForce, bool bLocal )
    : ClassInit ( this ), CClientStreamElement ( pManager->GetProjectileStreamer ( ), INVALID_ELEMENT_ID )
{
    CClientEntityRefManager::AddEntityRefs ( ENTITY_REF_DEBUG ( this, "CClientProjectile" ), &m_pCreator, &m_pTarget, NULL );

    m_pManager = pManager;
    m_pProjectileManager = pManager->GetProjectileManager ( );
    m_pProjectile = pProjectile;
    m_pProjectileInfo = pProjectileInfo;

    SetTypeName ( "projectile" );

    m_pCreator = pCreator;
    m_pTarget = pTarget;
    m_weaponType = weaponType;

    GetRotationDegrees ( m_vecRotation );
    GetPosition ( m_vecPosition );

    m_usModel = pProjectile->GetModelIndex ( );

    m_fForce = fForce;
    m_bLocal = bLocal;
    m_llCreationTime = GetTickCount64_ ( );

    /* Since this is a GTA-created projectile we've already been streamed in */
    m_bStreamedIn = true;

    m_pProjectileManager->AddToList ( this );
    m_bLinked = true;

    if ( pCreator )
    {
        switch ( pCreator->GetType ( ) )
        {
            case CCLIENTPLAYER:
            case CCLIENTPED:
                static_cast < CClientPed * > ( pCreator )->AddProjectile ( this );
                break;
            case CCLIENTVEHICLE:
                static_cast < CClientVehicle * > ( pCreator )->AddProjectile ( this );
                break;
            default: break;
        }
    }
}


CClientProjectile::~CClientProjectile ( void )
{
    // If our creator is getting destroyed, this should be null
    if ( m_pCreator )
    {
        switch ( m_pCreator->GetType ( ) )
        {
            case CCLIENTPLAYER:
            case CCLIENTPED:
                static_cast < CClientPed * > ( (CClientEntity*) m_pCreator )->RemoveProjectile ( this );
                break;
            case CCLIENTVEHICLE:
                static_cast < CClientVehicle * > ( (CClientEntity*) m_pCreator )->RemoveProjectile ( this );
                break;
            default: break;
        }
    }

    Unlink ( );

    if ( m_pProjectile )
    {
        // Make sure we're destroyed
        delete m_pProjectile;
        m_pProjectile = NULL;
    }

    CClientEntityRefManager::RemoveEntityRefs ( 0, &m_pCreator, &m_pTarget, NULL );
}


void CClientProjectile::Unlink ( void )
{
    // Are we still linked? (this bool will be set to false when our manager is being destroyed)
    if ( m_bLinked )
    {
        m_pProjectileManager->RemoveFromList ( this );
        m_bLinked = false;
    }
}


void CClientProjectile::DoPulse ( void )
{
    if ( IsStreamedIn ( ) )
    {
        // Reset position/velocity if we are frozen
        if ( m_bFrozen )
        {
            CVector vecTemp;
            m_pProjectile->SetMatrix ( &m_matFrozen );
            m_pProjectile->SetMoveSpeed ( &vecTemp );
            m_pProjectile->SetTurnSpeed ( &vecTemp );
        }
        else
        {
            // Projectile might be moving
            CVector vecPosition;
            GetPosition ( vecPosition );
            UpdateStreamPosition ( vecPosition );
        }
    }
    else
    {
        // Streamed out and supposed to be gone? 
        if ( m_dwCounter <= 0 && GetWeaponType ( ) != eWeaponType::WEAPONTYPE_REMOTE_SATCHEL_CHARGE )
        {
            CElementDeleter* pElementDeleter = g_pClientGame->GetElementDeleter ( );
            pElementDeleter->Delete ( this );
        }
            
    }


    // Update our position/rotation if we're attached
    DoAttaching ( );

    if ( m_bCorrected == false &&
         m_pProjectile != NULL &&
         GetWeaponType ( ) == eWeaponType::WEAPONTYPE_REMOTE_SATCHEL_CHARGE )
    {
        m_bCorrected = m_pProjectile->CorrectPhysics ( );
    }
}

bool CClientProjectile::IsActive ( void )
{
    // Ensure airbomb is cleaned up
    if ( m_weaponType == WEAPONTYPE_FREEFALL_BOMB && GetTickCount64_ ( ) - m_llCreationTime > AIRBOMB_MAX_LIFETIME )
        return false;
    return ( m_pProjectile && m_pProjectileInfo->IsActive ( ) );
}


bool CClientProjectile::GetMatrix ( CMatrix & matrix ) const
{
    if ( m_pProjectile )
    {
        if ( m_pProjectile->GetMatrix ( &matrix ) )
        {
            // Jax: If the creator is a ped, we need to invert X and Y on Direction and Was for CMultiplayer::ConvertMatrixToEulerAngles
            if ( m_pCreator && IS_PED ( m_pCreator ) )
            {
                matrix.vFront.fX = 0.0f - matrix.vFront.fX;
                matrix.vFront.fY = 0.0f - matrix.vFront.fY;
                matrix.vUp.fX = 0.0f - matrix.vUp.fX;
                matrix.vUp.fY = 0.0f - matrix.vUp.fY;
            }
            return true;
        }
    }
    return false;
}


bool CClientProjectile::SetMatrix ( const CMatrix & matrix_ )
{
    CMatrix matrix ( matrix_ );

    // Jax: If the creator is a ped, we need to invert X and Y on Direction and Was for CMultiplayer::ConvertEulerAnglesToMatrix
    if ( m_pCreator && IS_PED ( m_pCreator ) )
    {
        matrix.vFront.fX = 0.0f - matrix.vFront.fX;
        matrix.vFront.fY = 0.0f - matrix.vFront.fY;
        matrix.vUp.fX = 0.0f - matrix.vUp.fX;
        matrix.vUp.fY = 0.0f - matrix.vUp.fY;
    }

    if ( m_pProjectile )
        m_pProjectile->SetMatrix ( &matrix );
    return true;
}


void CClientProjectile::GetPosition ( CVector & vecPosition ) const
{
    if ( m_pProjectile )
        vecPosition = *m_pProjectile->GetPosition ( );
    else
        vecPosition = m_vecPosition;
}


void CClientProjectile::SetPosition ( const CVector & vecPosition )
{
    if ( m_pProjectile )
        m_pProjectile->SetPosition ( const_cast < CVector* > ( &vecPosition ) );

    m_vecPosition = vecPosition;
}


void CClientProjectile::GetRotationRadians ( CVector & vecRotation ) const
{
    CMatrix matrix;
    GetMatrix ( matrix );
    g_pMultiplayer->ConvertMatrixToEulerAngles ( matrix, vecRotation.fX, vecRotation.fY, vecRotation.fZ );
}

void CClientProjectile::GetRotationDegrees ( CVector & vecRotation ) const
{
    GetRotationRadians ( vecRotation );
    ConvertRadiansToDegrees ( vecRotation );
}


void CClientProjectile::SetRotationRadians ( const CVector & vecRotation )
{
    CMatrix matrix;
    GetPosition ( matrix.vPos );
    g_pMultiplayer->ConvertEulerAnglesToMatrix ( matrix, vecRotation.fX, vecRotation.fY, vecRotation.fZ );
    SetMatrix ( matrix );
}

void CClientProjectile::SetRotationDegrees ( const CVector & vecRotation )
{
    CVector vecTemp = vecRotation;
    ConvertDegreesToRadians ( vecTemp );
    SetRotationRadians ( vecTemp );
}


void CClientProjectile::GetVelocity ( CVector & vecVelocity )
{
    if ( m_pProjectile )
        m_pProjectile->GetMoveSpeed ( &vecVelocity );
    else
        vecVelocity = m_vecVelocity;
}


void CClientProjectile::SetVelocity ( CVector & vecVelocity )
{
    if ( m_pProjectile )
        m_pProjectile->SetMoveSpeed ( &vecVelocity );

    m_vecVelocity = vecVelocity;
}

unsigned short CClientProjectile::GetModel ( void )
{
    if ( m_pProjectile )
        return m_pProjectile->GetModelIndex ( );
    return m_usModel;
}

void CClientProjectile::SetModel ( unsigned short usModel )
{
    if ( m_pProjectile )
        m_pProjectile->SetModelIndex ( usModel );

    m_usModel = usModel;
}

void CClientProjectile::SetCounter ( DWORD dwCounter )
{
    if ( m_pProjectile )
        m_pProjectileInfo->SetCounter ( dwCounter );

    m_dwCounter = dwCounter;
}

DWORD CClientProjectile::GetCounter ( void )
{
    if ( m_pProjectile )
        return m_pProjectileInfo->GetCounter ( );
    return m_dwCounter;
}

CClientEntity* CClientProjectile::GetSatchelAttachedTo ( void )
{
    if ( !m_pProjectile )
        return NULL;

    CEntity* pAttachedToSA = m_pProjectile->GetAttachedEntity ( );

    if ( !pAttachedToSA )
        return NULL;

    return m_pManager->FindEntity ( pAttachedToSA, false );
}


void CClientProjectile::StreamIn ( bool bInstantly )
{
    if ( m_pProjectile )
    {
        // This happens upon instantiation of a CClientProjectile for an already existing SA Projectile
        // Do not call NotifyCreate here
        return;
    }
    else
        Create ( );
}



void CClientProjectile::StreamOut ( )
{
    // We should have a projectile here
    dassert ( m_pProjectile );

    GetPosition ( m_vecPosition );
    GetRotationDegrees ( m_vecRotation );

    m_pProjectile->GetMoveSpeed ( &m_vecVelocity );

    // Do not explode when streaming out
    Destroy ( false );
}


void CClientProjectile::Create ( void )
{
    if ( !m_pProjectileManager->Create ( this ) )
        return NotifyUnableToCreate ( );

    if ( m_vecPosition != CVector ( 0, 0, 0 ) )
        m_pProjectile->Teleport ( m_vecPosition.fX, m_vecPosition.fY, m_vecPosition.fZ );
    m_pProjectile->SetModelIndex ( m_usModel );
    m_pProjectile->SetOrientation ( m_vecRotation.fX, m_vecRotation.fY, m_vecRotation.fZ );

    if ( m_vecVelocity != CVector ( 0, 0, 0 ) )
    m_pProjectile->SetMoveSpeed ( &m_vecVelocity );
    m_pProjectileInfo->SetActive ( true );

    // +1 to avoid projectiles never exploding
    if ( m_dwCounter != 0 )
    {
        m_pProjectileInfo->SetCounter ( m_dwCounter );
    }

    if ( m_pCreator )
        m_pProjectileInfo->SetCreator ( m_pCreator->GetGameEntity ( ) );
    if ( m_pTarget )
        m_pProjectileInfo->SetTarget ( m_pTarget->GetGameEntity ( ) );
    m_pProjectileInfo->SetWeaponType ( m_weaponType );

    NotifyCreate ( );
}


void CClientProjectile::Destroy ( bool bBlow )
{
    if ( m_pProjectile )
    {
        m_pProjectile->Destroy ( bBlow );
        delete m_pProjectile;
        m_pProjectile = NULL;
        m_pProjectileInfo->SetActive ( false );
        m_pProjectileInfo = NULL;
    }
}


void CClientProjectile::SetFrozen ( bool bFrozen )
{
    if ( bFrozen != m_bFrozen )
    {
        if ( m_pProjectile )
        {
            CVector vecTemp;
            m_pProjectile->GetMatrix ( &m_matFrozen );
            m_pProjectile->SetMoveSpeed ( &vecTemp );
            m_pProjectile->SetTurnSpeed ( &vecTemp );
        }
    }
    m_bFrozen = bFrozen;
}

bool CClientProjectile::IsFrozen ( )
{
    return m_bFrozen;
}
