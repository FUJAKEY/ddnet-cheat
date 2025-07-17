#pragma once
// Минимальные заглушки для game/collision.h

class ICollision 
{
public:
    virtual ~ICollision() {}
    virtual int GetPureMapIndex(float x, float y) { return 0; }
    virtual int GetTileIndex(int Index) { return 0; }
    virtual int GetFrontTileIndex(int Index) { return 0; }
    virtual bool CheckPoint(float x, float y) { return false; }
    virtual bool IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) { return false; }
    virtual int IntersectNoLaser(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) { return 0; }
    virtual int GetCollisionAt(float x, float y) { return 0; }
    virtual int GetMapIndex(vec2 Pos) { return 0; }
    virtual vec2 GetPos(int Index) { return vec2(0, 0); }
    virtual int GetTile(int x, int y) { return 0; }
    virtual int GetFTile(int x, int y) { return 0; }
};

// Minimal tile definitions for testing
enum {
    TILE_AIR = 0,
    TILE_SOLID = 1,
    TILE_DEATH = 2,
    TILE_NOHOOK = 3,
    TILE_FREEZE = 9,
    TILE_UNFREEZE = 10,
    TILE_DFREEZE = 11,
    TILE_DUNFREEZE = 12,
    TILE_LFREEZE = 13,
    TILE_THROUGH = 20,
    TILE_THROUGH_ALL = 21,
    TILE_THROUGH_DIR = 22,
    TILE_THROUGH_CUT = 23
};
class CCollision { 
public: 
    int GetPureMapIndex(float x, float y) { return 0; }
    int GetTileIndex(int index) { return TILE_AIR; }
    int GetFrontTileIndex(int index) { return TILE_AIR; }
    bool CheckPoint(float x, float y) { return false; }
};

