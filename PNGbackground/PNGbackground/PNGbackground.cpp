// PNGbackground.cpp : Defines the entry point for the application.
//
// BackgroundTest.cpp
// Win32 + GDI+
// Loads and displays a 900x700 PNG as a fixed background
// ============================================================

#include <windows.h>
#undef min
#undef max
#include <cstdlib>
#include <gdiplus.h>
#include <cwchar>
#include <vector>
#include <ctime>
#include <fstream>
#include <queue>
#include <algorithm>
#include <random>
#include <string>
#include <unordered_map>


#pragma comment(lib, "gdiplus.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

using namespace Gdiplus;

// ------------------------------------------------------------
// Globals
// ------------------------------------------------------------
static const int GAME_W = 900;
static const int GAME_H = 700;
static const int HUD_H = 80;
static const int SCREEN_W = GAME_W;
static const int SCREEN_H = GAME_H + HUD_H;
//scaling
static float g_scale = 1.0f;
static int g_windowW = SCREEN_W;
static int g_windowH = SCREEN_H;
bool g_isOver = false;
// Back buffer globals
static HDC     g_backDC = nullptr;
static HBITMAP g_backBitmap = nullptr;
static HBITMAP g_backOldBmp = nullptr;
static int     g_backW = 0;
static int     g_backH = 0;
// Timing globals
static LARGE_INTEGER g_qpcFreq;
static LARGE_INTEGER g_lastTime;
static float g_deltaTime = 0.0f;
// GDI+ globals
ULONG_PTR g_gdiplusToken = 0;
Image* g_background = nullptr;


// Overlay grid
static const int GRID_COLS = 45;
static const int GRID_ROWS = 35;
static const int CELL_SIZE = 20;
static int g_hoverTileX = -1;
static int g_hoverTileY = -1;
// Pathfinding globals
std::vector<std::string> g_pathFiles;
int g_pathIndex = 0;
bool g_pathCompleted = false;
bool RestartPath;
// Wave globals
float g_waveTime = 0.0f;
bool  g_waveActive = false;
static int g_minWave = 0; //min wave for this path
static int g_maxWave = 0; //max wave for this path
static int g_waves = 0; // number of waves to iterate 1-9

// Random seed
static unsigned int g_rngSeed = 0;
// Resource Globals
static float g_resource = 100.0f;
static float g_resourceCap = 500.0f;
static float g_resourcePerSecond = 0.0f;
static float g_resourceTimer = 0.0f;
static float g_resourceLast = 0.0f;
// Kownledge Globals
static size_t Ktype = 3;
static size_t Ktier = 4;
static int g_Kcount = 9; // Kowledge available at start of game
//Tower Globals
static int g_TowerType = -1;
std::vector<RectF> g_buildSlots; // Stores the rectangles
bool isBlocked(int tileX, int tileY);

static std::wstring g_assetsPath; // must be set to exe directory in WinMain
static std::unordered_map<std::wstring, Image*> g_spriteCache;

Image* GetSprite(const std::wstring& name)
{
    if (name.empty())
        return nullptr;

    auto it = g_spriteCache.find(name);
    if (it != g_spriteCache.end())
        return it->second;

    std::wstring fullPath = g_assetsPath + name;
    Image* img = Image::FromFile(fullPath.c_str());
    if (!img || img->GetLastStatus() != Ok)
    {
        delete img;
        g_spriteCache[name] = nullptr; // remember missing so we don't keep trying
        return nullptr;
    }

    g_spriteCache[name] = img;
    return img;
}

void CleanupSprites()
{
    for (auto& kv : g_spriteCache)
        delete kv.second;
    g_spriteCache.clear();
}
// ------------------------------------------------------------
// Back Buffer Initialization
// ------------------------------------------------------------
void InitBackBuffer(HWND hwnd, int width, int height)
{
    HDC windowDC = GetDC(hwnd);

    g_backDC = CreateCompatibleDC(windowDC);
    g_backBitmap = CreateCompatibleBitmap(windowDC, width, height);
    g_backOldBmp = (HBITMAP)SelectObject(g_backDC, g_backBitmap);

    g_backW = width;
    g_backH = height;

    ReleaseDC(hwnd, windowDC);
}
// Path files list
void LoadPathFiles()
{
    g_pathFiles.clear();

        g_pathFiles.push_back("Path-0.txt");
        g_pathFiles.push_back("Path-1.txt");
        g_pathFiles.push_back("Path-2.txt");
        g_pathFiles.push_back("Path-3.txt");
        g_pathFiles.push_back("Path-4.txt");
        g_pathFiles.push_back("Path-5.txt");
        g_pathFiles.push_back("Path-6.txt");
        g_pathFiles.push_back("Path-7.txt");
        g_pathFiles.push_back("Path-L-0.txt");
        g_pathFiles.push_back("Path-L-1.txt");
        g_pathFiles.push_back("Path-L-2.txt");
        g_pathFiles.push_back("Path-L-3.txt");

        g_pathFiles.push_back("Path-LR-0.txt");
        g_pathFiles.push_back("Path-LR-1.txt");
        g_pathFiles.push_back("Path-LR-2.txt");
        g_pathFiles.push_back("Path-LR-3.txt");
        g_pathFiles.push_back("Path-LR-4.txt");
        g_pathFiles.push_back("Path-LT-0.txt");
        g_pathFiles.push_back("Path-LT-1.txt");
        g_pathFiles.push_back("Path-LT-2.txt");
        g_pathFiles.push_back("Path-LT-3.txt");
        g_pathFiles.push_back("Path-RT-0.txt");
        g_pathFiles.push_back("Path-RT-1.txt");
        g_pathFiles.push_back("Path-RT-2.txt");
        g_pathFiles.push_back("Path-RT-3.txt");
        
        g_pathFiles.push_back("Path-R-0.txt");
        g_pathFiles.push_back("Path-R-1.txt");
        g_pathFiles.push_back("Path-R-2.txt");
        g_pathFiles.push_back("Path-R-3.txt");
        g_pathFiles.push_back("Path-X-0.txt");
        g_pathFiles.push_back("Path-X-1.txt");
        g_pathFiles.push_back("Path-X-2.txt");
        g_pathFiles.push_back("Path-X-3.txt");
        g_pathFiles.push_back("Path-X-4.txt");
        g_pathFiles.push_back("Path-X-5.txt");
        g_pathFiles.push_back("Path-X-6.txt");
        g_pathFiles.push_back("Path-X-7.txt");
        
		g_pathIndex = 0;
}
void ShufflePaths()
{
    for (int i = (int)g_pathFiles.size() - 1; i > 0; --i)
    {
        int j = rand() % (i + 1);

        std::swap(g_pathFiles[i], g_pathFiles[j]);
    }
}

// ------------------------------------------------------------
// Path Overlay Parsing and Rendering
// ------------------------------------------------------------
static std::vector<std::string> g_pathData;

bool LoadPathFile(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        MessageBoxA(nullptr, "Failed to open path file", "Load Error", MB_OK);
        return false;
    }

    g_pathData.clear();
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
            g_pathData.push_back(line);
    }

    return true;
}

struct Tile
{
    bool isPath = false;
    bool isBuildable = false;
    bool hasKnowledge = false;
    int distToExit = -1;
    int pathWidth = 0;
    float flowDX = 0.0f;
    float flowDY = 0.0f;
    std::vector<int> neighborsDirs;
};
static Tile g_pathGrid[GRID_ROWS][GRID_COLS];

Tile ParseTileChar(char c) //Path and Wave data encoding:
{
    Tile t{};

    switch (c)
    {
    case '0': // empty
		break;

    case 'p': // path only
        t.isPath = true;
        break;

    case 'b': // build only
        t.isBuildable = true;
        break;

    case 'B': // path + build
        t.isPath = true;
        t.isBuildable = true;
        break;

    case 'k': // knowledge + build
        t.isBuildable = true;
        t.hasKnowledge = true;
        break;

    case 'K': // knowledge + path
        t.isPath = true;
        t.isBuildable = true;
        t.hasKnowledge = true;
        break;
		// Wave range indicators (set global min/max wave for this path)
	case 'c': g_minWave = 0; break;
    case 'd': g_minWave = 1; break;
    case 'e': g_minWave = 2; break;
	case 'f': g_minWave = 3; break; 
    case 'g': g_minWave = 4; break;

    case 'C': g_maxWave = 0; break;
    case 'D': g_maxWave = 1; break;
    case 'E': g_maxWave = 2; break;
    case 'F': g_maxWave = 3; break;
    case 'G': g_maxWave = 4; break;

	case '1': g_waves = 1; break;
	case '2': g_waves = 2; break;
	case '3': g_waves = 3; break;
	case '4': g_waves = 4; break;
	case '5': g_waves = 5; break;
	case '6': g_waves = 6; break;
	case '7': g_waves = 7; break;
	case '8': g_waves = 8; break;
	case '9': g_waves = 9; break;

    default:
        MessageBoxA(nullptr, "Invalid tile character in path data", "Parse Error", MB_OK);
        exit(1);
    }
    return t;
}

void ParsePathOverlay(Tile grid[GRID_ROWS][GRID_COLS])
{
    if (g_pathData.size() != GRID_ROWS)
    {
        MessageBoxA(nullptr, "Row count mismatch in path data", "Parse Error", MB_OK);
        ExitProcess(1);
    }

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        const std::string& line = g_pathData[row];

        if (line.length() != GRID_COLS + 1 || line.back() != ',')
        {
            MessageBoxA(nullptr, "Invalid line format in path data", "Parse Error", MB_OK);
            ExitProcess(1);
        }

        for (int col = 0; col < GRID_COLS; ++col)
            grid[row][col] = ParseTileChar(line[col]);
    }
}

void BuildNeighbors()
{
    const int dxs[4] = { 1, 0, -1, 0 };
    const int dys[4] = { 0, 1, 0, -1 };

    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            Tile& t = g_pathGrid[y][x];
            t.neighborsDirs.clear();

            // Only path tiles that can reach the exit participate
            if (!t.isPath || t.distToExit < 0)
                continue;

            for (int i = 0; i < 4; ++i)
            {
                int nx = x + dxs[i];
                int ny = y + dys[i];

                if (nx < 0 || ny < 0 || nx >= GRID_COLS || ny >= GRID_ROWS)
                    continue;

                const Tile& n = g_pathGrid[ny][nx];

                // Must be path and must be able to reach exit
                if (!n.isPath || n.distToExit < 0)
                    continue;

                // Valid forward continuation
                t.neighborsDirs.push_back(i);
            }
        }
    }
}

void BuildDistanceField()
{
    std::queue<std::pair<int, int>> q;
    //Distance from exit
    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            g_pathGrid[y][x].distToExit = -1;
        }
    }
    // Find exit tiles (bottom row)
    for (int x = 0; x < GRID_COLS; ++x)
    {
        if (g_pathGrid[GRID_ROWS - 1][x].isPath)
        {
            g_pathGrid[GRID_ROWS - 1][x].distToExit = 0;
            q.push({ x, GRID_ROWS - 1 });
        }
    }
    // BFS to compute distances
    const int dx[4] = { 1, 0, -1, 0 };
    const int dy[4] = { 0, 1, 0, -1 };

    while (!q.empty())
    {
        std::pair<int, int> tile = q.front();
        int x = tile.first;
        int y = tile.second;
        q.pop();
        int curDist = g_pathGrid[y][x].distToExit;

        for (int i = 0; i < 4; ++i)
        {
            int nx = x + dx[i];
            int ny = y + dy[i];

            if (nx < 0 || ny < 0 || nx >= GRID_COLS || ny >= GRID_ROWS)
                continue;

            Tile& neighbor = g_pathGrid[ny][nx];

            if (!neighbor.isPath)
                continue;

            // Skip already visited
            if (neighbor.distToExit != -1)
                continue;

            neighbor.distToExit = curDist + 1;
            q.push({ nx, ny });
        }
    }
}
void DrawPathOverlay(Graphics& gfx, const Tile grid[GRID_ROWS][GRID_COLS], int cellSize)
{
    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            const Tile& t = grid[y][x];

            if (!t.isPath && !t.isBuildable && !t.hasKnowledge)continue;


            Rect r(
                x * cellSize,
                y * cellSize,
                cellSize,
                cellSize);

            if (t.hasKnowledge && g_Kcount > 0) {
                SolidBrush brush(Color(255, 200, 0, 200));//purple
                gfx.FillRectangle(&brush, r);
            }
            else if (!t.isPath && t.isBuildable) { continue; }
            else if ((t.isPath && t.isBuildable) || t.isPath) {
                SolidBrush brush(Color(60, 0, 0, 255));//blue
                gfx.FillRectangle(&brush, r);
            }
        }
    }
}

//------------------------------------------------------------
// Knowledge System
//------------------------------------------------------------
struct Knowledge
{
    bool unlocked = false;
    bool Active = false;
};
struct WorldKnowledge
{
    int type = 0;   // knowledge line (0..Ktype-1)
    int tier = 0;   // tier (1..Ktier-1)
    bool acquired = false;
};

std::vector < Knowledge> g_knowledge;
std::vector<WorldKnowledge> g_worldKnowledge;// Dynamic knowledge

size_t idx(int type, int tier) //column centric indexing
{
    return static_cast<size_t>(tier) * Ktype
        + static_cast<size_t>(type);
}
void InitKnowledge()
{

	g_knowledge.clear();

    // Reserve flat vector capacity to avoid reallocations
    g_knowledge.resize(Ktype * Ktier);

    for (int type = 0; type < Ktype; ++type)
    {
        for (int tier = 0; tier < Ktier; ++tier)
        {
            Knowledge& k = g_knowledge[idx(type, tier)];

            if (tier == 0)
            {
                k.unlocked = true;
                k.Active = true;
            }
            else
            {
                k.unlocked = false;
                k.Active = false;
            }
        }
    }
	//shuffle available knowledge for this run
    g_worldKnowledge.clear();
    std::vector<std::pair<int, int>> pool;
    for (int type = 0; type < (int)Ktype; ++type)
    {
        for (int tier = 1; tier < (int)Ktier; ++tier)
        {
            pool.push_back({ type, tier });
        }
    }
    std::shuffle(pool.begin(), pool.end(), std::mt19937(g_rngSeed + 1));
    for (int i = 0; i < 9 && i < (int)pool.size(); ++i)
    {
        g_worldKnowledge.push_back({ pool[i].first, pool[i].second, false });
    }
}

void ActivateKnowledge(int type, int tier)
{
    // Unlock first
    g_knowledge[idx(type, tier)].unlocked = true;

    // Recompute activation for this entire type line
    for (int t = 1; t < Ktier; ++t)
    {
        Knowledge& current = g_knowledge[idx(type, t)];
        Knowledge& prev = g_knowledge[idx(type, t - 1)];
        current.Active = (current.unlocked && prev.Active);
    }
}
bool AcquireNextWorldKnowledge()
{
    for (auto& wk : g_worldKnowledge)
    {
        if (!wk.acquired)
        {
            ActivateKnowledge(wk.type, wk.tier);
            wk.acquired = true;
            g_Kcount--;
            return true;
        }
    }
    return false; // nothing left
}

// ------------------------------------------------------------
// Enemy
// ------------------------------------------------------------

struct Enemy
{
    float x, y;        // world position (pixels)
    int tileX, tileY;  // current grid cell
    int nextTileX, nextTileY;
    int prevDX, prevDY; // previous movement direction
    float speed;       // pixels per second
    int level;             // 0-4, tier level
    float emissionRadius;  // contamination radius
    float maxDmg;       // max damage
    float displaySize;     // radius for rendering
    Gdiplus::Color color;        // color for rendering
    int HP = 0;               // health points
    int mHP = 0;              // max health points
    int commitsLeft = 0;    // steps left in committed direction
    int committedDir = -1;  // committed direction index
};

struct SpawnPoint
{
    int tileX;
    int tileY;
};

std::vector<Enemy> g_enemies;
std::vector<SpawnPoint> g_topSpawns;

void CollectSpawnPoints()
{
    g_topSpawns.clear();

    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            if (!g_pathGrid[y][x].isPath)
                continue;

            // Top edge (primary)
            if (y == 0)
            {
                g_topSpawns.push_back({ x, y });
            }

            // Left edge (secondary)
            if (x == 0)
            {
                g_topSpawns.push_back({ x, y });
            }

            // Right edge (secondary)
            if (x == GRID_COLS - 1)
            {
                g_topSpawns.push_back({ x, y });
            }
        }
    }
}

SpawnPoint GetRandomSpawn()
{
    if (!g_topSpawns.empty()) {
        return g_topSpawns[rand() % g_topSpawns.size()];
    }
    else {
        MessageBoxA(nullptr, "No valid spawn points", "Spawn Error", MB_OK);
        ExitProcess(1);
    }
}
bool FindNextPathTile(
    int x, int y,
    int& outX, int& outY,
    int prevDX, int prevDY,
    int& outDX, int& outDY,
    int idx,
    int& commitsLeft,
    int& committedDir)
{
    const int dxs[4] = { 1, 0, -1, 0 };
    const int dys[4] = { 0, 1, 0, -1 };

    Tile& t = g_pathGrid[y][x];

    // Tile must be a valid path tile with neighbors
    if (!t.isPath || t.distToExit < 0)
        return false;

    std::vector<int> candidates;

    // Use precomputed neighbors
    for (int dir : t.neighborsDirs)
    {
        // Avoid trivial backtracking
        if (dxs[dir] == -prevDX && dys[dir] == -prevDY)
            continue;

        // Optional: allow slight uphill or lateral moves
        int nx = x + dxs[dir];
        int ny = y + dys[dir];
        Tile& n = g_pathGrid[ny][nx];

        if (n.distToExit <= t.distToExit + 1) // forward or lateral
            candidates.push_back(dir);
    }
    // If no candidates due to backtracking restriction, allow all valid neighbors
    if (candidates.empty())
    {
        for (int dir : t.neighborsDirs)
        {
            int nx = x + dxs[dir];
            int ny = y + dys[dir];
            Tile& n = g_pathGrid[ny][nx];

            if (!n.isPath || n.distToExit < 0)
                continue;

            candidates.push_back(dir); // include even if backtracking
        }
    }

    // If we are committed to a direction, keep moving that way
    if (commitsLeft > 0)
    {
        for (int dir : candidates)
        {
            if (dir == committedDir)
            {
                outDX = dxs[dir];
                outDY = dys[dir];
                outX = x + outDX;
                outY = y + outDY;
                commitsLeft--;
                return true;
            }
        }
        // If committed direction is blocked, reset commitment
        commitsLeft = 0;
    }

    // Junction: pick one candidate randomly and commit
    if (!candidates.empty())
    {
        std::vector<int> downhill;
        for (int dir : candidates)
        {
            int nx = x + dxs[dir];
            int ny = y + dys[dir];
            Tile& n = g_pathGrid[ny][nx];

            if (n.distToExit < t.distToExit)
                downhill.push_back(dir);
        }

        // If there are downhill options, pick from them; otherwise pick from all candidates
        const std::vector<int>& pool = !downhill.empty() ? downhill : candidates;

        int choice = rand() % pool.size();
        int dir = pool[choice];

        outDX = dxs[dir];
        outDY = dys[dir];
        outX = x + outDX;
        outY = y + outDY;

        // Commit for next N steps (random between 2-5 for variety)
        commitsLeft = 2 + rand() % 4;
        committedDir = dir;

        return true;
    }

    return false; // completely blocked
}

float TileCenterX(int tileX)
{
    return tileX * CELL_SIZE + CELL_SIZE * 0.5f;
}

float TileCenterY(int tileY)
{
    return tileY * CELL_SIZE + CELL_SIZE * 0.5f;
}

void SpawnEnemy(int count = 1, int level = 0) // level 0-4
{
    for (int i = 0; i < count; ++i)
    {
        SpawnPoint sp = GetRandomSpawn();

        Enemy e;
        e.level = level;
        e.prevDX = 0;
        e.prevDY = 1; // initial downward movement
        e.tileX = sp.tileX;
        e.tileY = sp.tileY;
        e.nextTileX = sp.tileX;
        e.nextTileY = sp.tileY;
        e.x = TileCenterX(e.tileX);
        e.y = TileCenterY(e.tileY);

        // Tiered stats per level
        switch (level)
        {
        case 0: e.speed = 30; e.emissionRadius = 30; e.maxDmg = 3; e.displaySize = 6;
			e.color = Color(255, 200, 200, 200); e.HP = 10; break;//white
        case 1: e.speed = 18; e.emissionRadius = 40; e.maxDmg = 4; e.displaySize = 8;
			e.color = Color(255, 100, 255, 100); e.HP = 20; break;//green
        case 2: e.speed = 25; e.emissionRadius = 50; e.maxDmg = 5; e.displaySize = 10;
			e.color = Color(255, 100, 100, 200); e.HP = 30; break;//blue
        case 3: e.speed = 20; e.emissionRadius = 40; e.maxDmg = 6; e.displaySize = 12;
            e.color = Color(255, 255, 0, 0); e.HP = 40; break;//red
        case 4: e.speed = 19; e.emissionRadius = 60; e.maxDmg = 10; e.displaySize = 16;
			e.color = Color(255, 255, 0, 255); e.HP = 50; break;//magenta
        }
        e.mHP = e.HP;

        g_enemies.push_back(e);
    }
}

void UpdateEnemy(Enemy& e, float dt)
{
    // --- Check for barrier at next tile ---
    if (isBlocked(e.nextTileX, e.nextTileY))
    {
        // Enemy waits at current tile until barrier destroyed
        e.nextTileX = e.tileX;
        e.nextTileY = e.tileY;
        return; // skip movement this frame
    }
    float targetX = TileCenterX(e.nextTileX);
    float targetY = TileCenterY(e.nextTileY);

    float dx = targetX - e.x;
    float dy = targetY - e.y;
    float dist = sqrtf(dx * dx + dy * dy);
    int oldDX = e.prevDX;
    int oldDY = e.prevDY;

    float move = e.speed * dt;
    // ---- arrive at tile ----
    if (move >= dist)
    {
        // snap to center
        e.x = targetX;
        e.y = targetY;

        e.tileX = e.nextTileX;
        e.tileY = e.nextTileY;

        int outDX = 0;
        int outDY = 0;
        int idx = &e - &g_enemies[0];  // compute index in vector

        if (FindNextPathTile(
            e.tileX, e.tileY,
            e.nextTileX, e.nextTileY,
            e.prevDX, e.prevDY,
            outDX, outDY, idx,
            e.commitsLeft, e.committedDir))
        {
            // Only update intent if a real decision happened
            e.prevDX = outDX;
            e.prevDY = outDY;
        }
        else
        {
            // reached exit or dead end
            move = 0.0f;
        }
        return;
    }

    // ---- move toward next tile ----
    e.x += (dx / dist) * move;
    e.y += (dy / dist) * move;
}

void UpdateEnemies(float dt)
{
    for (auto& e : g_enemies)
    {
        UpdateEnemy(e, dt);
    }

    // Remove dead enemies
    for (int i = (int)g_enemies.size() - 1; i >= 0; --i)
    {
        if (g_enemies[i].HP <= 0)
        {
            g_enemies.erase(g_enemies.begin() + i);
        }
    }
}

void DrawContamination(HDC hdc)
{
    Graphics g(hdc);

    for (const auto& e : g_enemies)
    {
        // Calculate alpha (clamp to 128)
        float hpFraction = float(e.HP) / float(e.mHP);
        int alpha = int(100 * hpFraction);
        if (alpha > 128) alpha = 128;

        // Semi-transparent greenish contamination
        Color c(alpha, 0, 255, 0);

        SolidBrush brush(c);

        g.FillEllipse(
            &brush,
            e.x - e.emissionRadius,
            e.y - e.emissionRadius,
            e.emissionRadius * 2,
            e.emissionRadius * 2
        );
    }
}

void DrawEnemies(HDC hdc)
{
    Graphics g(hdc);
    for (const auto& e : g_enemies)
    {
        SolidBrush brush(e.color);
        g.FillEllipse(
            &brush,
            e.x - e.displaySize,
            e.y - e.displaySize,
            e.displaySize * 2,
            e.displaySize * 2
        );
    }
}
//-------------------------------------------------------------
// Wave System
//-------------------------------------------------------------
struct Wave
{
	float startTime; // time when wave starts
    int enemyCount;
    int enemyLevel; // 0-4
    float spawnInterval; // seconds between spawns
};
std::vector<Wave>           g_wave;
std::vector<int>            g_spawnedCount;
std::vector<float>          g_nextSpawnTime;

void StartWave(const std::vector<Wave>& instructions)
{
    g_wave = instructions;

    g_spawnedCount.clear();
    g_nextSpawnTime.clear();

    for (size_t i = 0; i < instructions.size(); ++i)
    {
        g_spawnedCount.push_back(0);
        g_nextSpawnTime.push_back(instructions[i].startTime);
    }

    g_waveTime = 0.0f;
    g_waveActive = true;
}
void ThisWave(int waveNumber)//wave definitions (c-g for wave range, 1-9 for repeats)
{
    std::vector<Wave> wave;

    switch (waveNumber)
    {
	case 0:  //(c)Starttime, enemy count, enemy level, spawn interval
        wave = {
            { 1.5f, 5, 0, 1.5f }// Basic
        };
        break;
    case 1: //(d)
        wave = {
            { 1.5f, 5, 0, 1.0f },
            { 3.0f, 3, 1, 0.5f }
        };
        break;
    case 2: //(e)
        wave = {
            { 0.0f, 8, 0, 0.0f },
            { 2.0f, 4, 1, 0.6f },
            { 4.0f, 2, 2, 1.5f } // burst
        };
        break;
    case 3:  //(f)
        wave = {
            { 1.5f, 10, 0, 0.6f },
            { 3.0f, 6, 1, 1.0f },
            { 6.0f, 2, 3, 6.0f } // mini-boss
        };
        break;
	case 4:  //(g)
        wave = {
            { 2.0f, 3, 1, 1.5f },
            { 5.0f, 2, 4, 5.0f } // boss
        };
		break;
    }
    if (!wave.empty())
        StartWave(wave);
}

void UpdateWave(float deltaTime)
{

    if (!g_waveActive) return;

    g_waveTime += deltaTime;

	static int waveCount = g_waves; // number of waves to iterate through (1-9)
    if (waveCount == 0)
    {
        waveCount = std::max(1, g_waves); // ensure at least one repetition
    }

    bool allDone = true;
    
    for (size_t i = 0; i < g_wave.size(); ++i)
    {
        Wave& instr = g_wave[i];

        if (g_spawnedCount[i] >= instr.enemyCount) continue;

        allDone = false;

        // spawn enemies if it's time
        while (g_waveTime >= g_nextSpawnTime[i] && g_spawnedCount[i] < instr.enemyCount)
        {
            SpawnEnemy(1, instr.enemyLevel);

            g_spawnedCount[i]++;
            g_nextSpawnTime[i] += instr.spawnInterval;

            if (instr.spawnInterval <= 0.0f) break; // burst
        }
	}
    if (allDone)
    {
        waveCount--;
        if (waveCount > 0)
        {   // Prepare next repetition: reset per-instruction counters and schedule times
            for (size_t i = 0; i < g_wave.size(); ++i)
            {
                g_spawnedCount[i] = 0;
                g_nextSpawnTime[i] = g_wave[i].startTime;
            }
            // Reset wave time to begin the next repetition cleanly
            g_waveTime = 0.0f;
            // Keep g_waveActive == true to continue spawning next repetition
        }
        else
        {
            // No repetitions left: deactivate wave system and reset repeatsLeft
            g_waveActive = false;
            waveCount = 0;
        }
    }
}

//-------------------------------------------------------------
// Tower System
//-------------------------------------------------------------
struct Tower
{
    int tileX;
    int tileY;
    float x; // world position (centered)
    float y;
    int type;              // tower type ID
    int level;             // tier 0-4
    float range;           // pixels
    float InfRadius;       // Influence radius
    bool inInf;            // inside radius
    float attackPower;     // HP per attack
    float attackRate;      // attacks per second
    float attackCooldown;  // time until next attack
    float costTotal;  // total resource units required to build
    float costPaid;   // units applied so far
    float maxAmmo;       // max ammo/resource capacity
    float ammo;          // current ammo/resource available for attack
    float ammoPerAttack; // units consumed per attack
    bool operational = false; // fully built and functional
    Gdiplus::Color color;  // visual color
	std::wstring spriteName;        // tower name for UI
    int HP;               // health points
    int maxHP;            // max health points
};

std::vector<Tower> g_towers;

//Bullet
struct Bullet
{
    float x, y;       // current world position
    float startX, startY;   // fire origin
    float maxDist;          // tower range
    Enemy* target;    // pointer to enemy being aimed at
    float speed;      // pixels per second
    float damage;     // attack power
};

std::vector<Bullet> g_bullets;

void InitializeTower(Tower& t)
{
    // reset per-instance runtime state
    t.level = 0;
    t.costPaid = 0.0f;
    t.ammo = 0.0f;

    // Data-driven definitions matching previous switch cases (index == original case value)
    struct TowerDef
    {
        int resType;
        int resLevel;
        float range;
        float InfRadius;
        float attackPower;
        float attackRate;
        int HP;
        float costTotal;
        float maxAmmo;
        float ammoPerAttack;
        Gdiplus::Color color;
        const wchar_t* sprite;
    };

    static const TowerDef defs[] =
    {
        // T,L, rng, InfRad, attPow, attR, HP,  cost, maxAmmo, aPerAt, Color,                         sprite
        { 0, 0, 100.f,  80.f,  0.f,  0.f,  80,  10.0f,  0.0f,   0.05f, Gdiplus::Color(255,120,200,255), L"siphon0.png" }, // case 0
        { 1, 0,   0.f,   0.f,  0.f,  0.f, 300,  15.0f,  0.0f,   0.0f,  Gdiplus::Color(255,160,160,200), L"barrier0.png" }, // case 1
        { 2, 0,  80.f,   0.f, 1.5f,  1.f, 120,  20.0f,  4.0f,  1.50f,  Gdiplus::Color(255,255,80,80),   L"attack0.png"   }, // case 2
        { 0, 1, 120.f, 120.f,  0.f,  0.f, 120,  20.0f,  0.0f,   0.25f, Gdiplus::Color(255,100,200,255), L"siphon1.png"   }, // case 3
        { 1, 1,   0.f,   0.f,  0.f,  0.f, 500,  30.0f,  0.0f,   0.0f,  Gdiplus::Color(255,160,140,220), L"barrier1.png"  }, // case 4
        { 2, 1, 120.f,   0.f,  2.f,  4.f, 150,  30.0f,  6.0f,   2.0f,  Gdiplus::Color(255,255,80,100),  L"attack1.png"   }, // case 5
        { 0, 2, 180.f, 180.f,  0.f,  0.f, 120,  30.0f,  0.0f,   0.5f,  Gdiplus::Color(255,80,200,255),  L"siphon2.png"   }, // case 6
        { 1, 2,   0.f,   0.f,  0.f,  0.f, 900,  40.0f,  0.0f,   0.0f,  Gdiplus::Color(255,160,140,220), L"barrier2.png"  }, // case 7
        { 2, 2, 180.f,   0.f,  3.f,  8.f, 200,  50.0f, 16.0f,   3.0f,  Gdiplus::Color(255,255,80,120),  L"attack2.png"   }  // case 8
    };

    const int defsCount = static_cast<int>(sizeof(defs) / sizeof(defs[0]));
    int idx = t.type;
    if (idx < 0 || idx >= defsCount)
    {
        // fallback to first definition if incoming type is invalid
        idx = 0;
    }

    const TowerDef& d = defs[idx];

    // preserve the original behavior: set t.type to base type and t.level to tier
    t.type = d.resType;
    t.level = d.resLevel;

    // assign remaining fields
    t.range = d.range;
    t.InfRadius = d.InfRadius;
    t.attackPower = d.attackPower;
    t.attackRate = d.attackRate;
    t.HP = d.HP;
    t.maxHP = d.HP;
    t.costTotal = d.costTotal;
    t.maxAmmo = d.maxAmmo;
    t.ammoPerAttack = d.ammoPerAttack;
    t.color = d.color;
    t.spriteName = d.sprite ? std::wstring(d.sprite) : std::wstring();
}
void UpdateInfluence()
{
    for (auto& t : g_towers)
        t.inInf = false;

    for (const auto& src : g_towers)
    {
        if (src.InfRadius <= 0) continue;

        float r2 = src.InfRadius * src.InfRadius;

        for (auto& t : g_towers)
        {
            float dx = t.x - src.x;
            float dy = t.y - src.y;
            if (dx * dx + dy * dy <= r2)
                t.inInf = true;
        }
    }
}

bool PlaceTowerAt(int tileX, int tileY)
{
    // --- Check bounds ---
    if (tileX < 0 || tileX >= GRID_COLS || tileY < 0 || tileY >= GRID_ROWS)
        return false;

    // If no tower is selected (g_TowerType == -1) treat click as "remove tower"
    if (g_TowerType == -1)
    {
        // Find any tower at the clicked cell and remove it.
        for (int i = (int)g_towers.size() - 1; i >= 0; --i)
        {
            if (g_towers[i].tileX == tileX && g_towers[i].tileY == tileY)
            {
                g_towers.erase(g_towers.begin() + i);

                // Recompute derived state (influence overlay etc.)
                UpdateInfluence();

                return true; // removed
            }
        }
        return false; // nothing to remove
    }

    Tile& t = g_pathGrid[tileY][tileX];

    // --- Check if buildable ---
    if (!t.isBuildable)
        return false;

    // --- Check for existing tower ---
    for (const auto& tower : g_towers)
    {
        if (tower.tileX == tileX && tower.tileY == tileY)
            return false;
    }

    // --- Place tower ---
    Tower newTower;
    newTower.tileX = tileX;
    newTower.tileY = tileY;
    newTower.x = TileCenterX(tileX);
    newTower.y = TileCenterY(tileY);
    newTower.type = g_TowerType;

    InitializeTower(newTower);

    g_towers.push_back(newTower);

    // Recompute influence because new tower may affect others
    UpdateInfluence();

    return true;
}

bool PlaceTowerAtMouse(int mouseX, int mouseY)
{
    int tileX = mouseX / CELL_SIZE;
    int tileY = mouseY / CELL_SIZE;
    return PlaceTowerAt(tileX, tileY);
}

bool IsTileInInfluence(int tileX, int tileY)
{
    float cx = TileCenterX(tileX);
    float cy = TileCenterY(tileY);

    for (const auto& t : g_towers)
    {
        if (!t.operational || t.InfRadius <= 0.0f)
            continue;

        float dx = cx - t.x;
        float dy = cy - t.y;
        if (dx * dx + dy * dy <= t.InfRadius * t.InfRadius)
            return true;
    }
    return false;
}
void DrawInfluenceOverlay(HDC hdc)
{
    Graphics g(hdc);

    // Soft tint. Low alpha.
    SolidBrush wash(Color(60, 120, 200, 255)); // bluish, subtle

    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            const Tile& tile = g_pathGrid[y][x];
            if (!tile.isBuildable)
                continue;

            if (!IsTileInInfluence(x, y))
                continue;

            REAL px = x * CELL_SIZE;
            REAL py = y * CELL_SIZE;
            g.FillRectangle(&wash, static_cast<INT>(px), static_cast<INT>(py), CELL_SIZE, CELL_SIZE);
        }
    }
}
void UpdateTower(Tower& t, float dt)
{
	g_resource += 0.01f; // passive income
    // inside influence
    if (!t.inInf) {
        return;
    }
    // --- 1. Build phase: consume resources to complete tower ---
    bool needsInfluence =
        !(t.type == 0 && t.costPaid < t.costTotal); // siphon exception

    if (needsInfluence && !t.inInf)
        return;

    if (t.costPaid < t.costTotal)
    {
        // Amount of resource to allocate this frame
        float buildRate = 0.1f; // example: units/frame
        if (g_resource > 0 && g_resource >= buildRate)
        {
            g_resource -= buildRate;
            t.costPaid += buildRate;
            if (t.costPaid >= t.costTotal) t.operational = true;
        }
    }

    // --- 2. Operational phase ---
    if (!t.operational) {
        return;
    }

    if (t.type == 0) // Siphon Tower (frame-based)
    {
        // Count enemies inside range (frame-based)
        int enemiesInRange = 0;
        float rangeSq = t.range * t.range;
        for (const auto& e : g_enemies)
        {
            float dx = e.x - t.x;
            float dy = e.y - t.y;
            float distSq = dx * dx + dy * dy;
            if (distSq <= rangeSq)
                ++enemiesInRange;
        }

        // Minimum per-frame gain is t.ammoPerAttack.
        // If there are enemies in range, gain = ammoPerAttack * enemiesInRange.
        int multiplier = (enemiesInRange > 0) ? enemiesInRange : 1;
        g_resource += t.ammoPerAttack * static_cast<float>(multiplier);
    }

    // --- 3. Resource-based ammo accumulation ---
    // Example: siphon a small amount of global resource to refill ammo
    if (t.inInf && t.ammo < t.maxAmmo)
    {
        float ammoGain = 0.5f; // example: units/frame
        if (g_resource >= ammoGain)
        {
            g_resource -= ammoGain;
            t.ammo += ammoGain;
            if (t.ammo > t.maxAmmo) t.ammo = t.maxAmmo;
        }
    }

    // --- 4. Attack logic ---
    if (t.type == 2 && t.operational)
    {
        t.attackCooldown -= dt;

        if (t.attackCooldown <= 0.0f && t.ammo >= t.ammoPerAttack)
        {
            // find target in range
            for (auto& e : g_enemies)
            {
                float dx = e.x - t.x;
                float dy = e.y - t.y;
                float distSq = dx * dx + dy * dy;

                if (distSq <= t.range * t.range)
                {
                    // fire
                    Bullet b;
                    b.x = t.x;
                    b.y = t.y;
                    b.startX = t.x;
                    b.startY = t.y;
                    b.maxDist = t.range;
                    b.target = &e;
                    b.speed = 300.0f; // VISUAL ONLY
                    b.damage = t.attackPower;

                    g_bullets.push_back(b);

                    t.ammo -= t.ammoPerAttack;
                    t.attackCooldown = 1.0f / t.attackRate;
                    break;
                }
            }
        }
    }
}
void CheckKnowledgeUnlock()
{
    for (const auto& tower : g_towers)
    {
        if (tower.type != 0 || !tower.operational)
            continue;

        float r2 = tower.InfRadius * tower.InfRadius;

        for (int y = 0; y < GRID_ROWS; ++y)
        {
            for (int x = 0; x < GRID_COLS; ++x)
            {
                Tile& tile = g_pathGrid[y][x];
                if (!tile.hasKnowledge)
                    continue;

                float dx = TileCenterX(x) - tower.x;
                float dy = TileCenterY(y) - tower.y;

                if (dx * dx + dy * dy <= r2)
                {
                    if (AcquireNextWorldKnowledge())
                    {
                        tile.hasKnowledge = false; // consume trigger
                    }
                    return; // one unlock per tick is enough
                }
            }
        }
    }
}
void UpdateBullets(float dt)
{
    for (int i = (int)g_bullets.size() - 1; i >= 0; --i)
    {
        Bullet& b = g_bullets[i];
        if (!b.target || b.target->HP <= 0)
        {
            g_bullets.erase(g_bullets.begin() + i);
            continue;
        }
        float dx0 = b.x - b.startX;
        float dy0 = b.y - b.startY;
        float traveledSq = dx0 * dx0 + dy0 * dy0;

        if (traveledSq >= b.maxDist * b.maxDist)
        {
            // Bullet exceeded tower range
            g_bullets.erase(g_bullets.begin() + i);
            continue;
        }
        // Move bullet toward target
        float dx = b.target->x - b.x;
        float dy = b.target->y - b.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist <= b.speed * dt)
        {
            // Bullet hits target
            b.target->HP -= int(b.damage);
            g_bullets.erase(g_bullets.begin() + i);
        }
        else
        {
            b.x += (dx / dist) * b.speed * dt;
            b.y += (dy / dist) * b.speed * dt;
        }
    }
}

void DrawBullets(HDC hdc)
{
    Graphics g(hdc);
    SolidBrush brush(Color(255, 255, 220, 0)); // yellow bullets
    const REAL size = 4.f;

    for (const auto& b : g_bullets)
    {
        g.FillEllipse(&brush, b.x - size / 2, b.y - size / 2, size, size);
    }
}

void DrawTowers()
{
    Graphics g(g_backDC);

    for (const auto& tower : g_towers)
    {
        REAL size = static_cast<REAL>(CELL_SIZE) / 2.0f;
        REAL left = tower.x - size;
        REAL top = tower.y - size;
        REAL width = size * 2;
        REAL height = size * 2;

        // --- Draw tower background (empty or under construction) ---
        SolidBrush bgBrush(Color(255, 80, 80, 80)); // grey for empty
        g.FillRectangle(&bgBrush, left, top, width, height);

        // --- Draw build progress or sprite ---
        if (tower.costPaid > 0 && !tower.operational)
        {
            float progress = tower.costPaid / tower.costTotal;
            REAL filledHeight = height * progress;

            // Fill from bottom up while building (no sprite yet)
            SolidBrush fillBrush(tower.color);
            g.FillRectangle(&fillBrush, left, top + height - filledHeight, width, filledHeight);
        }

        if (tower.operational)
        {
            // Try to draw sprite, scaled to tower rectangle
            Image* sprite = GetSprite(tower.spriteName);
            if (sprite)
            {
                g.DrawImage(sprite, left, top, width, height);
            }
            else
            {
                // Fallback: colored rectangle
                SolidBrush fillBrush(tower.color);
                g.FillRectangle(&fillBrush, left, top, width, height);
            }
        }

        // --- Optional: border ---
        Pen border(Color(255, 255, 255, 255), 2.0f);
        g.DrawRectangle(&border, left, top, width, height);

        // --- Optional: ammo bar overlay ---
        if (tower.operational && tower.maxAmmo > 0.0f)
        {
            float ammoRatio = 0.0f;
            if (tower.ammoPerAttack > 0)
                ammoRatio = tower.ammo / tower.maxAmmo;

            ammoRatio = std::max(0.0f, std::min(1.0f, ammoRatio));

            const REAL barHeight = 4.0f;
            const REAL barPadding = 1.0f;

            REAL barLeft = left + barPadding;
            REAL barTop = top + barPadding;
            REAL barWidth = width - barPadding * 2;
            REAL barFillW = barWidth * ammoRatio;

            // Background bar (dark)
            SolidBrush bg(Color(180, 40, 40, 40));
            g.FillRectangle(&bg, barLeft, barTop, barWidth, barHeight);

            // Filled ammo bar
            SolidBrush fg(Color(255, 255, 220, 0)); // yellow
            g.FillRectangle(&fg, barLeft, barTop, barFillW, barHeight);

            // Optional border
            Pen border(Color(255, 255, 255, 255), 1.0f);
            g.DrawRectangle(&border, barLeft, barTop, barWidth, barHeight);
        }
        // --- Tower HP bar (inside, bottom) ---
        {
            float hpRatio = (float)tower.HP / (float)tower.maxHP;
            if (hpRatio < 0.0f) hpRatio = 0.0f;
            if (hpRatio > 1.0f) hpRatio = 1.0f;

            const REAL barHeight = 4.0f;
            const REAL barMargin = 2.0f;

            REAL barLeft = left + barMargin;
            REAL barWidth = width - barMargin * 2;
            REAL barTop = top + height - barHeight - barMargin;
            REAL barFillW = barWidth * hpRatio;

            // Background
            SolidBrush bg(Color(200, 40, 40, 40));
            g.FillRectangle(&bg, barLeft, barTop, barWidth, barHeight);

            // Foreground (HP)
            Color hpColor(
                255,
                (BYTE)(255 * (1.0f - hpRatio)), // red
                (BYTE)(255 * hpRatio),         // green
                0                               // blue
            );
            SolidBrush fg(hpColor);
            g.FillRectangle(&fg, barLeft, barTop, barFillW, barHeight);
        }
    }
}

Tower GetTowerPrototype(int type)
{
    Tower t;
    t.type = type;
    InitializeTower(t); // fills in color, stats, etc.
    return t;
}

void DrawHoverHighlight(HDC hdc)
{
    Graphics g(hdc);

    if (g_hoverTileY >= 0)
    {
        // --- Existing grid tile hover ---
        const Tile& tile = g_pathGrid[g_hoverTileY][g_hoverTileX];
        if (!tile.isBuildable) return;


        const REAL padding = 2.0f;
        REAL x = g_hoverTileX * static_cast<REAL>(CELL_SIZE) + padding;
        REAL y = g_hoverTileY * static_cast<REAL>(CELL_SIZE) + padding;
        REAL size = static_cast<REAL>(CELL_SIZE) - padding * 2;

        Pen border(Color(255, 255, 255, 255), 2.0f);

        if (g_TowerType < 0)
        {
            // No tower selected white outline only
            g.DrawRectangle(&border, x, y, size, size);
            return;
        }

        Tower proto = GetTowerPrototype(g_TowerType);
        SolidBrush brush(proto.color);

        g.FillRectangle(&brush, x, y, size, size);

        g.DrawRectangle(&border, x, y, size, size);

        // Draw range circle
        Pen rangePen(Color(128, proto.color.GetRed(), proto.color.GetGreen(), proto.color.GetBlue()), 2.0f);
        REAL centerX = TileCenterX(g_hoverTileX);
        REAL centerY = TileCenterY(g_hoverTileY);
        g.DrawEllipse(&rangePen, centerX - proto.range, centerY - proto.range, proto.range * 2, proto.range * 2);
    }
    else if (g_hoverTileY == -1 && g_hoverTileX >= 0 && g_hoverTileX < (int)g_buildSlots.size())
    {
        // --- HUD build square hover ---
        const RectF& r = g_buildSlots[g_hoverTileX];

        Pen border(Color(255, 255, 255, 255), 4.0f);
        g.DrawRectangle(&border, r.X, r.Y, r.Width, r.Height);
    }

}
struct HudPanel {
    REAL x, y, w, h;
};
void DrawHudPanel(Graphics& g, const HudPanel& p)
{
    SolidBrush bg(Color(180, 20, 20, 20));
    Pen border(Color(200, 255, 255, 255), 1.5f);

    g.FillRectangle(&bg, p.x, p.y, p.w, p.h);
    g.DrawRectangle(&border, p.x, p.y, p.w, p.h);
}

void DrawHUD(HDC hdc)
{
    Graphics g(hdc);
    const REAL hudTop = static_cast<REAL>(GAME_H);
    const REAL padding = 10.0f;
    const REAL panelH = HUD_H - padding * 2;
    const REAL panelW = (GAME_W - padding * 4) / 4.0f;

    HudPanel economy{ padding, hudTop + padding, panelW, panelH };
    HudPanel build{ padding * 2 + panelW, hudTop + padding, panelW * 2, panelH };
    HudPanel status{ padding * 3 + panelW * 3, hudTop + padding, panelW * 3, panelH };

    DrawHudPanel(g, economy);
    DrawHudPanel(g, build);
    DrawHudPanel(g, status);

    // Text formatting
    FontFamily ff(L"Segoe UI");
    Font font(&ff, 16.0f, FontStyleBold);
    Font smallFont(&ff, 12.0f, FontStyleRegular);

    SolidBrush text(Color(255, 255, 255, 255));
    SolidBrush gain(Color(255, 120, 255, 120));
    SolidBrush loss(Color(255, 255, 120, 120));

    wchar_t buffer[128];

    // Resource total
    swprintf_s(buffer, L"Resource: %.1f", g_resource);
    g.DrawString(buffer, -1, &font,
        PointF(economy.x + 10.0f, economy.y + 6.0f), &text);

    // Gain per second
    swprintf_s(buffer, L"%+.2f / sec", g_resourcePerSecond);

    SolidBrush& rateBrush =
        (g_resourcePerSecond >= 0.0f) ? gain : loss;

    g.DrawString(buffer, -1, &smallFont,
        PointF(economy.x + 10.0f, economy.y + 32.0f), &rateBrush);

    // BUILD panel: single row of 9 fixed-size squares with equal padding
    const int slots = std::min(9, static_cast<int>(g_knowledge.size()));
    const REAL slotSize = 26.0f;
    
    // Uniform padding on left, right, and top
    REAL paddingX = (build.w - slots * slotSize) / (slots + 1);
    if (paddingX < 2.0f) paddingX = 2.0f; // safety clamp

    REAL paddingY = 6.0f; // same top margin

    REAL x = build.x + paddingX;
    REAL y = build.y + paddingY;

    Pen slotBorder(Color(200, 180, 180, 180), 1.5f);
    SolidBrush unlockedBrush(Color(255, 120, 255, 120));
    SolidBrush lockedBrush(Color(255, 180, 180, 180));

    g_buildSlots.clear(); // clear old frame
    for (int i = 0; i < slots; ++i)
    {
        // Determine prototype sprite for this slot index
        Tower proto = GetTowerPrototype(i); // slot index maps to prototype id (tier*Ktype + type)

        if (g_knowledge[i].unlocked && g_knowledge[i].Active)
        {
            // Draw sprite for active slot (fallback to color if missing)
            Image* icon = GetSprite(proto.spriteName);
            if (icon)
            {
                g.DrawImage(icon, x, y, slotSize, slotSize);
            }
            else
            {
                g.FillRectangle(&unlockedBrush, x, y, slotSize, slotSize);
            }
        }
		else if (!g_knowledge[i].Active)
        {
            // Inactive/locked visual
            SolidBrush& brush = lockedBrush;
            g.FillRectangle(&brush, x, y, slotSize, slotSize);
        }

        // Slot border
        g.DrawRectangle(&slotBorder, x, y, slotSize, slotSize);

        // Store rectangle in vector
        g_buildSlots.emplace_back(x, y, slotSize, slotSize);
        x += slotSize + paddingX; // move to next block
    }
    // LIGHTS under build squares
    const int lightCount = static_cast<int>(g_knowledge.size());
    const REAL lightSize = 8.0f;
    const REAL lightSpacing = 5.0f;

    REAL lightsY = y + slotSize + 10.0f; // 6 pixels below squares
    x = build.x + paddingX;             // reset x to left start

    Pen lightBorder(Color(200, 180, 180, 180), 1.0f);
    SolidBrush grayBrush(Color(255, 100, 100, 100));
    SolidBrush whiteBrush(Color(255, 255, 255, 255));

    for (int i = 0; i < lightCount; ++i)
    {
        if (!g_knowledge[i].unlocked)
        {
            // outlined only
            g.DrawRectangle(&lightBorder, x, lightsY, lightSize, lightSize);
        }
        else if (g_knowledge[i].Active)
        {
            g.FillRectangle(&whiteBrush, x, lightsY, lightSize, lightSize);
        }
        else
        {
            g.FillRectangle(&grayBrush, x, lightsY, lightSize, lightSize);
        }

        x += lightSize + lightSpacing; // move to next light
    }

    // STATUS panel header (reserved space)
    g.DrawString(L"STATUS", -1, &smallFont,
        PointF(status.x + 10.0f, status.y + 6.0f), &text);

    wchar_t pathBuffer[128];
    swprintf_s(pathBuffer, L"Path: %S",
        (g_pathIndex >= 0 && g_pathIndex < (int)g_pathFiles.size()) ?
        g_pathFiles[g_pathIndex].c_str() : "N/A");

    g.DrawString(pathBuffer, -1, &smallFont,
        PointF(status.x + 10.0f, status.y + 28.0f), &text);
}

bool isBlocked(int tileX, int tileY)
{
    for (const auto& t : g_towers)
    {
        if (!t.operational) { continue; }
        if (t.type != 1) { continue; } // only Barrier towers block
        if (t.tileX == tileX && t.tileY == tileY)
            return true;
    }
    return false;
}

void RenderStaticWorld(HWND hwnd)
{
    // Clear buffer
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
    RECT r = { 0, 0, g_backW, g_backH };
    FillRect(g_backDC, &r, black);
    DeleteObject(black);

    // Draw background using GDI+
    if (g_background)
    {
        Graphics g(g_backDC);
        g.SetInterpolationMode(InterpolationModeNearestNeighbor);
        g.DrawImage(g_background, 0, 0, GAME_W, GAME_H);
    }

    // Draw path overlay ONCE
    Graphics g(g_backDC);
    DrawPathOverlay(g, g_pathGrid, CELL_SIZE);
}

void Render(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    PatBlt(g_backDC, 0, 0, g_backW, g_backH, BLACKNESS);
    RenderStaticWorld(hwnd);
    DrawInfluenceOverlay(g_backDC);
    DrawHUD(g_backDC);
    DrawContamination(g_backDC);
    DrawTowers();
    DrawBullets(g_backDC);
    DrawEnemies(g_backDC);
    // Copy static world to screen
    StretchBlt(
        hdc,
        0, 0,
        g_windowW, g_windowH,
        g_backDC,
        0, 0,
        g_backW, g_backH,
        SRCCOPY
    );

    // ---- Dynamic rendering goes here ----
    DrawHoverHighlight(hdc);
    //DrawPathFlow(hdc);

    EndPaint(hwnd, &ps);
}

// ------------------------------------------------------------
// Spawn System
// ------------------------------------------------------------

void UpdateTowers(float dt)
{
    UpdateInfluence();
    if (g_towers.empty()) { g_resource += 0.01f * dt; } // passive income when no towers
    for (auto& tower : g_towers)
    {
        UpdateTower(tower, dt);
    }
}
void ApplyCorruptionDamage(float dt)
{
    for (auto& tower : g_towers)
    {
        if (!tower.operational)
            continue;

        float totalDPS = 0.0f;

        for (const auto& enemy : g_enemies)
        {
            float dx = tower.x - enemy.x;
            float dy = tower.y - enemy.y;
            float distSq = dx * dx + dy * dy;
            float radiusSq = enemy.emissionRadius * enemy.emissionRadius;

            if (distSq <= radiusSq)
            {
                //float dist = sqrtf(distSq);
                //float falloff = 1.0f - (dist / enemy.emissionRadius);
                //totalDPS += enemy.maxDmg * falloff;
                totalDPS += enemy.maxDmg;
            }
        }

        if (totalDPS > 0.0f)
        {
            int damage = (int)(totalDPS * dt);
            if (damage < 1) damage = 1; // ensure visible damage
            tower.HP -= damage;
        }
    }
}
void CleanupDestroyedTowers()
{
    for (int i = (int)g_towers.size() - 1; i >= 0; --i)
    {
        if (g_towers[i].HP <= 0)
        {
            g_towers.erase(g_towers.begin() + i);
        }
    }
}
void UpdateResourceRate(float dt)
{
    g_resourceTimer += dt;

    // Sample once per second
    if (g_resourceTimer >= 1.0f)
    {
        float delta = g_resource - g_resourceLast;
        g_resourcePerSecond = delta / g_resourceTimer;

        g_resourceLast = g_resource;
        g_resourceTimer = 0.0f;
    }
    // Enforce resource bounds to prevent farming/excessive accumulation
    if (g_resource > g_resourceCap)
        g_resource = g_resourceCap;
    if (g_resource < 0.0f)
        g_resource = 0.0f;
}
struct PathState
{
    float resource;
    int Kcount;
    int selectedWave;
    bool waveActive;
    std::vector<Knowledge> PSknowledge;
    std::vector<WorldKnowledge> PSworldKnowledge;
    int PSpathidx;

    // Any other path-specific state
};
PathState g_pathState;

void NextPath()
{
    int selectedWave = g_minWave;
    // --------------------------------
    // RESTART PATH
    // --------------------------------
	bool isRestart = RestartPath; // capture flag state at start of function
    if (isRestart)
    {
        RestartPath = false;
        // Restore snapshot state
        g_resource = g_pathState.resource;
        g_Kcount = g_pathState.Kcount;
        selectedWave = g_pathState.selectedWave;
        g_waveActive = g_pathState.waveActive;
        g_knowledge = g_pathState.PSknowledge;
        g_worldKnowledge = g_pathState.PSworldKnowledge;
        g_pathIndex = g_pathState.PSpathidx;
        QueryPerformanceCounter(&g_lastTime);
    }
    else
    {
        // Normal progression
        g_pathIndex++;
        if (g_pathIndex >= (int)g_pathFiles.size())
        {
            g_isOver = true;
            MessageBoxA(nullptr, "All paths completed!", "Victory", MB_OK);
            return;
        }
    }

    // Load the next path file
    if (!LoadPathFile(g_pathFiles[g_pathIndex].c_str()))
    {
        MessageBoxA(nullptr, "Failed to load path file", "Load Error", MB_OK);
        g_isOver = true;
        return;
    }
    // Rebuild all path-derived data
	ParsePathOverlay(g_pathGrid);
    BuildDistanceField();
    BuildNeighbors();
	CollectSpawnPoints();
    // Reset path-scoped runtime state
    g_enemies.clear();
    g_bullets.clear();
	g_towers.clear();
    g_hoverTileX = -1;
    g_hoverTileY = -1;
    g_pathCompleted = false;
    // reset Influence overlay
    UpdateInfluence();
    // Wave data for new path
    if (!isRestart)
    {
        if (g_maxWave < g_minWave)
            g_maxWave = g_minWave;

        if (g_maxWave > g_minWave)
        {
            selectedWave =
                g_minWave + (rand() % (g_maxWave - g_minWave + 1));
        }
    }
    ThisWave(selectedWave);
// -----------------------------
// Capture snapshot for restart
// -----------------------------
    if (!isRestart) {
        g_pathState.resource = g_resource;
        g_pathState.Kcount = g_Kcount;
        g_pathState.selectedWave = selectedWave;
        g_pathState.waveActive = g_waveActive;
        g_pathState.PSknowledge = g_knowledge;
        g_pathState.PSworldKnowledge = g_worldKnowledge;
        g_pathState.PSpathidx = g_pathIndex;
    }
}

bool CheckWinLose(HWND hwnd)
{
    if (g_isOver) return true;
    // Lose: any enemy reached the bottom
    // Compute the Y position of the bottom of the last row of tiles
    float exitY = TileCenterY(GRID_ROWS - 1);
    for (auto& enemy : g_enemies)
    {
        if (enemy.y >= exitY)
		{  //Rework this section for bool RestartPath instead of game over
           // Pause game updates
            KillTimer(hwnd, 1);
            int result = MessageBox(
                nullptr,
                L"You Lose!\nRestart Level?",
                L"Level Failed",
                MB_YESNO | MB_ICONQUESTION
            );

            if (result == IDYES)
            {
                RestartPath = true;
				g_isOver = false; // allow game loop to continue for restart
				NextPath(); // restart current path
                QueryPerformanceCounter(&g_lastTime); // reset timer delta
                SetTimer(hwnd, 1, 16, nullptr);  //Restart Frame timer
                return false;
            }
            else { return true; } // game over
        }
    }

    // Win: no enemies left
    if (!g_waveActive && g_enemies.empty() && !g_pathCompleted) //Check Wave complete
    {
		g_pathCompleted = true;
		//reset wave state for next path
		g_minWave = 0; // Reset Wave data to default
		g_maxWave = 0;
        NextPath();
        return false; // game continues
    }

    return false; // game continues
}

// ------------------------------------------------------------
// Window Procedure
// ------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        Render(hwnd);
        return 0;
    }
    case WM_TIMER:
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        g_deltaTime =
            float(now.QuadPart - g_lastTime.QuadPart) /
            float(g_qpcFreq.QuadPart);

        g_lastTime = now;

        // --- Update game logic here ---
        UpdateEnemies(g_deltaTime);
        // Check win/lose immediately
        UpdateWave(g_deltaTime);
        if (CheckWinLose(hwnd))
        {
            // Stop the timer to freeze the game
            KillTimer(hwnd, 1);
            return 0;
        }
        UpdateTowers(g_deltaTime);
        CheckKnowledgeUnlock();
        UpdateBullets(g_deltaTime);
        ApplyCorruptionDamage(g_deltaTime);
        CleanupDestroyedTowers();
        UpdateResourceRate(g_deltaTime);
        //update Wave system

        // Request redraw
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);

        // numeric keys: '1'..'9' => prototype indices 0..8, '0' => none (-1)
        if (wParam >= '1' && wParam <= '9')
        {
            g_TowerType = static_cast<int>(wParam - '1'); // '1' -> 0
        }
        else if (wParam == '0')
        {
            g_TowerType = -1; // none / removal
        }

        // numpad support
        if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9)
        {
            g_TowerType = static_cast<int>(wParam - VK_NUMPAD1); // NUMPAD1 -> 0
        }
        else if (wParam == VK_NUMPAD0)
        {
            g_TowerType = -1;
        }

        if (wParam == VK_SPACE) // Pause
        {
            static bool isPaused = false;
            isPaused = !isPaused;
            if (isPaused)
                KillTimer(hwnd, 1);
            else
                SetTimer(hwnd, 1, 16, nullptr);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int mx = (int)(GET_X_LPARAM(lParam) / g_scale);
        int my = (int)(GET_Y_LPARAM(lParam) / g_scale);

        if (my < GAME_H)
        {
            // Hovering grid
            g_hoverTileX = mx / CELL_SIZE;
            g_hoverTileY = my / CELL_SIZE;

            // Clamp to grid bounds
            if (g_hoverTileX < 0) g_hoverTileX = 0;
            if (g_hoverTileX >= GRID_COLS) g_hoverTileX = GRID_COLS - 1;
            if (g_hoverTileY < 0) g_hoverTileY = 0;
            if (g_hoverTileY >= GRID_ROWS) g_hoverTileY = GRID_ROWS - 1;
        }
        else
        {
            g_hoverTileX = -1;
            g_hoverTileY = -1; // indicate HUD
            // Hovering HUD build panel
            for (size_t i = 0; i < g_buildSlots.size(); ++i)
            {
                const RectF& r = g_buildSlots[i];
                if (mx >= r.X && mx <= r.X + r.Width &&
                    my >= r.Y && my <= r.Y + r.Height)
                {
                    g_hoverTileX = static_cast<int>(i); // HUD index
                    break;
                }
            }
            // g_hoverTileY remains -1 to indicate HUD
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int mouseX = (int)(GET_X_LPARAM(lParam) / g_scale);
        int mouseY = (int)(GET_Y_LPARAM(lParam) / g_scale);

        bool clickedSlot = false;
        if (mouseY >= GAME_H)
        {
            // Check if click is in the build panel squares
            for (size_t i = 0; i < g_buildSlots.size(); ++i)
            {
                const RectF& r = g_buildSlots[i];
                if (mouseX >= r.X && mouseX <= r.X + r.Width &&
                    mouseY >= r.Y && mouseY <= r.Y + r.Height)
                {
                    // Tower square clicked4
                    if (g_knowledge[i].Active)
                        g_TowerType = static_cast<int>(i); // select this tower
                    clickedSlot = true;
                    break; // only one square can be clicked
                }
            }

            // If user clicked the HUD but not on a slot, deselect
            if (!clickedSlot)
                g_TowerType = -1;
        }

        PlaceTowerAtMouse(mouseX, mouseY);
        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        // Right click to unselect/deselect current tower prototype
        g_TowerType = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ------------------------------------------------------------
// WinMain
// ------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // Seed controls spawn positions and wave randomness
    g_rngSeed = (unsigned int)time(nullptr);
    srand(g_rngSeed);

    // --- Initialize GDI+ ---
    GdiplusStartupInput gdiplusStartupInput;
    Status gdiStatus = GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    if (gdiStatus != Ok)
    {
        MessageBox(nullptr, L"GdiplusStartup failed", L"Error", MB_OK);
        return -1;
    }

    // --- Register window class ---
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"BGTestClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    // --- Create window sized to match image ---
// --- Get monitor resolution ---
    HMONITOR hMonitor = MonitorFromPoint({ 0,0 }, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    int monitorW = mi.rcMonitor.right - mi.rcMonitor.left;
    int monitorH = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // --- Compute uniform scale ---
    float scaleX = (float)monitorW / SCREEN_W;
    float scaleY = (float)monitorH / SCREEN_H;
    float scale = std::min(scaleX, scaleY);

    // Optional: integer scaling for crisp pixels
    int intScale = (int)scale;
    if (intScale < 1) intScale = 1;
    scale = (float)intScale;

    // --- Compute scaled window size ---
    int windowW = (int)(SCREEN_W * scale);
    int windowH = (int)(SCREEN_H * scale);

    // --- Center window ---
    int posX = (monitorW - windowW) / 2;
    int posY = (monitorH - windowH) / 2;

    // --- Create popup window ---
    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        L"",
        WS_POPUP,
        posX, posY,
        windowW,
        windowH,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    if (!hwnd)
    {
        MessageBox(nullptr, L"CreateWindow failed", L"Error", MB_OK);
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        return -1;
    }

    // --- Build image path from exe directory ---
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(exePath, L'\\');
    
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0'; // keep trailing slash
    }
    g_assetsPath = exePath;
    
    wchar_t imagePath[MAX_PATH] = { 0 };
    wcsncpy_s(imagePath, exePath, _TRUNCATE);
    wcscat_s(imagePath, L"FantasyStraight.png");

    // --- Load PNG (absolute path) ---
    g_background = Image::FromFile(imagePath);

    if (!g_background || g_background->GetLastStatus() != Ok)
    {
        int status = g_background ? static_cast<int>(g_background->GetLastStatus()) : -1;
        wchar_t buf[512];
        swprintf_s(buf, L"Failed to load '%s' (Gdiplus::Status=%d)", imagePath, status);
        MessageBox(hwnd, buf, L"Error", MB_OK);
        // continue anyway or return -1 here if you prefer to exit
    }
    QueryPerformanceFrequency(&g_qpcFreq);
    QueryPerformanceCounter(&g_lastTime);

    LoadPathFiles();
	ShufflePaths(); // randomize path order each run
	g_pathIndex = -1; // will be incremented to 0 in NextPath
	NextPath(); // load first path and build data structures
	// Wave data initialization would go here when implemented
    InitKnowledge();

    // --- Back buffer init ---
    InitBackBuffer(hwnd, SCREEN_W, SCREEN_H);
    RenderStaticWorld(hwnd);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetTimer(hwnd, 1, 16, nullptr); // ~60 FPS

    // --- Message loop ---
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // --- Cleanup ---
    if (g_backDC)
    {
        SelectObject(g_backDC, g_backOldBmp);
        DeleteObject(g_backBitmap);
        DeleteDC(g_backDC);
    }
    delete g_background;
    CleanupSprites();
    Gdiplus::GdiplusShutdown(g_gdiplusToken);

    return 0;
}


