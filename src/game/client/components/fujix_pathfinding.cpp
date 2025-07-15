#include "fujix_pathfinding.h"
#include <game/mapitems.h>
#include <base/math.h>
#include <algorithm>
#include <cmath>

// =====================================================
// PATH NODE COMPARATOR IMPLEMENTATION
// =====================================================
bool SPathNodeComparator::operator()(int a, int b) const
{
    if(!m_pNodes)
        return false;

    const SPathNode &NodeA = (*m_pNodes)[a];
    const SPathNode &NodeB = (*m_pNodes)[b];

    if(NodeA.m_FCost == NodeB.m_FCost)
        return NodeA.m_HCost > NodeB.m_HCost;

    return NodeA.m_FCost > NodeB.m_FCost;
}

// =====================================================
// MAP ANALYZER IMPLEMENTATION
// =====================================================
ETileType CMapAnalyzer::ClassifyTile(int TileIndex, int FrontTileIndex)
{
    // Check front layer first (higher priority)
    if(FrontTileIndex != 0)
    {
        switch(FrontTileIndex)
        {
            case TILE_FREEZE:
            case TILE_DFREEZE:
            case TILE_LFREEZE:
                return TILE_TYPE_FREEZE;
            case TILE_DEATH:
                return TILE_TYPE_DEATH;
            case TILE_SOLID:
                return TILE_TYPE_SOLID;
            case TILE_NOHOOK:
                return TILE_TYPE_NOHOOK;
        }
    }
    
    // Check game layer
    switch(TileIndex)
    {
        case TILE_FREEZE:
        case TILE_DFREEZE:
        case TILE_LFREEZE:
            return TILE_TYPE_FREEZE;
        case TILE_DEATH:
            return TILE_TYPE_DEATH;
        case TILE_SOLID:
            return TILE_TYPE_SOLID;
        case TILE_NOHOOK:
            return TILE_TYPE_NOHOOK;
        case TILE_THROUGH:
        case TILE_THROUGH_ALL:
        case TILE_THROUGH_DIR:
            return TILE_TYPE_PLATFORM;
        case TILE_AIR:
        default:
            return TILE_TYPE_SAFE;
    }
}

SMapCell CMapAnalyzer::AnalyzeCell(vec2 Pos)
{
    return AnalyzeCell((int)Pos.x, (int)Pos.y);
}

SMapCell CMapAnalyzer::AnalyzeCell(int x, int y)
{
    if(!m_pCollision)
        return SMapCell(); // Return safe default
    
    int CacheKey = GetCacheKey(x, y);
    auto it = m_CachedCells.find(CacheKey);
    if(it != m_CachedCells.end())
        return it->second;
    
    SMapCell Cell;
    
    // Get tile indices
    int TileIndex = m_pCollision->GetTile(x, y);
    int FrontTileIndex = m_pCollision->GetFrontTile(x, y);
    
    // Classify tile type
    Cell.m_Type = ClassifyTile(TileIndex, FrontTileIndex);
    
    // Determine passability
    switch(Cell.m_Type)
    {
        case TILE_TYPE_SOLID:
        case TILE_TYPE_DEATH:
        case TILE_TYPE_FREEZE:
            Cell.m_IsPassable = false;
            Cell.m_MovementCost = 1000.0f; // Very high cost
            break;
        case TILE_TYPE_PLATFORM:
            Cell.m_IsPassable = true;
            Cell.m_MovementCost = 1.5f; // Slightly higher cost
            break;
        case TILE_TYPE_NOHOOK:
            Cell.m_IsPassable = true;
            Cell.m_IsHookable = false;
            Cell.m_MovementCost = 1.2f;
            break;
        case TILE_TYPE_SAFE:
        default:
            Cell.m_IsPassable = true;
            Cell.m_MovementCost = 1.0f;
            break;
    }
    
    // Special handling for hookability
    if(Cell.m_Type == TILE_TYPE_NOHOOK)
        Cell.m_IsHookable = false;
    else
        Cell.m_IsHookable = (Cell.m_Type == TILE_TYPE_SOLID || Cell.m_Type == TILE_TYPE_HOOKABLE);
    
    // Cache the result
    m_CachedCells[CacheKey] = Cell;
    
    return Cell;
}

bool CMapAnalyzer::IsSafePosition(vec2 Pos, float Radius)
{
    if(!m_pCollision)
        return true;
    
    // Check multiple points around the character's bounding box
    const int CheckPoints = 8;
    const float Step = 2.0f * M_PI / CheckPoints;
    
    for(int i = 0; i < CheckPoints; i++)
    {
        float Angle = i * Step;
        vec2 CheckPos = Pos + vec2(cos(Angle), sin(Angle)) * Radius;
        
        SMapCell Cell = AnalyzeCell(CheckPos);
        if(!Cell.m_IsPassable)
            return false;
    }
    
    // Also check center point
    SMapCell CenterCell = AnalyzeCell(Pos);
    return CenterCell.m_IsPassable;
}

bool CMapAnalyzer::IsPathClear(vec2 From, vec2 To, float Radius)
{
    if(!m_pCollision)
        return true;
    
    vec2 Dir = To - From;
    float Distance = length(Dir);
    
    if(Distance < 1.0f)
        return IsSafePosition(To, Radius);
    
    Dir = normalize(Dir);
    const float StepSize = 16.0f; // Check every 16 units
    int Steps = (int)(Distance / StepSize) + 1;
    
    for(int i = 0; i <= Steps; i++)
    {
        vec2 CheckPos = From + Dir * (i * StepSize);
        if(!IsSafePosition(CheckPos, Radius))
            return false;
    }
    
    return true;
}

float CMapAnalyzer::GetMovementCost(vec2 Pos, float Radius)
{
    if(!m_pCollision)
        return 1.0f;
    
    // Sample multiple points and take the maximum cost
    float MaxCost = 1.0f;
    const int SamplePoints = 4;
    
    for(int i = 0; i < SamplePoints; i++)
    {
        float Angle = (i * 2.0f * M_PI) / SamplePoints;
        vec2 SamplePos = Pos + vec2(cos(Angle), sin(Angle)) * (Radius * 0.7f);
        
        SMapCell Cell = AnalyzeCell(SamplePos);
        MaxCost = std::max(MaxCost, Cell.m_MovementCost);
    }
    
    return MaxCost;
}

vec2 CMapAnalyzer::FindNearestSafePos(vec2 Pos, float SearchRadius)
{
    if(!m_pCollision)
        return Pos;
    
    // If current position is already safe, return it
    if(IsSafePosition(Pos))
        return Pos;
    
    // Search in expanding circles
    const float StepSize = 8.0f;
    int MaxSteps = (int)(SearchRadius / StepSize);
    
    for(int radius = 1; radius <= MaxSteps; radius++)
    {
        float CurrentRadius = radius * StepSize;
        int CirclePoints = (int)(2.0f * M_PI * CurrentRadius / StepSize);
        CirclePoints = std::max(8, CirclePoints); // At least 8 points
        
        for(int i = 0; i < CirclePoints; i++)
        {
            float Angle = (i * 2.0f * M_PI) / CirclePoints;
            vec2 TestPos = Pos + vec2(cos(Angle), sin(Angle)) * CurrentRadius;
            
            if(IsSafePosition(TestPos))
                return TestPos;
        }
    }
    
    // If no safe position found, return original position
    return Pos;
}

// =====================================================
// PATH PLANNER IMPLEMENTATION (skeleton)
// =====================================================
std::vector<SPathNode> CPathPlanner::FindPath(const CCharacterCore &StartCore, vec2 Goal)
{
    std::vector<SPathNode> Path;
    
    if(!m_pMapAnalyzer || !m_pCollision)
        return Path;
    
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
        // This would normally generate all possible moves (left, right, jump, hook, etc.)
        // For brevity, I'll implement a basic version
        
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
// SAFETY SIMULATOR IMPLEMENTATION (skeleton)
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
    if(!pInput || m_Status.m_State == AUTOPILOT_IDLE)
        return false;
    
    UpdateStatus(Core);
    
    switch(m_Status.m_State)
    {
        case AUTOPILOT_PLANNING:
            if(PlanPath(Core))
            {
                m_Status.m_State = AUTOPILOT_EXECUTING;
                m_Status.m_pStatusMessage = "Executing path...";
                m_CurrentStepIndex = 0;
            }
            else
            {
                m_Status.m_State = AUTOPILOT_STUCK;
                m_Status.m_pStatusMessage = "Cannot find path to target";
            }
            break;
            
        case AUTOPILOT_EXECUTING:
            if(!ExecuteStep(Core, pInput))
            {
                // Check if we need to replan
                if(ShouldReplan(Core))
                {
                    m_Status.m_State = AUTOPILOT_PLANNING;
                    m_Status.m_pStatusMessage = "Replanning path...";
                }
                else if(IsStuck(Core))
                {
                    m_Status.m_State = AUTOPILOT_STUCK;
                    m_Status.m_pStatusMessage = "Stuck, trying to recover...";
                }
            }
            break;
            
        case AUTOPILOT_STUCK:
            HandleStuckState(Core);
            break;
            
        case AUTOPILOT_REACHED:
            // Target reached, nothing to do
            return false;
            
        default:
            break;
    }
    
    return true;
}

void CSmartAutopilot::Stop()
{
    m_Status.m_State = AUTOPILOT_IDLE;
    m_Status.m_pStatusMessage = "Idle";
    m_CurrentPath.clear();
    m_CurrentStepIndex = 0;
}

void CSmartAutopilot::Reset()
{
    Stop();
    m_StuckCounter = 0;
    m_LastPlanTick = 0;
    if(m_pMapAnalyzer)
        m_pMapAnalyzer->ClearCache();
}

bool CSmartAutopilot::ShouldReplan(const CCharacterCore &Core) const
{
    // Replan if target changed significantly
    if(length(m_LastTarget - m_Status.m_Target) > 64.0f)
        return true;
    
    // Replan if we deviated significantly from the path
    if(!m_CurrentPath.empty() && m_CurrentStepIndex < (int)m_CurrentPath.size())
    {
        vec2 ExpectedPos = m_CurrentPath[m_CurrentStepIndex].m_Pos;
        if(length(Core.m_Pos - ExpectedPos) > 96.0f)
            return true;
    }
    
    // Replan periodically for long paths
    return m_CurrentPath.size() > 10 && m_CurrentStepIndex > 5;
}

bool CSmartAutopilot::PlanPath(const CCharacterCore &Core)
{
    if(!m_pPathPlanner)
        return false;
    
    // Find safe target position if needed
    vec2 Target = m_Status.m_Target;
    if(m_pMapAnalyzer && !m_pMapAnalyzer->IsSafePosition(Target))
    {
        Target = m_pMapAnalyzer->FindNearestSafePos(Target);
    }
    
    // Plan the path
    m_CurrentPath = m_pPathPlanner->FindPath(Core, Target);
    
    if(!m_CurrentPath.empty())
    {
        m_Status.m_TotalSteps = (int)m_CurrentPath.size();
        m_Status.m_CurrentStep = 0;
        return true;
    }
    
    return false;
}

bool CSmartAutopilot::ExecuteStep(CCharacterCore &Core, CNetObj_PlayerInput *pInput)
{
    if(m_CurrentPath.empty() || m_CurrentStepIndex >= (int)m_CurrentPath.size())
        return false;
    
    // Check if we reached the final target
    if(length(Core.m_Pos - m_Status.m_Target) < 48.0f)
    {
        m_Status.m_State = AUTOPILOT_REACHED;
        m_Status.m_pStatusMessage = "Target reached";
        return true;
    }
    
    // Get current target node
    SPathNode &TargetNode = m_CurrentPath[m_CurrentStepIndex];
    
    // Simple movement logic (this would be much more sophisticated in reality)
    vec2 Dir = TargetNode.m_Pos - Core.m_Pos;
    float Distance = length(Dir);
    
    // Move to next step if we're close enough
    if(Distance < 32.0f)
    {
        m_CurrentStepIndex++;
        m_Status.m_CurrentStep = m_CurrentStepIndex;
        m_Status.m_Progress = (float)m_CurrentStepIndex / std::max(1, (int)m_CurrentPath.size());
        
        if(m_CurrentStepIndex >= (int)m_CurrentPath.size())
        {
            m_Status.m_State = AUTOPILOT_REACHED;
            m_Status.m_pStatusMessage = "Target reached";
            return true;
        }
        return true;
    }
    
    // Basic directional input
    if(Dir.x > 8.0f)
        pInput->m_Direction = 1;
    else if(Dir.x < -8.0f)
        pInput->m_Direction = -1;
    else
        pInput->m_Direction = 0;
    
    // Jump if we need to go up significantly
    if(Dir.y < -32.0f)
        pInput->m_Jump = 1;
    
    return true;
}

void CSmartAutopilot::UpdateStatus(const CCharacterCore &Core)
{
    // Update progress
    if(!m_CurrentPath.empty())
    {
        m_Status.m_Progress = (float)m_CurrentStepIndex / std::max(1, (int)m_CurrentPath.size());
    }
    
    // Check for stuck state
    if(IsStuck(Core))
    {
        m_StuckCounter++;
        if(m_StuckCounter > 50) // Stuck for ~1 second at 50 FPS
        {
            m_Status.m_State = AUTOPILOT_STUCK;
        }
    }
    else
    {
        m_StuckCounter = std::max(0, m_StuckCounter - 1);
    }
}

bool CSmartAutopilot::IsStuck(const CCharacterCore &Core)
{
    // Simple stuck detection - not moving much despite having a path
    if(m_CurrentPath.empty())
        return false;
    
    static vec2 s_LastPos = Core.m_Pos;
    static int s_LastUpdate = 0;
    
    // Check every few frames
    if(s_LastUpdate++ % 10 == 0)
    {
        float Movement = length(Core.m_Pos - s_LastPos);
        s_LastPos = Core.m_Pos;
        
        // If we haven't moved much in 10 frames, we might be stuck
        return Movement < 8.0f;
    }
    
    return false;
}

void CSmartAutopilot::HandleStuckState(const CCharacterCore &Core)
{
    // Try to get unstuck by finding a new path
    m_StuckCounter--;
    
    if(m_StuckCounter <= 0)
    {
        // Try replanning
        m_Status.m_State = AUTOPILOT_PLANNING;
        m_Status.m_pStatusMessage = "Trying new path...";
        m_StuckCounter = 0;
        
        // Clear path cache to force fresh planning
        if(m_pMapAnalyzer)
            m_pMapAnalyzer->ClearCache();
    }
}
