/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*               (Shared logic for modifications)
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/shared_logic/CClientProjectile.h
*  PURPOSE:     Projectile entity class header
*  DEVELOPERS:  Jax <>
*               Ed Lyons <eai@opencoding.net>
*
*****************************************************************************/

class CClientProjectile;

#ifndef __CCLIENTPROJECTILE_H
#define __CCLIENTPROJECTILE_H

#include "CClientEntity.h"
#include "CClientCommon.h"

class CProjectile;
class CProjectileInfo;
class CClientEntity;
class CVector;
enum eWeaponType;
class CClientProjectileManager;
class CClientPed;
class CClientVehicle;


class CClientProjectile : public CClientStreamElement
{
    DECLARE_CLASS( CClientProjectile, CClientStreamElement )
    friend class CClientProjectileManager;
    friend class CClientPed;
    friend class CClientVehicle;
public:
                                        /* SA Constructor, should be cleaned up fixme: sbx320 */
                                        CClientProjectile ( class CClientManager* pManager,
                                                                  CProjectile* pProjectile,
                                                                  CProjectileInfo* pProjectileInfo,
                                                                  CClientEntity * pCreator,
                                                                  CClientEntity * pTarget,
                                                                  eWeaponType weaponType,
                                                                  CVector * pvecOrigin,
                                                                  CVector * pvecTarget,
                                                                  float fForce,
                                                                  bool bLocal );
                                        /* MTA Constructor */
                                        CClientProjectile ( class CClientManager* pManager,
                                                            CClientEntity * pCreator,
                                                            CClientEntity * pTarget,
                                                            eWeaponType weaponType,
                                                            const CVector& vecOrigin,
                                                            const CVector& vecVelocity,
                                                            const CVector& vecRotation,
                                                            float fForce,
                                                            unsigned short usModel,
                                                            bool bLocal );

                                        ~CClientProjectile      ( void );

    eClientEntityType                   GetType                 ( void ) const                      { return CCLIENTPROJECTILE; }
    inline CEntity*                     GetGameEntity           ( void )                            { return m_pProjectile; }
    void                                Unlink                  ( void );


    void                                DoPulse                 ( void );
    
    bool                                IsActive                ( void );
    bool                                GetMatrix               ( CMatrix & matrix ) const;
    bool                                SetMatrix               ( const CMatrix & matrix );
    void                                GetPosition             ( CVector & vecPosition ) const;
    void                                SetPosition             ( const CVector & vecPosition );
    void                                GetRotationRadians      ( CVector & vecRotation ) const;
    void                                GetRotationDegrees      ( CVector & vecRotation ) const;
    void                                SetRotationRadians      ( const CVector & vecRotation );
    void                                SetRotationDegrees      ( const CVector & vecRotation );
    void                                GetVelocity             ( CVector & vecVelocity );
    void                                SetVelocity             ( CVector & vecVelocity );
    unsigned short                      GetModel                ( void );
    void                                SetModel                ( unsigned short usModel );
    void                                SetCounter              ( DWORD dwCounter );
    DWORD                               GetCounter              ( void );
    inline CClientEntity *              GetCreator              ( void )        { return m_pCreator; }
    inline CClientEntity *              GetTargetEntity         ( void )        { return m_pTarget; }
    inline eWeaponType                  GetWeaponType           ( void )        { return m_weaponType; }
    inline float                        GetForce                ( void )        { return m_fForce; }
    inline bool                         IsLocal                 ( void )        { return m_bLocal; }
    CClientEntity*                      GetSatchelAttachedTo    ( void );

    void                                SetFrozen ( bool bFrozen );
    bool                                IsFrozen ( );

    void                                StreamIn                ( bool bInstantly );
    void                                StreamOut               ( void );

    void                                Create                  ( void );
    void                                Destroy                 ( bool bBlow = true );

    void                                StreamedInPulse         ( void );
    inline bool                         StreamingOut            ( void ) { return m_bStreamingOut; }
protected:
    CClientProjectileManager*           m_pProjectileManager;
    bool                                m_bLinked;

    CProjectile *                       m_pProjectile;
    CProjectileInfo *                   m_pProjectileInfo;

    CClientEntityPtr                    m_pCreator;
    CClientEntityPtr                    m_pTarget;
    eWeaponType                         m_weaponType;
    float                               m_fForce;
    bool                                m_bLocal;
    long long                           m_llCreationTime;
    bool                                m_bCorrected;

    CVector                             m_vecPosition;
    CVector                             m_vecRotation;
    CVector                             m_vecVelocity;
    
    unsigned short                      m_usModel;
    DWORD                               m_dwCounter;
    bool                                m_bStreamingOut;
    bool                                m_bFrozen;
    CMatrix                             m_matFrozen;
};

#endif