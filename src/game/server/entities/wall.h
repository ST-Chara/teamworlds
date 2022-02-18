/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#pragma once

#include <game/server/entity.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include "pickup.h"

class CWall : public CEntity
{
public:
	CWall(CGameWorld *pGameWorld, int Owner);

	virtual void Reset();
    virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
    void StartWallEdit(vec2 Dir);
    void EndWallEdit(int ammo);
    bool Created;
    int m_Owner;

    void Die(int Killer, int Weapon=-1);
    bool TakeDamage(int Dmg, int From, int Weapon=-1);

    void HeIsHealing(CPlayer* player);
protected:
    bool HitCharacter();
    void CheckForBullets();
    void CheckForBulletCollision();
    void UpdateHealthInterface();
private:
    const float m_deconstruct_range = 50.f;
    const float m_collision_range = 30.f;
    const int m_wall_score = 2;

    CPlayer *pPlayer;
	vec2 m_From;
    vec2 m_Dir;
	int m_EvalTick;
    bool m_Done;
    float m_Delay_fac;
    int m_Health;
    CPickup *m_Health_Interface[CPlayer::m_Engineer_MaxWallHp];
};
