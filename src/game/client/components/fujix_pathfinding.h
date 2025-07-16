#ifndef GAME_CLIENT_COMPONENTS_FUJIX_PATHFINDING_H
#define GAME_CLIENT_COMPONENTS_FUJIX_PATHFINDING_H

#include <base/vmath.h>
#include <base/system.h>
#include <game/gamecore.h>
#include <game/collision.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>

// Forward declarations
class CCollision;
class CCharacterCore;

// =====================================================
// PATH NODE - Represents a point in the pathfinding graph
// =====================================================
struct SPathNode
{
    vec2 m_Pos;                    // World position
    vec2 m_Vel;                    // Velocity at this node
    CNetObj_PlayerInput m_Input;   // Input required to reach this node
    
    float m_GCost;                 // Cost from start
    float m_HCost;                 // Heuristic cost to goal
    float m_FCost;                 // Total cost (G + H)
    
    int m_ParentIndex;             // Index of parent node (-1 if start)
    int m_TimeTicks;               // Time in ticks to reach this node
    bool m_IsGrounded;             // Whether character is on ground
    bool m_CanHook;                // Whether hook is available
    
    // Constructor
    SPathNode() : m_Pos(0, 0), m_Vel(0, 0), m_GCost(0), m_HCost(0), m_FCost(0), 
                  m_ParentIndex(-1), m_TimeTicks(0), m_IsGrounded(false), m_CanHook(true)
    {
        mem_zero(&m_Input, sizeof(m_Input));
    }
    
    SPathNode(vec2 Pos, vec2 Vel = vec2(0, 0)) : m_Pos(Pos), m_Vel(Vel), m_GCost(0), m_HCost(0), 
                  m_FCost(0), m_ParentIndex(-1), m_TimeTicks(0), m_IsGrounded(false), m_CanHook(true)
    {
        mem_zero(&m_Input, sizeof(m_Input));
    }
};

// Comparison for priority queue (lower F cost = higher priority)
struct SPathNodeComparator
{
    const std::vector<SPathNode> *m_pNodes;

    SPathNodeComparator(const std::vector<SPathNode> *pNodes = nullptr)
        : m_pNodes(pNodes)
    {
    }

    bool operator()(int a, int b) const;
};

// =====================================================
// MAP ANALYZER - Analyzes terrain and obstacles
// =====================================================
enum ETileType
{
    TILE_TYPE_SAFE = 0,
    TILE_TYPE_SOLID,
    TILE_TYPE_FREEZE,
    TILE_TYPE_DEATH,
    TILE_TYPE_PLATFORM,
    TILE_TYPE_HOOKABLE,
    TILE_TYPE_NOHOOK
};

struct SMapCell
{
    ETileType m_Type;
    bool m_IsPassable;
    bool m_IsHookable;
    float m_MovementCost;  // Additional cost for movement through this cell
    
    SMapCell() : m_Type(TILE_TYPE_SAFE), m_IsPassable(true), m_IsHookable(true), m_MovementCost(1.0f) {}
};

class CMapAnalyzer
{
    CCollision *m_pCollision;
    std::unordered_map<int, SMapCell> m_CachedCells; // Cache analyzed cells
    
public:
    CMapAnalyzer() : m_pCollision(nullptr) {}
    void Init(CCollision *pCollision) { m_pCollision = pCollision; }
    
    // Analyze a specific cell
    SMapCell AnalyzeCell(vec2 Pos);
    SMapCell AnalyzeCell(int x, int y);
    
    // Check if position is safe for movement
    bool IsSafePosition(vec2 Pos, float Radius = 14.0f);
    
    // Check if path between two points is clear
    bool IsPathClear(vec2 From, vec2 To, float Radius = 14.0f);
    
    // Get movement cost for a specific area
    float GetMovementCost(vec2 Pos, float Radius = 14.0f);
    
    // Find nearest safe position
    vec2 FindNearestSafePos(vec2 Pos, float SearchRadius = 64.0f);
    
    // Clear cache (call when map changes)
    void ClearCache() { m_CachedCells.clear(); }
    
private:
    ETileType ClassifyTile(int TileIndex, int FrontTileIndex);
    int GetCacheKey(int x, int y) { return (y << 16) | (x & 0xFFFF); }
};

// =====================================================
// PATH PLANNER - A* pathfinding with physics simulation
// =====================================================
struct SPathfindingConfig
{
    float m_MaxSearchRadius;       // Maximum search distance
    int m_MaxSearchTime;           // Maximum time in ticks to search
    int m_MaxNodes;                // Maximum nodes to explore
    float m_HookRange;             // Maximum hook range
    bool m_AllowJumps;             // Whether to use jumps
    bool m_AllowHook;              // Whether to use hook
    bool m_PreferGroundPath;       // Prefer paths that stay on ground
    
    SPathfindingConfig() : m_MaxSearchRadius(800.0f), m_MaxSearchTime(300), m_MaxNodes(1000),
                          m_HookRange(300.0f), m_AllowJumps(true), m_AllowHook(true),
                          m_PreferGroundPath(false) {}
};

class CPathPlanner
{
    CMapAnalyzer *m_pMapAnalyzer;
    CCollision *m_pCollision;
    SPathfindingConfig m_Config;
    
    std::vector<SPathNode> m_Nodes;
    std::priority_queue<int, std::vector<int>, SPathNodeComparator> m_OpenSet;
    std::vector<bool> m_ClosedSet;
    
public:
    CPathPlanner()
        : m_pMapAnalyzer(nullptr),
          m_pCollision(nullptr),
          m_OpenSet(SPathNodeComparator(&m_Nodes))
    {
    }
    void Init(CMapAnalyzer *pMapAnalyzer, CCollision *pCollision) 
    { 
        m_pMapAnalyzer = pMapAnalyzer; 
        m_pCollision = pCollision;
    }
    
    // Set configuration
    void SetConfig(const SPathfindingConfig &Config) { m_Config = Config; }
    
    // Find path from start to goal
    std::vector<SPathNode> FindPath(const CCharacterCore &StartCore, vec2 Goal);
    
    // Generate possible next moves from current state
    std::vector<SPathNode> GenerateSuccessors(const SPathNode &Node, const CCharacterCore &Core);
    
private:
    // Calculate costs
    float CalculateGCost(const SPathNode &From, const SPathNode &To);
    float CalculateHCost(vec2 From, vec2 To);
    
    // Physics simulation
    SPathNode SimulateMovement(const SPathNode &Node, const CNetObj_PlayerInput &Input, 
                              const CCharacterCore &Core, int Steps = 1);
    
    // Validate and optimize path
    std::vector<SPathNode> OptimizePath(const std::vector<SPathNode> &Path);
    bool ValidateNode(const SPathNode &Node);
    
    // Helper functions
    void Reset();
    int FindNodeIndex(vec2 Pos, float Tolerance = 16.0f);
};

// =====================================================
// SAFETY SIMULATOR - Validates path safety
// =====================================================
enum ESafetyLevel
{
    SAFETY_SAFE = 0,
    SAFETY_RISKY,
    SAFETY_DANGEROUS,
    SAFETY_FATAL
};

struct SSafetyResult
{
    ESafetyLevel m_Level;
    int m_FailurePoint;           // Index where path becomes unsafe (-1 if safe)
    vec2 m_DangerPos;             // Position of danger
    const char *m_pReason;        // Description of danger
    
    SSafetyResult() : m_Level(SAFETY_SAFE), m_FailurePoint(-1), m_DangerPos(0, 0), m_pReason("Safe") {}
};

class CSafetySimulator
{
    CMapAnalyzer *m_pMapAnalyzer;
    CCollision *m_pCollision;
    
public:
    CSafetySimulator() : m_pMapAnalyzer(nullptr), m_pCollision(nullptr) {}
    void Init(CMapAnalyzer *pMapAnalyzer, CCollision *pCollision)
    {
        m_pMapAnalyzer = pMapAnalyzer;
        m_pCollision = pCollision;
    }
    
    // Validate entire path
    SSafetyResult ValidatePath(const std::vector<SPathNode> &Path, const CCharacterCore &StartCore);
    
    // Simulate single step
    SSafetyResult SimulateStep(const CCharacterCore &Core, const CNetObj_PlayerInput &Input, int Steps = 5);
    
    // Check if position is safe
    ESafetyLevel CheckPositionSafety(vec2 Pos);
    
    // Get corrected path if original is unsafe
    std::vector<SPathNode> GetCorrectedPath(const std::vector<SPathNode> &Path, 
                                           const CCharacterCore &StartCore);
    
private:
    bool WillHitFreeze(const CCharacterCore &Core, int LookAheadSteps = 10);
    bool WillFallInVoid(const CCharacterCore &Core, int LookAheadSteps = 15);
    ESafetyLevel AnalyzeTrajectory(const CCharacterCore &Core, int Steps);
};

// =====================================================
// SMART AUTOPILOT - Main controller integrating all systems
// =====================================================
enum EAutopilotState
{
    AUTOPILOT_IDLE = 0,
    AUTOPILOT_PLANNING,
    AUTOPILOT_EXECUTING,
    AUTOPILOT_STUCK,
    AUTOPILOT_REACHED
};

struct SAutopilotStatus
{
    EAutopilotState m_State;
    vec2 m_Target;
    float m_Progress;               // 0.0 to 1.0
    int m_CurrentStep;
    int m_TotalSteps;
    const char *m_pStatusMessage;
    
    SAutopilotStatus() : m_State(AUTOPILOT_IDLE), m_Target(0, 0), m_Progress(0.0f),
                        m_CurrentStep(0), m_TotalSteps(0), m_pStatusMessage("Idle") {}
};

class CSmartAutopilot
{
    std::unique_ptr<CMapAnalyzer> m_pMapAnalyzer;
    std::unique_ptr<CPathPlanner> m_pPathPlanner;
    std::unique_ptr<CSafetySimulator> m_pSafetySimulator;
    
    std::vector<SPathNode> m_CurrentPath;
    int m_CurrentStepIndex;
    vec2 m_LastTarget;
    int m_LastPlanTick;
    int m_StuckCounter;
    
    SAutopilotStatus m_Status;
    SPathfindingConfig m_Config;
    
public:
    CSmartAutopilot();
    ~CSmartAutopilot();
    
    void Init(CCollision *pCollision);
    void SetTarget(vec2 Target);
    void SetConfig(const SPathfindingConfig &Config);
    
    // Main update function
    bool UpdateAutopilot(CCharacterCore &Core, CNetObj_PlayerInput *pInput);
    
    // Status and control
    SAutopilotStatus GetStatus() const { return m_Status; }
    bool IsActive() const { return m_Status.m_State != AUTOPILOT_IDLE; }
    void Stop();
    void Reset();
    
    // Configuration
    void EnableJumps(bool Enable) { m_Config.m_AllowJumps = Enable; }
    void EnableHook(bool Enable) { m_Config.m_AllowHook = Enable; }
    void SetMaxSearchRadius(float Radius) { m_Config.m_MaxSearchRadius = Radius; }
    
    // Debug and visualization
    const std::vector<SPathNode> &GetCurrentPath() const { return m_CurrentPath; }
    bool ShouldReplan(const CCharacterCore &Core) const;
    
private:
    bool PlanPath(const CCharacterCore &Core);
    bool ExecuteStep(CCharacterCore &Core, CNetObj_PlayerInput *pInput);
    void UpdateStatus(const CCharacterCore &Core);
    bool IsStuck(const CCharacterCore &Core);
    void HandleStuckState(const CCharacterCore &Core);
};

#endif // GAME_CLIENT_COMPONENTS_FUJIX_PATHFINDING_H
