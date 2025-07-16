
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

