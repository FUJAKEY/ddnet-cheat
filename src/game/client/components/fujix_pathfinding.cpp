#include "fujix_pathfinding.h"
#include <game/collision.h>
#include <game/mapitems.h>
#include <base/system.h>
#include <base/math.h>
#include <algorithm>
#include <cmath>

// =====================================================
// PATH NODE COMPARATOR IMPLEMENTATION
// =====================================================
static std::vector<SPathNode> *s_pNodes = nullptr;

bool SPathNodeComparator::operator()(int a, int b) const
{
    if (!s_pNodes || a >= (int)s_pNodes->size() || b >= (int)s_pNodes->size())
        return false;
    return (*s_pNodes)[a].m_FCost > (*s_pNodes)[b].m_FCost; // Higher F cost = lower priority
}

// =====================================================
// MAP ANALYZER IMPLEMENTATION  
// =====================================================
SMapCell CMapAnalyzer::AnalyzeCell(vec2 Pos)
{
    int x = round_to_int(Pos.x / 32.0f);
    int y = round_to_int(Pos.y / 32.0f);
    return AnalyzeCell(x, y);
}

SMapCell CMapAnalyzer::AnalyzeCell(int x, int y)
{
    int Key = GetCacheKey(x, y);
    auto It = m_CachedCells.find(Key);
    if (It != m_CachedCells.end())
        return It->second;

    SMapCell Cell;
    if (!m_pCollision)
    {
        m_CachedCells[Key] = Cell;
        return Cell;
    }

    vec2 Pos = vec2(x * 32.0f, y * 32.0f);
    int Index = m_pCollision->GetPureMapIndex(Pos.x, Pos.y);
    int TileIndex = m_pCollision->GetTileIndex(Index);
    int FrontTileIndex = m_pCollision->GetFrontTileIndex(Index);
    
    Cell.m_Type = ClassifyTile(TileIndex, FrontTileIndex);
    
    // Determine passability
    switch (Cell.m_Type)
    {
        case TILE_TYPE_SOLID:
            Cell.m_IsPassable = false;
            Cell.m_IsHookable = m_pCollision->CheckPoint(Pos.x, Pos.y) ? false : true;
            Cell.m_MovementCost = 999.0f; // Very high cost
            break;
        case TILE_TYPE_FREEZE:
            Cell.m_IsPassable = true;
            Cell.m_IsHookable = true;
            Cell.m_MovementCost = 50.0f; // High cost but passable
            break;
        case TILE_TYPE_DEATH:
            Cell.m_IsPassable = false;
            Cell.m_IsHookable = false;
            Cell.m_MovementCost = 999.0f;
            break;
        case TILE_TYPE_PLATFORM:
            Cell.m_IsPassable = true;
            Cell.m_IsHookable = true;
            Cell.m_MovementCost = 2.0f; // Slightly higher cost
            break;
        case TILE_TYPE_NOHOOK:
            Cell.m_IsPassable = true;
            Cell.m_IsHookable = false;
            Cell.m_MovementCost = 1.5f;
            break;
        default: // TILE_TYPE_SAFE
            Cell.m_IsPassable = true;
            Cell.m_IsHookable = true;
            Cell.m_MovementCost = 1.0f;
            break;
    }

    m_CachedCells[Key] = Cell;
    return Cell;
}

ETileType CMapAnalyzer::ClassifyTile(int TileIndex, int FrontTileIndex)
{
    // Check front layer first (higher priority)
    if (FrontTileIndex == TILE_FREEZE || FrontTileIndex == TILE_DFREEZE || FrontTileIndex == TILE_LFREEZE)
        return TILE_TYPE_FREEZE;
    if (FrontTileIndex == TILE_DEATH)
        return TILE_TYPE_DEATH;
    if (FrontTileIndex == TILE_NOHOOK)
        return TILE_TYPE_NOHOOK;
    
    // Check main layer
    if (TileIndex == TILE_FREEZE || TileIndex == TILE_DFREEZE || TileIndex == TILE_LFREEZE)
        return TILE_TYPE_FREEZE;
    if (TileIndex == TILE_DEATH)
        return TILE_TYPE_DEATH;
    if (TileIndex == TILE_SOLID)
        return TILE_TYPE_SOLID;
    if (TileIndex == TILE_NOHOOK)
        return TILE_TYPE_NOHOOK;
    if (TileIndex >= TILE_PLATFORM_BLUE && TileIndex <= TILE_PLATFORM_ORANGE)
        return TILE_TYPE_PLATFORM;
    
    return TILE_TYPE_SAFE;
}

bool CMapAnalyzer::IsSafePosition(vec2 Pos, float Radius)
{
    // Check multiple points around the character
    const int CheckPoints = 8;
    for (int i = 0; i < CheckPoints; i++)
    {
        float Angle = (2.0f * pi * i) / CheckPoints;
        vec2 CheckPos = Pos + vec2(cos(Angle) * Radius, sin(Angle) * Radius);
        
        SMapCell Cell = AnalyzeCell(CheckPos);
        if (Cell.m_Type == TILE_TYPE_DEATH || Cell.m_Type == TILE_TYPE_SOLID)
            return false;
    }
    
    // Check center
    SMapCell CenterCell = AnalyzeCell(Pos);
    return CenterCell.m_Type != TILE_TYPE_DEATH && CenterCell.m_Type != TILE_TYPE_SOLID;
}

bool CMapAnalyzer::IsPathClear(vec2 From, vec2 To, float Radius)
{
    vec2 Dir = normalize(To - From);
    float Distance = length(To - From);
    const int Steps = maximum(1, (int)(Distance / 16.0f)); // Check every 16 units
    
    for (int i = 0; i <= Steps; i++)
    {
        vec2 CheckPos = From + Dir * (Distance * i / Steps);
        if (!IsSafePosition(CheckPos, Radius))
            return false;
    }
    
    return true;
}

float CMapAnalyzer::GetMovementCost(vec2 Pos, float Radius)
{
    float TotalCost = 0.0f;
    int CheckCount = 0;
    
    // Sample points around the position
    const int SampleRadius = (int)(Radius / 16.0f) + 1;
    for (int dx = -SampleRadius; dx <= SampleRadius; dx++)
    {
        for (int dy = -SampleRadius; dy <= SampleRadius; dy++)
        {
            vec2 SamplePos = Pos + vec2(dx * 16.0f, dy * 16.0f);
            if (length(SamplePos - Pos) <= Radius)
            {
                SMapCell Cell = AnalyzeCell(SamplePos);
                TotalCost += Cell.m_MovementCost;
                CheckCount++;
            }
        }
    }
    
    return CheckCount > 0 ? TotalCost / CheckCount : 1.0f;
}

vec2 CMapAnalyzer::FindNearestSafePos(vec2 Pos, float SearchRadius)
{
    if (IsSafePosition(Pos))
        return Pos;
    
    // Search in expanding circles
    const int Steps = 8;
    for (float r = 16.0f; r <= SearchRadius; r += 16.0f)
    {
        for (int i = 0; i < Steps; i++)
        {
            float Angle = (2.0f * pi * i) / Steps;
            vec2 TestPos = Pos + vec2(cos(Angle) * r, sin(Angle) * r);
            if (IsSafePosition(TestPos))
                return TestPos;
        }
    }
    
    return Pos; // Return original if no safe position found
}
// =====================================================
// PATH PLANNER IMPLEMENTATION
// =====================================================
std::vector<SPathNode> CPathPlanner::FindPath(const CCharacterCore &StartCore, vec2 Goal)
{
    std::vector<SPathNode> Path;
    
    if(!m_pMapAnalyzer || !m_pCollision)
        return Path;
    
    // Set global pointer for comparator
    s_pNodes = &m_Nodes;
    
    // Reset state
    Reset();
    
    // Create start node
    SPathNode StartNode(StartCore.m_Pos, StartCore.m_Vel);
    StartNode.m_GCost = 0.0f;
    StartNode.m_HCost = CalculateHCost(StartCore.m_Pos, Goal);
    StartNode.m_FCost = StartNode.m_GCost + StartNode.m_HCost;
    
    m_Nodes.push_back(StartNode);
    m_OpenSet.push(0);
    
    int Iterations = 0;
    while(!m_OpenSet.empty() && Iterations < m_Config.m_MaxNodes)
    {
        Iterations++;
        
        // Get node with lowest F cost
        int CurrentIndex = m_OpenSet.top();
        m_OpenSet.pop();
        
        if(CurrentIndex >= (int)m_ClosedSet.size())
            m_ClosedSet.resize(CurrentIndex + 1, false);
        
        if(m_ClosedSet[CurrentIndex])
            continue; // Already processed
        
        m_ClosedSet[CurrentIndex] = true;
        
        SPathNode &CurrentNode = m_Nodes[CurrentIndex];
        
        // Check if we reached the goal
        if(length(CurrentNode.m_Pos - Goal) < 32.0f) // Within 32 units
        {
            // Reconstruct path
            int NodeIndex = CurrentIndex;
            while(NodeIndex != -1)
            {
                Path.insert(Path.begin(), m_Nodes[NodeIndex]);
                NodeIndex = m_Nodes[NodeIndex].m_ParentIndex;
            }
            break;
        }
        
        // Generate successors (simplified for now)
        // Simple movement in 4 directions
        vec2 Directions[] = {
            vec2(32, 0),   // Right
            vec2(-32, 0),  // Left
            vec2(0, -32),  // Up (jump)
            vec2(0, 32)    // Down
        };
        
        for(int i = 0; i < 4; i++)
        {
            vec2 NewPos = CurrentNode.m_Pos + Directions[i];
            
            // Check if position is safe
            if(!m_pMapAnalyzer->IsSafePosition(NewPos))
                continue;
            
            // Check if we already have this node
            int ExistingIndex = FindNodeIndex(NewPos);
            
            float GCost = CurrentNode.m_GCost + length(Directions[i]) * m_pMapAnalyzer->GetMovementCost(NewPos);
            
            if(ExistingIndex != -1)
            {
                // Update if we found a better path
                if(GCost < m_Nodes[ExistingIndex].m_GCost)
                {
                    m_Nodes[ExistingIndex].m_GCost = GCost;
                    m_Nodes[ExistingIndex].m_FCost = GCost + m_Nodes[ExistingIndex].m_HCost;
                    m_Nodes[ExistingIndex].m_ParentIndex = CurrentIndex;
                }
            }
            else
            {
                // Create new node
                SPathNode NewNode(NewPos);
                NewNode.m_GCost = GCost;
                NewNode.m_HCost = CalculateHCost(NewPos, Goal);
                NewNode.m_FCost = NewNode.m_GCost + NewNode.m_HCost;
                NewNode.m_ParentIndex = CurrentIndex;
                
                m_Nodes.push_back(NewNode);
                m_OpenSet.push((int)m_Nodes.size() - 1);
            }
        }
    }
    
    return OptimizePath(Path);
}

std::vector<SPathNode> CPathPlanner::GenerateSuccessors(const SPathNode &Node, const CCharacterCore &Core)
{
    std::vector<SPathNode> Successors;
    
    // TODO: Implement physics-based successor generation
    // This would simulate various inputs (movement + jump + hook combinations)
    // and create successor nodes based on realistic character physics
    
    return Successors;
}

float CPathPlanner::CalculateGCost(const SPathNode &From, const SPathNode &To)
{
    float Distance = length(To.m_Pos - From.m_Pos);
    float MovementCost = m_pMapAnalyzer ? m_pMapAnalyzer->GetMovementCost(To.m_Pos) : 1.0f;
    float TimePenalty = To.m_TimeTicks * 0.1f; // Small penalty for longer paths
    
    return Distance * MovementCost + TimePenalty;
}

float CPathPlanner::CalculateHCost(vec2 From, vec2 To)
{
    // Manhattan distance with some adjustments for Teeworlds physics
    vec2 Diff = To - From;
    float HorizontalDist = abs(Diff.x);
    float VerticalDist = abs(Diff.y);
    
    // Vertical movement is generally more expensive (requires jumps/hook)
    return HorizontalDist + VerticalDist * 1.2f;
}

SPathNode CPathPlanner::SimulateMovement(const SPathNode &Node, const CNetObj_PlayerInput &Input, 
                                        const CCharacterCore &Core, int Steps)
{
    // TODO: Implement physics simulation
    // This would create a temporary character core, apply the input, and simulate Steps ticks
    SPathNode Result = Node;
    return Result;
}

std::vector<SPathNode> CPathPlanner::OptimizePath(const std::vector<SPathNode> &Path)
{
    if(Path.size() < 3)
        return Path;
    
    std::vector<SPathNode> OptimizedPath;
    OptimizedPath.push_back(Path[0]);
    
    // Remove unnecessary intermediate nodes
    for(size_t i = 1; i < Path.size() - 1; i++)
    {
        vec2 Prev = OptimizedPath.back().m_Pos;
        vec2 Next = Path[i + 1].m_Pos;
        
        // Check if we can go directly from Prev to Next
        if(!m_pMapAnalyzer || !m_pMapAnalyzer->IsPathClear(Prev, Next))
        {
            OptimizedPath.push_back(Path[i]);
        }
    }
    
    OptimizedPath.push_back(Path.back());
    return OptimizedPath;
}

bool CPathPlanner::ValidateNode(const SPathNode &Node)
{
    if(!m_pMapAnalyzer)
        return true;
    
    return m_pMapAnalyzer->IsSafePosition(Node.m_Pos);
}

void CPathPlanner::Reset()
{
    m_Nodes.clear();
    while(!m_OpenSet.empty())
        m_OpenSet.pop();
    m_ClosedSet.clear();
}

int CPathPlanner::FindNodeIndex(vec2 Pos, float Tolerance)
{
    for(size_t i = 0; i < m_Nodes.size(); i++)
    {
        if(length(m_Nodes[i].m_Pos - Pos) <= Tolerance)
            return (int)i;
    }
    return -1;
}
// =====================================================
// SAFETY SIMULATOR IMPLEMENTATION
// =====================================================
SSafetyResult CSafetySimulator::ValidatePath(const std::vector<SPathNode> &Path, const CCharacterCore &StartCore)
{
    SSafetyResult Result;
    
    if(Path.empty())
    {
        Result.m_Level = SAFETY_FATAL;
        Result.m_pReason = "No path provided";
        return Result;
    }
    
    // TODO: Implement full path validation using phantom core simulation
    
    return Result;
}

SSafetyResult CSafetySimulator::SimulateStep(const CCharacterCore &Core, const CNetObj_PlayerInput &Input, int Steps)
{
    SSafetyResult Result;
    
    // TODO: Implement step-by-step safety simulation
    
    return Result;
}

ESafetyLevel CSafetySimulator::CheckPositionSafety(vec2 Pos)
{
    if(!m_pMapAnalyzer)
        return SAFETY_SAFE;
    
    SMapCell Cell = m_pMapAnalyzer->AnalyzeCell(Pos);
    
    switch(Cell.m_Type)
    {
        case TILE_TYPE_FREEZE:
        case TILE_TYPE_DEATH:
            return SAFETY_FATAL;
        case TILE_TYPE_SOLID:
            return SAFETY_DANGEROUS;
        case TILE_TYPE_PLATFORM:
            return SAFETY_RISKY;
        default:
            return SAFETY_SAFE;
    }
}

std::vector<SPathNode> CSafetySimulator::GetCorrectedPath(const std::vector<SPathNode> &Path, const CCharacterCore &StartCore)
{
    // TODO: Implement path correction algorithm
    return Path;
}

bool CSafetySimulator::WillHitFreeze(const CCharacterCore &Core, int LookAheadSteps)
{
    // TODO: Implement freeze detection
    return false;
}

bool CSafetySimulator::WillFallInVoid(const CCharacterCore &Core, int LookAheadSteps)
{
    // TODO: Implement void fall detection  
    return false;
}

ESafetyLevel CSafetySimulator::AnalyzeTrajectory(const CCharacterCore &Core, int Steps)
{
    // TODO: Implement trajectory analysis
    return SAFETY_SAFE;
}

// =====================================================
// SMART AUTOPILOT IMPLEMENTATION
// =====================================================
CSmartAutopilot::CSmartAutopilot()
{
    m_pMapAnalyzer = std::make_unique<CMapAnalyzer>();
    m_pPathPlanner = std::make_unique<CPathPlanner>();
    m_pSafetySimulator = std::make_unique<CSafetySimulator>();
    
    m_CurrentStepIndex = 0;
    m_LastTarget = vec2(0, 0);
    m_LastPlanTick = 0;
    m_StuckCounter = 0;
}

CSmartAutopilot::~CSmartAutopilot() = default;

void CSmartAutopilot::Init(CCollision *pCollision)
{
    if(!pCollision)
        return;
    
    m_pMapAnalyzer->Init(pCollision);
    m_pPathPlanner->Init(m_pMapAnalyzer.get(), pCollision);
    m_pSafetySimulator->Init(m_pMapAnalyzer.get(), pCollision);
    
    // Set default configuration
    m_pPathPlanner->SetConfig(m_Config);
}

void CSmartAutopilot::SetTarget(vec2 Target)
{
    m_Status.m_Target = Target;
    m_LastTarget = Target;
    m_Status.m_State = AUTOPILOT_PLANNING;
    m_Status.m_pStatusMessage = "Planning path...";
    m_CurrentStepIndex = 0;
    m_StuckCounter = 0;
}

void CSmartAutopilot::SetConfig(const SPathfindingConfig &Config)
{
    m_Config = Config;
    if(m_pPathPlanner)
        m_pPathPlanner->SetConfig(Config);
}

bool CSmartAutopilot::UpdateAutopilot(CCharacterCore &Core, CNetObj_PlayerInput *pInput)
{
    if(!pInput)
        return false;
    
    UpdateStatus(Core);
    
    // Check if we need to replan
    if(ShouldReplan(Core))
    {
        if(!PlanPath(Core))
        {
            m_Status.m_State = AUTOPILOT_STUCK;
            m_Status.m_pStatusMessage = "Failed to find path";
            return false;
        }
    }
    
    // Execute current step
    if(m_Status.m_State == AUTOPILOT_EXECUTING)
    {
        return ExecuteStep(Core, pInput);
    }
    
    return false;
}

void CSmartAutopilot::Stop()
{
    m_Status.m_State = AUTOPILOT_IDLE;
    m_Status.m_pStatusMessage = "Stopped";
    m_CurrentPath.clear();
    m_CurrentStepIndex = 0;
    m_StuckCounter = 0;
}

void CSmartAutopilot::Reset()
{
    Stop();
    m_LastTarget = vec2(0, 0);
    m_LastPlanTick = 0;
}

bool CSmartAutopilot::ShouldReplan(const CCharacterCore &Core) const
{
    // Replan if we don't have a path
    if(m_CurrentPath.empty())
        return true;
    
    // Replan if target changed significantly
    if(length(m_LastTarget - m_Status.m_Target) > 64.0f)
        return true;
    
    // Replan if we're stuck
    if(m_Status.m_State == AUTOPILOT_STUCK)
        return true;
    
    // Replan if we've completed the current path
    if(m_CurrentStepIndex >= (int)m_CurrentPath.size())
        return true;
    
    return false;
}

bool CSmartAutopilot::PlanPath(const CCharacterCore &Core)
{
    m_Status.m_State = AUTOPILOT_PLANNING;
    m_Status.m_pStatusMessage = "Planning path...";
    
    if(!m_pPathPlanner)
        return false;
    
    // Clear old path
    m_CurrentPath.clear();
    m_CurrentStepIndex = 0;
    
    // Find new path
    std::vector<SPathNode> NewPath = m_pPathPlanner->FindPath(Core, m_Status.m_Target);
    
    if(NewPath.empty())
    {
        m_Status.m_State = AUTOPILOT_STUCK;
        m_Status.m_pStatusMessage = "No path found";
        return false;
    }
    
    // Validate path safety
    if(m_pSafetySimulator)
    {
        SSafetyResult SafetyResult = m_pSafetySimulator->ValidatePath(NewPath, Core);
        if(SafetyResult.m_Level == SAFETY_FATAL)
        {
            m_Status.m_State = AUTOPILOT_STUCK;
            m_Status.m_pStatusMessage = SafetyResult.m_pReason;
            return false;
        }
    }
    
    m_CurrentPath = NewPath;
    m_CurrentStepIndex = 0;
    m_Status.m_State = AUTOPILOT_EXECUTING;
    m_Status.m_pStatusMessage = "Executing path";
    m_Status.m_TotalSteps = (int)m_CurrentPath.size();
    m_LastTarget = m_Status.m_Target;
    m_StuckCounter = 0;
    
    return true;
}

bool CSmartAutopilot::ExecuteStep(CCharacterCore &Core, CNetObj_PlayerInput *pInput)
{
    if(m_CurrentPath.empty() || m_CurrentStepIndex >= (int)m_CurrentPath.size())
        return false;
    
    // Check if we're close enough to the target
    if(length(Core.m_Pos - m_Status.m_Target) < 32.0f)
    {
        m_Status.m_State = AUTOPILOT_REACHED;
        m_Status.m_pStatusMessage = "Target reached";
        mem_zero(pInput, sizeof(*pInput));
        return true;
    }
    
    // Get current target node
    const SPathNode &TargetNode = m_CurrentPath[m_CurrentStepIndex];
    
    // Check if we reached current step
    if(length(Core.m_Pos - TargetNode.m_Pos) < 24.0f)
    {
        m_CurrentStepIndex++;
        if(m_CurrentStepIndex >= (int)m_CurrentPath.size())
        {
            m_Status.m_State = AUTOPILOT_REACHED;
            m_Status.m_pStatusMessage = "Path completed";
            return true;
        }
    }
    
    // Calculate input for current step
    vec2 Direction = normalize(TargetNode.m_Pos - Core.m_Pos);
    float Distance = length(TargetNode.m_Pos - Core.m_Pos);
    
    // Basic movement logic
    mem_zero(pInput, sizeof(*pInput));
    
    // Horizontal movement
    if(Direction.x > 0.1f)
        pInput->m_Direction = 1;
    else if(Direction.x < -0.1f)
        pInput->m_Direction = -1;
    
    // Jumping logic
    if(Direction.y < -32.0f && Core.m_Vel.y > -1.0f) // Need to go up and not already jumping
        pInput->m_Jump = 1;
    
    // Hook logic for long distances
    if(Distance > 96.0f && m_Config.m_AllowHook)
    {
        pInput->m_Hook = 1;
        pInput->m_TargetX = (int)(Direction.x * 256.0f);
        pInput->m_TargetY = (int)(Direction.y * 256.0f);
    }
    
    // Check for stuck state
    if(IsStuck(Core))
    {
        HandleStuckState(Core);
        return false;
    }
    
    return true;
}

void CSmartAutopilot::UpdateStatus(const CCharacterCore &Core)
{
    m_Status.m_CurrentStep = m_CurrentStepIndex;
    
    if(!m_CurrentPath.empty() && m_Status.m_TotalSteps > 0)
    {
        m_Status.m_Progress = (float)m_CurrentStepIndex / m_Status.m_TotalSteps;
    }
    else
    {
        m_Status.m_Progress = 0.0f;
    }
}

bool CSmartAutopilot::IsStuck(const CCharacterCore &Core)
{
    static vec2 s_LastPos = Core.m_Pos;
    static int s_StuckTicks = 0;
    
    // Check if position changed significantly
    if(length(Core.m_Pos - s_LastPos) < 8.0f)
    {
        s_StuckTicks++;
    }
    else
    {
        s_StuckTicks = 0;
        s_LastPos = Core.m_Pos;
    }
    
    // Consider stuck after 60 ticks (1 second) without movement
    return s_StuckTicks > 60;
}

void CSmartAutopilot::HandleStuckState(const CCharacterCore &Core)
{
    m_StuckCounter++;
    
    if(m_StuckCounter > 3) // After 3 stuck attempts, give up
    {
        m_Status.m_State = AUTOPILOT_STUCK;
        m_Status.m_pStatusMessage = "Stuck - giving up";
        return;
    }
    
    // Try to replan with different settings
    SPathfindingConfig NewConfig = m_Config;
    NewConfig.m_AllowJumps = true;
    NewConfig.m_AllowHook = true;
    NewConfig.m_MaxSearchRadius *= 1.5f; // Expand search
    
    SetConfig(NewConfig);
    m_Status.m_State = AUTOPILOT_PLANNING; // Force replan
}
// Compatibility method for fujix_tas integration
bool CSmartAutopilot::Update(const SAutopilotState &State, CNetObj_PlayerInput *pInput)
{
    // Create a temporary character core from the state
    CCharacterCore TempCore;
    TempCore.m_Pos = State.m_Position;
    TempCore.m_Vel = State.m_Velocity;
    
    // Set target if it changed
    if(length(State.m_Target - m_Status.m_Target) > 8.0f)
    {
        SetTarget(State.m_Target);
    }
    
    // Use the main update method
    return UpdateAutopilot(TempCore, pInput);
}
