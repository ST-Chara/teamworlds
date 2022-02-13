/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/gamecontext.h>

#include "character.h"
#include "wall.h"
#include "projectile.h"
#include "pickup.h"

CWall::CWall(CGameWorld *pGameWorld, int Owner) :  CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, vec2(0, 0))
{
    m_Owner = Owner;
    m_EvalTick = 0;
    pPlayer = GameServer()->GetPlayerChar(m_Owner)->GetPlayer();
    Created= false;
    m_Done= false;
    m_Delay_fac = 1000.0f;
    m_Health=0;
    for (int i = 0; i<=pPlayer->m_Engineer_MaxWallHp; i++){
        m_Health_Interface[i] = nullptr;
    }
    m_Pos=vec2 (0,0);
}


bool CWall::HitCharacter()
{
    vec2 At;
    CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

    CCharacter *pHit = GameWorld()->IntersectCharacter(m_Pos, m_From, 0.f, At, pOwnerChar);
    if(!pHit)
        return false;
    if(pHit->GetPlayer()->GetTeam() == pPlayer->GetTeam())
        return false;

    pHit->TakeDamage(vec2(0.f, 0.f), normalize(m_Pos-m_From), g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage*2, m_Owner, WEAPON_LASER);
    TakeDamage(1, pHit->GetPlayer()->GetCID());
    return true;
}
void CWall::EndWallEdit(){
    m_From = pPlayer->GetCharacter()->GetPos();
    m_Delay_fac = 10000.0f;

    GameServer()->Collision()->IntersectLine(m_Pos, m_From, 0x0, &m_From);
    
    m_Health=pPlayer->m_Engineer_MaxWallHp;

    vec2 Middle = vec2((m_Pos.x+m_From.x)/2, (m_Pos.y+m_From.y)/2);
    for (int i = 0; i<=pPlayer->m_Engineer_MaxWallHp; i++){
        m_Health_Interface[i] = new CPickup(GameWorld(), PICKUP_HEALTH, Middle, false);
    }

    m_Done= true;
}

void CWall::StartWallEdit(vec2 Dir){
    if (!m_Done) {
        m_Dir = Dir;
        GameWorld()->InsertEntity(this);
        m_From = pPlayer->GetCharacter()->GetPos();
        m_Pos = m_From + m_Dir * 800.0f;
        GameServer()->Collision()->IntersectLine(m_From, m_Pos, 0x0, &m_Pos);
        m_EvalTick = Server()->Tick();
        Created = true;
    }
}

bool CWall::TakeDamage(int Dmg, int From, int Weapon) {
    if (Dmg>0){
        m_Health-=Dmg;
    }

    if (m_Health <= 0) {
        Die(From, Weapon);
        return true;
    }
    return false;
}

void CWall::Die(int Killer, int Weapon) {
    new CProjectile(GameWorld(), WEAPON_GRENADE,
                    pPlayer->GetCID(),
                    m_Pos,
                    vec2(0, 0),
                    0,
                    g_pData->m_Weapons.m_Grenade.m_pBase->m_Damage, true, 0, SOUND_GRENADE_EXPLODE,
                    WEAPON_GRENADE);
    new CProjectile(GameWorld(), WEAPON_GRENADE,
                    pPlayer->GetCID(),
                    m_From,
                    vec2(0, 0),
                    0,
                    g_pData->m_Weapons.m_Grenade.m_pBase->m_Damage, true, 0, SOUND_GRENADE_EXPLODE,
                    WEAPON_GRENADE);
    if (Killer==-1){
        CNetMsg_Sv_Chat chatMsg;
        chatMsg.m_Mode = CHAT_WHISPER;
        chatMsg.m_ClientID = m_Owner;
        chatMsg.m_TargetID = m_Owner;
        chatMsg.m_pMessage = "Wall was destroyed by zombie";
        Server()->SendPackMsg(&chatMsg, MSGFLAG_VITAL, m_Owner);
    } else if (Killer == -2) {

    } else {
        CNetMsg_Sv_Chat chatMsg;
        chatMsg.m_Mode = CHAT_WHISPER;
        chatMsg.m_ClientID = m_Owner;
        chatMsg.m_TargetID = m_Owner;
        char WallMsg[128];
        str_format(WallMsg, sizeof(WallMsg), "Wall was destroyed by %s", Server()->ClientName(Killer));
        chatMsg.m_pMessage = WallMsg;
        Server()->SendPackMsg(&chatMsg, MSGFLAG_VITAL, m_Owner);
        GameServer()->m_apPlayers[Killer]->m_Score+=m_wall_score;
    }
    Reset();
}

void CWall::CheckForBulletCollision(){
    CProjectile *pAttackBullet = (CProjectile *) GameWorld()->FindFirst(GameWorld()->ENTTYPE_PROJECTILE);
    if (pAttackBullet){
        for (; pAttackBullet; pAttackBullet = (CProjectile *)pAttackBullet->TypeNext()){
            vec2 At;
            if (GameWorld()->IntersectBullet(m_Pos, m_From, m_collision_range, At, pAttackBullet)){

                if (GameServer()->m_apPlayers[pAttackBullet->GetOwner()]->GetTeam() != GameServer()->m_apPlayers[m_Owner]->GetTeam()){

                    float Ct = (Server()->Tick()-pAttackBullet->GetStartTick())/(float)Server()->TickSpeed();

                    if (distance(pAttackBullet->GetPos(Ct), m_Pos) <= m_deconstruct_range){
                        int Dmg;
                        if (pAttackBullet->GetExposive()){
                            Dmg = 3;
                        }else{
                            Dmg = 1;
                        }
                        if (TakeDamage(Dmg, pAttackBullet->GetOwner(), pAttackBullet->GetWeapon())){
                            pAttackBullet->Wall_Coll= true;
                            pAttackBullet->Tick();
                            return;
                        }
                        GameServer()->CreateDamage(m_Pos, m_Owner, pAttackBullet->GetPos(Ct), Dmg, 0, false);
                    }

                    if (distance(pAttackBullet->GetPos(Ct), m_From) <= m_deconstruct_range){
                        int Dmg;
                        if (pAttackBullet->GetExposive()){
                            Dmg = 3;
                        }else{
                            Dmg = 1;
                        }
                        if (TakeDamage(Dmg, pAttackBullet->GetOwner(), pAttackBullet->GetWeapon())){
                            pAttackBullet->Wall_Coll= true;
                            pAttackBullet->Tick();
                            return;
                        }
                        GameServer()->CreateDamage(m_From, m_Owner, pAttackBullet->GetPos(Ct), Dmg, 0, false);
                    }
                    pAttackBullet->Wall_Coll= true;
                    pAttackBullet->Tick();
                }
                if (pAttackBullet==(CProjectile *)pAttackBullet->TypeNext()){
                    break;
                }
            }
        }
    }
}

void CWall::CheckForBullets() {
    CProjectile *pPosBullet = (CProjectile *) GameWorld()->ClosestEntity(m_Pos, m_deconstruct_range, GameWorld()->ENTTYPE_PROJECTILE, this);
    if (pPosBullet){
        if(pPosBullet->GetOwner() == m_Owner and pPosBullet->GetWeapon()==WEAPON_GUN){
            pPlayer->GetCharacter()->GiveWeapon(WEAPON_LASER, 10); //max laser ammo
            pPosBullet->Destroy();
            Reset();
        }
    }

    CProjectile *pFromBullet = (CProjectile *) GameWorld()->ClosestEntity(m_From, m_deconstruct_range, GameWorld()->ENTTYPE_PROJECTILE, this);
    if (pFromBullet){
        if(pFromBullet->GetOwner() == m_Owner and pFromBullet->GetWeapon()==WEAPON_GUN){
            pPlayer->GetCharacter()->GiveWeapon(WEAPON_LASER, 10); //max laser ammo
            pFromBullet->Destroy();
            Reset();
        }
    }
}

void CWall::UpdateHealthInterface(){
}

void CWall::Reset()
{
    for (int i = 0; i<=pPlayer->m_Engineer_MaxWallHp; i++){
        m_Health_Interface[i]->Destroy();
    }
    pPlayer->m_Engineer_ActiveWalls--;
    GameWorld()->DestroyEntity(this);
}

void CWall::Tick() {
    if (Server()->Tick() > m_EvalTick + (Server()->TickSpeed() * GameServer()->Tuning()->m_LaserBounceDelay) / m_Delay_fac){
        if(pPlayer->GetCharacter()) {
            if (pPlayer->GetCharacter()->GetActiveWeapon()!=WEAPON_LASER and !m_Done){
                return;
            }
            if (pPlayer->m_Engineer_Wall_Editing and !m_Done) {
                m_EvalTick = Server()->Tick();

                m_From = pPlayer->GetCharacter()->GetPos();

                GameServer()->Collision()->IntersectLine(m_Pos, m_From, 0x0, &m_From);

                GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
            } else {
                m_EvalTick = Server()->Tick();

                GameServer()->Collision()->IntersectLine(m_Pos, m_From, 0x0, &m_From);

                HitCharacter();
                CheckForBullets();
                CheckForBulletCollision();
                UpdateHealthInterface();
            }
        } else{
            Die(-2);
        }
    }
}

void CWall::TickPaused()
{
    ++m_EvalTick;
}

void CWall::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient) && NetworkClipped(SnappingClient, m_From))
        return;

    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
    if(!pObj)
        return;

    pObj->m_X = (int)m_Pos.x;
    pObj->m_Y = (int)m_Pos.y;
    pObj->m_FromX = (int)m_From.x;
    pObj->m_FromY = (int)m_From.y;
    pObj->m_StartTick = m_EvalTick;
}