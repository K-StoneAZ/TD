// PNGbackground.cpp : Defines the entry point for the application.
//
// BackgroundTest.cpp
// Win32 + GDI+
// Loads and displays a 900x700 PNG as a fixed background
// ============================================================

#include <windows.h>
#include <gdiplus.h>
#include <cwchar>
#include <vector>
#include <ctime>
#include <queue>

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
// Random seed
static unsigned int g_rngSeed = 0;
// Resource Globals
static float g_resource = 100.0f;
static float g_resourcePerSecond = 0.0f;
static float g_resourceTimer = 0.0f;
static float g_resourceLast = 0.0f;
//Tower Globals
static int g_TowerType = -1;
bool isBlocked(int tileX, int tileY);
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

// ------------------------------------------------------------
// Path Overlay Parsing and Rendering
// ------------------------------------------------------------
static const char* g_pathData =
"0000000000bbbbbbBBBBpppppBBBBBbbbb00000000000,"
"0000000bbbbbBBBBBBBBpppppBBBBBBBBbbb000000000,"
"0000000000bbbBBBBBBBBpppBBBBBBBBbbb0000000000,"
"000000000000000bbbBBBpppppBBBbbbbbb0000000000,"
"000000000000000000bbbBBBpppppBBBbbbbb00000000,"
"000000000000000000000bbbBBBpppppBBBbbb0000000,"
"000000000000000000000000bbbBBBpppppBBBbbb0000,"
"00000000000000000000000000bbbBBBpppppBBBbbb00,"
"00000000000000bbbbbbbbbbbbbbbbBBBpppppBBBbbb0,"
"00000bbbbbbbbbBBBBBBpppppBBBBBBBBBBbbbbb00000,"
"00000bbbbbbbbBBBBBBBpppppBBBBBBBBBBbbbbb00000,"
"00000bbbbbbBBBBBBBBBpppppBBBBBBBBBBbbbbb00000,"
"00000bbbbbBBBBBppppppppppBBBBBbbbbb0000000000,"
"00000bbbbbbbBBBppppppppBBBBBbbbbb000000000000,"
"00000bbbbbBBBppppppppBBBBbbbb0000000000000000,"
"000bbbbbBppppppppBBBbbb0000000000000000000000,"
"000bbbBBBpppKpBBBbbb0000000000000000000000000," //17
"000bbBBpppppBBbb00000000000000000000000000000,"
"000bbBBBBBBBbb0000000000000000000000000000000,"
"0000bbBBBBBBBbb000000000000000000000000000000,"
"000000bbBBBBBBBbb0000000000000000000000000000,"
"00000000bbBBBBBBBbb00000000000000000000000000,"
"0000000000bbBBBBBBBbb000000000000000000000000,"
"0000000000000bbBBBBBBBbb000000000000000000000,"
"0000000000000000bbBBBBBBBbb000000000000000000,"
"00000000000000000bbbbbBBBppppBBBbbbbbb0000000,"
"00000000000000000000k0bbbBBBpppppBBBbbb000000,"
"00000000000000000000000000bbbBBBpppppBBBbbb00,"
"000000000000000000000000000000bbbBBBpppppBBB0,"
"0000000000000000000000bbbBBBpppppBBBbbb000000," //30
"0000000000000000000bbbBBBppppBBBbbbb000000000,"
"0000000000000000000000bbbBBBpppppBBBbbb000000,"
"0000000000000000000000bbbBBBpppppBBBbbb000000,"
"0000000000000000000000bbbBBBpppppBBBbbb000000,"
"0000000000000000000000bbbBBBpppppBBBbbb000000";

struct Tile
{
    bool isPath;
    bool isBuildable;
    bool hasKnowledge;
    int distToExit = -1;
    int pathWidth = 0;
};

static Tile g_pathGrid[GRID_ROWS][GRID_COLS];

Tile ParseTileChar(char c)
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
        t.hasKnowledge = true;
        break;

    default:
        MessageBoxA(nullptr, "Invalid tile character in path data", "Parse Error", MB_OK);
        exit(1);
    }
	return t;
}

void ParsePathOverlay(Tile grid[GRID_ROWS][GRID_COLS])
    {
        int row = 0;
        int col = 0;

        for (const char* p = g_pathData; *p; ++p)
        {
            if (*p == ',')
            {
                if (col != GRID_COLS)
                {
                    MessageBoxA(nullptr, "Column count mismatch in path data", "Parse Error", MB_OK);
                    ExitProcess(1);
                }

                row++;
                col = 0;
                continue;
            }

            if (row >= GRID_ROWS || col >= GRID_COLS)
            {
                MessageBoxA(nullptr, "Path data exceeds grid size", "Parse Error", MB_OK);
                ExitProcess(1);
            }

            grid[row][col] = ParseTileChar(*p);
            col++;
        }

        if (row != GRID_ROWS - 1)
        {
            MessageBoxA(nullptr, "Row count mismatch in path data", "Parse Error", MB_OK);
            ExitProcess(1);
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

    void BuildPathWidthField()
    {
        const int dx[4] = { 1, 0, -1, 0 };
        const int dy[4] = { 0, 1, 0, -1 };

        for (int y = 0; y < GRID_ROWS; ++y)
        {
            for (int x = 0; x < GRID_COLS; ++x)
            {
                Tile& t = g_pathGrid[y][x];
                if (!t.isPath)
                    continue;

                int count = 0;
                for (int i = 0; i < 4; ++i)
                {
                    int nx = x + dx[i];
                    int ny = y + dy[i];

                    if (nx < 0 || ny < 0 || nx >= GRID_COLS || ny >= GRID_ROWS)
                        continue;

                    if (g_pathGrid[ny][nx].isPath)
                        count++;
                }
                t.pathWidth = count;
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

                if (t.hasKnowledge) {
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
		int HP;               // health points
		int mHP;              // max health points
    };

    struct SpawnPoint
    {
        int tileX;
        int tileY;
    };

    std::vector<Enemy> g_enemies;
    std::vector<SpawnPoint> g_topSpawns;
    std::vector<SpawnPoint> g_leftSpawns;
    std::vector<SpawnPoint> g_rightSpawns;

    void CollectSpawnPoints()
    {
        g_topSpawns.clear();
        g_leftSpawns.clear();
        g_rightSpawns.clear();

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
                    g_leftSpawns.push_back({ x, y });
                }

                // Right edge (secondary)
                if (x == GRID_COLS - 1)
                {
                    g_rightSpawns.push_back({ x, y });
                }
            }
        }
    }

    SpawnPoint GetRandomSpawn()
    {
        if (!g_topSpawns.empty())
            return g_topSpawns[rand() % g_topSpawns.size()];

        if (!g_leftSpawns.empty())
            return g_leftSpawns[rand() % g_leftSpawns.size()];

        if (!g_rightSpawns.empty())
            return g_rightSpawns[rand() % g_rightSpawns.size()];

        MessageBoxA(nullptr, "No valid spawn points", "Spawn Error", MB_OK);
        ExitProcess(1);
    }

    bool FindNextPathTile(int x, int y, int& outX, int& outY, int prevDX, int prevDY, int& outDX, int& outDY)
    {
                float bestScore = 1e9f;
                bool found = false;

                const int dx[4] = { 1, 0, -1, 0 };
                const int dy[4] = { 0, 1, 0, -1 };

                int curDist = g_pathGrid[y][x].distToExit;

                for (int i = 0; i < 4; ++i)
                {
                    int nx = x + dx[i];
                    int ny = y + dy[i];

                    if (nx < 0 || ny < 0 || nx >= GRID_COLS || ny >= GRID_ROWS)
                        continue;

                    const Tile& t = g_pathGrid[ny][nx];
                    if (!t.isPath || t.distToExit < 0)
                        continue;

                    if (isBlocked(nx, ny))
						continue;

                    // Must move downhill or stay level
                    int distDelta = t.distToExit - curDist;
                    if (distDelta > 0)
                        continue;

                    // ---- scoring ----
                    float score = 0.0f;

                    // 1. Distance priority
                    score += t.distToExit * 10.0f;

                    // 2. Penalize direction changes
                    if (dx[i] != prevDX || dy[i] != prevDY)
                        score += 2.5f;

                    // 3. Prefer wider paths
                    score -= t.pathWidth * 1.2f;

                    // 4. Tiny deterministic bias breaker
                    score += (dx[i] * 0.01f + dy[i] * 0.02f);

                    if (!found || score < bestScore)
                    {
                        bestScore = score;
                        outX = nx;
                        outY = ny;
                        outDX = dx[i];
                        outDY = dy[i];
                        found = true;
                    }
                }

                return found;
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
    case 0: e.speed = 30; e.emissionRadius = 30; e.maxDmg = 3; e.displaySize = 6; e.color = Color(255, 200, 200, 200); e.HP = 10; break;
    case 1: e.speed = 15; e.emissionRadius = 40; e.maxDmg = 4; e.displaySize = 8; e.color = Color(255, 0, 255, 0); e.HP = 20; break;
    case 2: e.speed = 25; e.emissionRadius = 50; e.maxDmg = 5; e.displaySize = 10; e.color = Color(255, 100, 100, 200); e.HP = 30; break;
    case 3: e.speed = 20; e.emissionRadius = 40; e.maxDmg = 6; e.displaySize = 12; e.color = Color(255, 255, 0, 0); e.HP = 40; break;
    case 4: e.speed = 10; e.emissionRadius = 60; e.maxDmg = 10; e.displaySize = 16; e.color = Color(255, 255, 0, 255); e.HP = 50; break;
    default: e.speed = 10; e.emissionRadius = 40; e.maxDmg = 4; e.displaySize = 8; e.color = Color(255, 0, 255, 0); e.HP = 10; break;
            }
			e.mHP = e.HP;

            g_enemies.push_back(e);
        }
    }

    void UpdateEnemy(Enemy& e, float dt)
    {
        float targetX = TileCenterX(e.nextTileX);
        float targetY = TileCenterY(e.nextTileY);

        float dx = targetX - e.x;
        float dy = targetY - e.y;
        float dist = sqrtf(dx * dx + dy * dy);

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

            if (FindNextPathTile(
                e.tileX, e.tileY,
                e.nextTileX, e.nextTileY,
                e.prevDX, e.prevDY,
                outDX, outDY))
            {
                // update direction memory
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
            // Calculate alpha (clamp to 255)
            float hpFraction = float(e.HP) / float(e.mHP);
            int alpha = int(100 * hpFraction);
            if (alpha > 255) alpha = 255;

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
        t.level = 0;
        t.costPaid = 0.0f; // start building from zero
        t.ammo = 0.0f;     // no ammo initially

        switch (t.type)
        {
            // Siphon Tower
        case 0:
            t.range = 100.f;
            t.attackPower = 0.f;  // resource per attack
            t.HP = 80;
            t.maxHP = t.HP;
            t.costTotal = 10.0f;     // total units required to build
			t.maxAmmo = 0.0f;        // no ammo capacity
            t.ammoPerAttack = 0.05f;  // units consumed/gain per attack
            t.color = Gdiplus::Color(255, 120, 200, 255);
            break;

            // Barrier Tower
        case 1:
            t.range = 0.f;
            t.attackPower = 0.0f;
            t.HP = 300;
            t.maxHP = t.HP;
            t.costTotal = 15.0f;
            t.maxAmmo = 0.0f;        // no ammo capacity
            t.ammoPerAttack = 0.0f;
            t.color = Gdiplus::Color(255, 160, 160, 200);//
            break;

            // Attack Tower
        case 2:
            t.range = 80.f;
            t.attackPower = 1.f;
			t.attackRate = 1.0f; // attacks per second
            t.HP = 100;
            t.maxHP = t.HP;
            t.costTotal = 20.0f;
            t.maxAmmo = 6.0f;        // no ammo capacity
            t.ammoPerAttack = 1.f;
            t.color = Gdiplus::Color(255, 255, 80, 80);
            break;

        default: // fallback
            t.range = 0.f;
            t.attackPower = 0.f;
            t.HP = 1;
            t.costTotal = 1.0f;
            t.ammoPerAttack = 1.0f;
            t.color = Gdiplus::Color(255, 255, 255, 255);
            break;
        }
    }

    bool PlaceTowerAt(int tileX, int tileY)
    {
        // --- Check bounds ---
        if (tileX < 0 || tileX >= GRID_COLS || tileY < 0 || tileY >= GRID_ROWS)
            return false;

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

        return true;
    }

    bool PlaceTowerAtMouse(int mouseX, int mouseY)
    {
        int tileX = mouseX / CELL_SIZE;
        int tileY = mouseY / CELL_SIZE;
        return PlaceTowerAt(tileX, tileY);
    }

    void UpdateTower(Tower& t, float dt)
    {
        // --- 1. Build phase: consume resources to complete tower ---
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
        if (t.ammo < t.maxAmmo)
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

            // --- Draw build progress ---
            if (tower.costPaid > 0)
            {
                float progress = tower.costPaid / tower.costTotal;
                REAL filledHeight = height * progress;

                // Fill from bottom up
                SolidBrush fillBrush(tower.color);
                g.FillRectangle(&fillBrush, left, top + height - filledHeight, width, filledHeight);
            }

            // --- Optional: border ---
            Pen border(Color(255, 255, 255, 255), 2.0f);
            g.DrawRectangle(&border, left, top, width, height);

            // --- Optional: ammo bar overlay ---
            // --- Ammo bar ABOVE tower ---
            if (tower.operational && tower.maxAmmo > 0.0f)
            {
                float ammoRatio = 0.0f;
                if (tower.ammoPerAttack > 0)
                    ammoRatio = tower.ammo / tower.maxAmmo;

                ammoRatio = max(0.0f, min(1.0f, ammoRatio));

                const REAL barHeight = 4.0f;
                const REAL barPadding = 1.0f;

                REAL barLeft = left +barPadding;
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
        if (g_hoverTileX < 0 || g_hoverTileY < 0) return;

        const Tile& tile = g_pathGrid[g_hoverTileY][g_hoverTileX];
        if (!tile.isBuildable) return;

        // Get color from a prototype tower of the selected type
        Tower proto = GetTowerPrototype(g_TowerType);

        Graphics g(hdc);
        SolidBrush brush(proto.color);

        const REAL padding = 2.0f;
        REAL x = g_hoverTileX * static_cast<REAL>(CELL_SIZE) + padding;
        REAL y = g_hoverTileY * static_cast<REAL>(CELL_SIZE) + padding;
        REAL size = static_cast<REAL>(CELL_SIZE) - padding * 2;

        g.FillRectangle(&brush, x, y, size, size);

        // Optional border
        Pen border(Color(255, 255, 255, 255), 2.0f);
        g.DrawRectangle(&border, x, y, size, size);

        // --- Draw range circle ---
        Pen rangePen(Color(128, proto.color.GetRed(), proto.color.GetGreen(), proto.color.GetBlue()), 2.0f);

        REAL centerX = TileCenterX(g_hoverTileX);
        REAL centerY = TileCenterY(g_hoverTileY);
        REAL radius = proto.range;

        g.DrawEllipse(&rangePen, centerX - radius, centerY - radius, radius * 2, radius * 2);

    }

    void DrawHUD(HDC hdc)
    {
        Graphics g(hdc);
        const REAL hudTop = static_cast<REAL>(GAME_H);
        const REAL padding = 10.0f;
        const REAL boxW = 220.0f;
        const REAL boxH = 60.0f;

        // Background panel
        SolidBrush bg(Color(180, 20, 20, 20));
        g.FillRectangle(&bg, padding, hudTop + padding, boxW, boxH);

        Pen border(Color(200, 255, 255, 255), 1.5f);
        g.DrawRectangle(&border, padding, hudTop + padding, boxW, boxH);

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
            PointF(padding + 10.0f, hudTop + padding + 6.0f), &text);

        // Gain per second
        swprintf_s(buffer, L"%+.2f / sec", g_resourcePerSecond);

        SolidBrush& rateBrush =
            (g_resourcePerSecond >= 0.0f) ? gain : loss;

        g.DrawString(buffer, -1, &smallFont,
            PointF(padding + 10.0f, hudTop + padding + 32.0f), &rateBrush);
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
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
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
		DrawHUD(g_backDC);
        DrawTowers();
		DrawContamination(g_backDC);
		DrawBullets(g_backDC);
        DrawEnemies(g_backDC);
        // Copy static world to screen
        BitBlt(
            hdc,
            0, 0,
            g_backW, g_backH,
            g_backDC,
            0, 0,
            SRCCOPY
        );

        // ---- Dynamic rendering goes here ----
		DrawHoverHighlight(hdc);

        EndPaint(hwnd, &ps);
    }

// ------------------------------------------------------------
// Spawn System
// ------------------------------------------------------------

    void UpdateTowers(float dt)
    {
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
    }

    bool CheckWinLose()
    {
        if (g_isOver) return true;
        // Lose: any enemy reached the bottom
        // Compute the Y position of the bottom of the last row of tiles
		float exitY = TileCenterY(GRID_ROWS - 1);
        for (auto& enemy : g_enemies)
        {
            if (enemy.y >= exitY)
            {
                g_isOver = true; // stop game loop actions
                MessageBox(nullptr, L"You Lose!", L"Level Failed", MB_OK);
                return true; // game over
            }
        }

        // Win: no enemies left
        if (g_enemies.empty())
        {
            g_isOver = true; // stop game loop actions
            MessageBox(nullptr, L"You Win!", L"Level Completed", MB_OK);
            return true; // game won
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
        if (CheckWinLose())
        {
            // Stop the timer to freeze the game
            KillTimer(hwnd, 1);
            return 0;
        }
        UpdateTowers(g_deltaTime);
		UpdateBullets(g_deltaTime);
		ApplyCorruptionDamage(g_deltaTime);
		CleanupDestroyedTowers();
		UpdateResourceRate(g_deltaTime);

        // Request redraw
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);

        // Tower selection
        if (wParam == '1') g_TowerType = 0; // Siphon
        if (wParam == '2') g_TowerType = 1; // Barrier
        if (wParam == '3') g_TowerType = 2; // Attack
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        if (my >= GAME_H)
        {
            g_hoverTileX = -1;
            g_hoverTileY = -1;
            return 0;
        }

        g_hoverTileX = mx / CELL_SIZE;
        g_hoverTileY = my / CELL_SIZE;

        // Clamp to grid bounds
        if (g_hoverTileX < 0) g_hoverTileX = 0;
        if (g_hoverTileX >= GRID_COLS) g_hoverTileX = GRID_COLS - 1;
        if (g_hoverTileY < 0) g_hoverTileY = 0;
        if (g_hoverTileY >= GRID_ROWS) g_hoverTileY = GRID_ROWS - 1;

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int mouseX = GET_X_LPARAM(lParam);
        int mouseY = GET_Y_LPARAM(lParam);
        if (mouseY >= GAME_H) {
            return 0;   // HUD click (handled later if needed)
        }
        PlaceTowerAtMouse(mouseX, mouseY);
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
    RECT r = { 0, 0, SCREEN_W, SCREEN_H };

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        L"",
		WS_POPUP,   //no border/titlebar
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left,
        r.bottom - r.top,
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

    ParsePathOverlay(g_pathGrid);
	BuildDistanceField();
    BuildPathWidthField();
    CollectSpawnPoints();
    // Spawn an enemy for testing
    for (int lvl = 0; lvl <= 4; ++lvl)
    {
        SpawnEnemy(2, 0); //lvl);
    }

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
    Gdiplus::GdiplusShutdown(g_gdiplusToken);

    return 0;
}

