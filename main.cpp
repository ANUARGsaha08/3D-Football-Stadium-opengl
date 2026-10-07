/* =====================================================================
   3D FOOTBALL STADIUM  -  STEP 20 : FINAL INTEGRATION (this build)
   Environment : Code::Blocks 20.03  (GLUT Shapes Demo template)
   Libraries   : GL / GLU / GLUT   (no extra libraries used)

   This is the final integrated build of STEP 1-14 + STEP 18 + STEP 19,
   plus a small round of POST-STEP-20 fixes/polish requested afterward:

     FIX 1 - GOAL POST BUG : the crossbar's gluCylinder was rotated
             glRotatef(90, 0,1,0), which points a cylinder's default
             +Z axis toward world +X instead of leaving it on +Z. Since
             the crossbar must run from the left post (z=-GOAL_HALF_WIDTH)
             to the right post (z=+GOAL_HALF_WIDTH), it needs to keep
             extending along Z, so no such rotation is needed at all -
             removed. Before this fix the crossbar shot off sideways
             instead of connecting the two posts.
     FIX 2 - SKY : the flat single glClearColor "sky" is replaced with
             drawSkyDome(), a large unlit hemisphere with a real colour
             GRADIENT (bright near the horizon, deeper blue overhead),
             for day and night alike. A handful of simple hierarchical
             CLOUDS (drawCloud() = a cluster of scaled spheres) drift
             slowly across the day sky.
     FIX 3 - NIGHT BALL VISIBILITY : setBallMaterial() now also sets a
             small GL_EMISSION value, so the football keeps a faint
             self-glow and stays clearly visible even where the night
             ambient/floodlight coverage is weak.

   ---------------------------------------------------------------------
   FACULTY REQUIREMENT CHECKLIST  (matches the brief's STEP 25 list)
   ---------------------------------------------------------------------
    1. 3D scene                        -> the whole stadium + field + city
    2. Translation                     -> glTranslatef() everywhere, e.g.
                                           drawPlayer(), drawCar(), ball
    3. Rotation                        -> glRotatef() : players facing,
                                           ball spin (ballRotX/ballRotZ),
                                           roof tilt, moving-car heading
    4. Scaling                         -> glScalef() : player limbs/torso,
                                           banners, buildings, cars, clouds
    5. Keyboard-controlled transform   -> WASD moves the player (updateControlledPlayer),
                                           Arrow keys rotate/snap facing (specialKeys)
    6. Mouse-controlled transform      -> right-drag orbits the camera
                                           (mouseMotion -> camAngle/camPitch)
    7. Complex objects                 -> hierarchical player (head/torso/
                                           arms/legs), car (body/cabin/wheels),
                                           tree (trunk/foliage), cloud (spheres),
                                           stadium roof
    8. Continuously rotating object    -> the football spins in flight
                                           (ballRotX/ballRotZ, every frame)
    9. Window showing external env.    -> NOT built in this run (STEP 16,
                                           VIP room, was skipped on request)
   10. >= 2 light sources              -> GL_LIGHT0..GL_LIGHT3 (4 floodlights)
                                           + GL_LIGHT4 (sun/fill light)
   11. Ambient material                -> setMaterial()'s ambient term,
                                           used by every applyMaterialForObject() call
   12. Diffuse material                -> setMaterial()'s diffuse term
   13. Specular material                -> setMaterial()'s specular term
                                           (metal/glass/gold = high, grass/brick = low)
   14. Textures                        -> procedural grass/seat/concrete/
                                           brick/metal/wood/road/ball textures
                                           (initTextures(), STEP 14)
   15. Object-coordinate transform     -> every draw function builds its
                                           shape in local space, then
                                           glTranslatef/glRotatef places it
                                           in world space (e.g. drawGoal(),
                                           drawCar(), drawFloodlightTower())
   16. Viewing-coordinate transform    -> gluLookAt() in setupCamera(),
                                           driven by camAngle/camPitch/camDist
   17. Animation                       -> running legs/arms (drawPlayer),
                                           ball spin + trajectory, moving
                                           cars, goal celebration, kicking,
                                           drifting clouds
   18. Interactive camera              -> mouse-drag orbit + 4 camera modes
                                           (Free / Follow / Overview / Goal)

   Requirement 9 (window + visible external environment) belongs to the
   VIP room (STEP 16), which was intentionally left out of this build -
   everything else on the list is demonstrated in this file.
   ===================================================================== */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------
   FIELD DIMENSIONS (world units)
   --------------------------------------------------------------------- */
const float FL = 50.0f;
const float FW = 32.0f;

const float GOAL_HALF_WIDTH = 4.0f;
const float GOAL_HEIGHT     = 3.0f;
const float GOAL_DEPTH      = 2.0f;
const float POST_RADIUS     = 0.12f;

const float STAND_GAP    = 12.0f;
const int   NUM_TIERS    = 3;
const float TIER_DEPTH   = 9.0f;
const float TIER_HEIGHT  = 6.0f;
const float OUTER_EXTENT = STAND_GAP + NUM_TIERS * TIER_DEPTH;

/* ---------------------------------------------------------------------
   PLAYER TUNNEL DIMENSIONS (new feature - colourful blue/red entry
   tunnel, styled after a typical club players' tunnel).
   It sits centred on the halfway line (X = 0), running from the pitch
   edge (TUNNEL_Z0 = FW) out to the empty concourse gap in front of the
   first seating tier (TUNNEL_Z1 = FW + STAND_GAP), so it never clips
   into the seating-tier boxes drawn later by drawSeatingTiers().
   --------------------------------------------------------------------- */
const float TUNNEL_HALF_WIDTH = 4.0f;
const float TUNNEL_HEIGHT     = 5.0f;
const float TUNNEL_Z0         = FW;                          // mouth, opens onto the pitch
const float TUNNEL_FLAT_LEN   = 7.0f;                        // flat walkway length
const float TUNNEL_Z_FLAT_END = TUNNEL_Z0 + TUNNEL_FLAT_LEN;
const float TUNNEL_Z1         = FW + STAND_GAP;              // back of the tunnel (= STAND_GAP deep)
const int   TUNNEL_STEPS      = 8;
const float TUNNEL_TOP_Y      = 2.6f;                        // height reached at the top of the stairs

/* ---------------------------------------------------------------------
   VIP ROOM DIMENSIONS (new feature - colourful VIP lounge with an open
   window looking out at the external environment: road, cars, trees,
   buildings, sky and moon).
   It floats above the -Z exterior wall's height (so it clears the wall
   and the roof pillars), centred on the halfway line, and is held up
   by four metal support pillars reaching down to the ground - exactly
   like the existing floodlight towers, just shorter.
   --------------------------------------------------------------------- */
const float VIP_WIDTH  = 16.0f;
const float VIP_HEIGHT = 6.0f;
const float VIP_DEPTH  = 10.0f;
const float VIP_BASE_Y = NUM_TIERS * TIER_HEIGHT + 2.0f;      // clears the exterior wall height
const float VIP_WALL_Z = -(FW + OUTER_EXTENT + 6.0f);         // same as the -Z exterior wall position
const float VIP_NEAR_Z = VIP_WALL_Z - 2.0f;                   // pitch-facing wall of the room
const float VIP_FAR_Z  = VIP_NEAR_Z - VIP_DEPTH;              // outward wall - carries the window

/* ---------------------------------------------------------------------
   CAMERA : STEP 11 mouse-controlled orbit camera with multiple modes.
   Right mouse drag rotates freely, mouse wheel zooms, and keys 1-4 select
   Free / Player Follow / Goal View / Top View.
   --------------------------------------------------------------------- */
float camAngle   = 45.0f;
float camPitch   = 28.0f;
float camDist    = 220.0f;
int   cameraMode = 0;       /* 0=Free, 1=Player Follow, 2=Goal View, 3=Top View */
bool  rightMouseDown = false;
bool  floodlightsOn  = true;
int   lastMouseX = 0;
int   lastMouseY = 0;
const float camTargetY = 10.0f;

/* STEP 19 : DAY / NIGHT MODE
   'N' -> night (dark sky, moon + stars, floodlights + street lights ON)
   'M' -> day   (bright sky, no moon, floodlights reduced/off)
   ('D' is already used by WASD player movement, so Day uses 'M' instead -
   see the "Controls can be adjusted if conflicts occur" note in the brief). */
bool isNightMode = false;

/* STEP 20 : on-screen controls legend, toggled with 'H' - makes the
   project self-explanatory during a viva without needing a cheat sheet. */
bool showHelp = true;

/* STEP 3 : SCOREBOARD STATE */
int  teamAScore = 0;
int  teamBScore = 0;
char matchTimeText[8] = "00:00";

/* ---------------------------------------------------------------------
   FOOTBALL STATE (STEP 4 - 8)
   --------------------------------------------------------------------- */
const float BALL_RADIUS = 0.28f;

float animTime = 0.0f;
float ballX = 0.0f;
float ballY = BALL_RADIUS;
float ballZ = 0.0f;

float ballVX = 0.0f;
float ballVY = 0.0f;
float ballVZ = 0.0f;
float ballRotX = 0.0f;
float ballRotZ = 0.0f;

const float POSSESS_RANGE  = 1.4f;
const float BALL_FRICTION  = 0.90f;
const float GRAVITY        = -0.045f;

int possessingTeam  = -1;
int possessingIndex = -1;

/* STEP 7 : passing.
   passTicksLeft is a safety timer - if a pass somehow never
   reaches the receiver, it is cancelled after PASS_MAX_TICKS. */
bool  passInProgress  = false;
int   passTargetTeam  = -1;
int   passTargetIndex = -1;
int   passTicksLeft   = 0;
const int   PASS_MAX_TICKS = 120;
const float PASS_SPEED     = 0.95f;
const float RECEIVE_RANGE  = 1.5f;

/* STEP 8 : shooting */
const float SHOOT_SPEED = 1.70f;
const float SHOOT_LIFT  = 0.55f;

int kickingTeam     = -1;
int kickingIndex    = -1;
int kickTicksLeft   = 0;
const int KICK_TICKS_TOTAL = 10;   /* ~0.3 s at a 30 ms timer */

/* STEP 9 : goal detection + celebration */
bool celebrating = false;
int  celebrationTeam = -1;
int  celebrationTicksLeft = 0;
const int CELEBRATION_TICKS_TOTAL = 70; /* ~2.1 s at a 30 ms timer */

/* OUT OF PLAY + RESTART STATE */
enum RestartType
{
    RESTART_NONE,
    THROW_IN,
    GOAL_KICK,
    CORNER_KICK
};

RestartType restartType = RESTART_NONE;
bool restartPending = false;
int  restartTeam = -1;
int  lastTouchTeam = -1;
int  restartTicksLeft = 0;
const int RESTART_TICKS = 45;
char restartMessage[40] = "";

/* STEP 10 : simple AI speeds */
const float GK_SPEED         = 0.22f;
const float AI_CHASE_SPEED   = 0.30f;
const float AI_SUPPORT_SPEED = 0.18f;
const float AI_RETURN_SPEED  = 0.15f;
const float GK_DANGER_RANGE  = 15.0f;
const float AI_CHASE_RANGE   = 14.0f;

const float PI_F = 3.1415926f;

/* ---------------------------------------------------------------------
   STEP 18 : OUTSIDE ENVIRONMENT DIMENSIONS
   The stadium exterior wall sits at half-extents (wallHalfX, wallHalfZ).
   Outside it, in order going further away from the pitch, we place:
     plaza/sidewalk (already drawn by drawGround()) -> ROAD LOOP (cars
     drive here) -> a thin ring of trees / street lights just past the
     road -> a few small buildings and a parking area further back.
   --------------------------------------------------------------------- */
const float STADIUM_WALL_HALF_X = FL + (OUTER_EXTENT + 6.0f);   // same as drawStadiumExterior()
const float STADIUM_WALL_HALF_Z = FW + (OUTER_EXTENT + 6.0f);

const float SIDEWALK_WIDTH  = 8.0f;
const float ROAD_WIDTH      = 12.0f;

/* centre-line half extents of the road loop (cars follow this rectangle) */
const float ROAD_HALF_X = STADIUM_WALL_HALF_X + SIDEWALK_WIDTH + ROAD_WIDTH / 2.0f;
const float ROAD_HALF_Z = STADIUM_WALL_HALF_Z + SIDEWALK_WIDTH + ROAD_WIDTH / 2.0f;

const float ROAD_PERIMETER = 4.0f * (ROAD_HALF_X + ROAD_HALF_Z); /* approx, good enough for looping */

/* STEP 18 : ring where trees / street lights stand, just outside the road,
   and the overall size of the flat outside "ground" plane. */
const float TREE_RING_X = ROAD_HALF_X + ROAD_WIDTH / 2.0f + 6.0f;
const float TREE_RING_Z = ROAD_HALF_Z + ROAD_WIDTH / 2.0f + 6.0f;

const float ENV_HALF_X = ROAD_HALF_X + 60.0f;
const float ENV_HALF_Z = ROAD_HALF_Z + 60.0f;

/* STEP 18 : simple moving cars - each car just travels distance
   carDist[i] along the rectangular road loop every tick. */
const int NUM_CARS = 6;
float carDist[NUM_CARS];
float carSpeed[NUM_CARS];
float carColorR[NUM_CARS], carColorG[NUM_CARS], carColorB[NUM_CARS];

/* STEP 19 : a fixed field of stars, placed once in initStars() and only
   drawn while isNightMode is true. */
const int NUM_STARS = 70;
float starX[NUM_STARS];
float starY[NUM_STARS];
float starZ[NUM_STARS];

/* FIX 2 : a handful of simple hierarchical clouds that drift slowly
   across the day sky. Positions/sizes are randomised once in
   initClouds(); cloudDriftX slides them all sideways over time. */
const int NUM_CLOUDS = 10;
float cloudX[NUM_CLOUDS];
float cloudY[NUM_CLOUDS];
float cloudZ[NUM_CLOUDS];
float cloudSize[NUM_CLOUDS];
float cloudDriftX = 0.0f;

/* ---------------------------------------------------------------------
   STEP 14 : TEXTURE IDs
   Each texture is created once in initTextures() and gets an integer id
   (a "name") from OpenGL. Later we only say glBindTexture(id).
   --------------------------------------------------------------------- */
GLuint texGrass    = 0;
GLuint texSeat     = 0;
GLuint texConcrete = 0;
GLuint texBrick    = 0;
GLuint texMetal    = 0;
GLuint texWood     = 0;   /* used by STEP 18 tree trunks                 */
GLuint texRoad     = 0;   /* used by STEP 18 road + parking lot          */
GLuint texBall     = 0;

/* ======================================================================
   MATERIALS (STEP 13)  - moved up so every draw function can use them
   ====================================================================== */
/* FIX 3 : setMaterial() now also accepts an optional EMISSION colour
   (defaults to none). Emission makes a surface glow with its own light
   regardless of how bright the scene lighting is - used for the ball so
   it never goes fully dark on a dim/night-lit part of the pitch. */
void setMaterial(float ar, float ag, float ab,
                 float dr, float dg, float db,
                 float sr, float sg, float sb,
                 float shininess,
                 float er = 0.0f, float eg = 0.0f, float eb = 0.0f)
{
    GLfloat ambient[]  = {ar, ag, ab, 1.0f};
    GLfloat diffuse[]  = {dr, dg, db, 1.0f};
    GLfloat specular[] = {sr, sg, sb, 1.0f};
    GLfloat emission[] = {er, eg, eb, 1.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);   /* reset every time, so no material "leaks" a glow into the next one */
}

void setFieldMaterial()
{
    setMaterial(0.16f, 0.28f, 0.16f,
                0.08f, 0.38f, 0.08f,
                0.15f, 0.25f, 0.15f, 12.0f);          /* grass : low specular */
}

void setWhiteMaterial()
{
    setMaterial(0.42f, 0.42f, 0.42f,
                0.85f, 0.85f, 0.85f,
                0.95f, 0.95f, 0.95f, 80.0f);
}

void setMetalMaterial()
{
    setMaterial(0.30f, 0.30f, 0.32f,
                0.55f, 0.58f, 0.62f,
                0.95f, 0.95f, 0.98f, 90.0f);          /* metal : high specular */
}

void setWoodMaterial()
{
    setMaterial(0.25f, 0.10f, 0.04f,
                0.48f, 0.20f, 0.06f,
                0.30f, 0.16f, 0.06f, 22.0f);
}

void setPlasticMaterial()
{
    setMaterial(0.22f, 0.22f, 0.24f,
                0.30f, 0.30f, 0.32f,
                0.75f, 0.75f, 0.78f, 55.0f);
}

void setBallMaterial()
{
    /* FIX 3 : small emissive glow (0.18,0.18,0.18) keeps the ball
       clearly visible at night, when ambient light is very low -
       without this, a dim corner of the pitch could make the ball
       almost disappear against the dark grass. */
    setMaterial(0.40f, 0.40f, 0.40f,
                0.80f, 0.80f, 0.80f,
                1.00f, 1.00f, 1.00f, 100.0f,
                0.18f, 0.18f, 0.18f);
}

void setGlassMaterial()
{
    setMaterial(0.12f, 0.25f, 0.30f,
                0.15f, 0.45f, 0.55f,
                0.90f, 0.95f, 1.00f, 110.0f);
}

void setGoldMaterial()
{
    setMaterial(0.45f, 0.28f, 0.05f,
                0.85f, 0.55f, 0.05f,
                1.00f, 0.90f, 0.55f, 100.0f);
}

void setConcreteMaterial()
{
    setMaterial(0.30f, 0.30f, 0.30f,
                0.60f, 0.60f, 0.60f,
                0.08f, 0.08f, 0.08f, 6.0f);
}

void setBrickMaterial()
{
    setMaterial(0.30f, 0.22f, 0.20f,
                0.62f, 0.45f, 0.40f,
                0.06f, 0.06f, 0.06f, 5.0f);
}

void applyMaterialForObject(const char* name)
{
    if (name == nullptr) return;

    if (strcmp(name, "field") == 0)         setFieldMaterial();
    else if (strcmp(name, "white") == 0)    setWhiteMaterial();
    else if (strcmp(name, "metal") == 0)    setMetalMaterial();
    else if (strcmp(name, "wood") == 0)     setWoodMaterial();
    else if (strcmp(name, "plastic") == 0)  setPlasticMaterial();
    else if (strcmp(name, "ball") == 0)     setBallMaterial();
    else if (strcmp(name, "glass") == 0)    setGlassMaterial();
    else if (strcmp(name, "gold") == 0)     setGoldMaterial();
    else if (strcmp(name, "concrete") == 0) setConcreteMaterial();
    else if (strcmp(name, "brick") == 0)    setBrickMaterial();
}

/* ======================================================================
   STEP 14 : PROCEDURAL TEXTURE GENERATION
   Every texture is a small RGB picture built pixel by pixel in code.
   ====================================================================== */
static unsigned int texSeed = 20240607u;
static unsigned char texBuf[128 * 128 * 3];

/* tiny repeatable random number generator : returns 0.0 .. 1.0 */
float texRand()
{
    texSeed = texSeed * 1664525u + 1013904223u;
    return (float)((texSeed >> 8) & 0xFFFF) / 65535.0f;
}

unsigned char toByte(float v)
{
    if (v < 0.0f)   v = 0.0f;
    if (v > 255.0f) v = 255.0f;
    return (unsigned char)v;
}

void putPixel(unsigned char* img, int w, int x, int y, float r, float g, float b)
{
    int i = (y * w + x) * 3;
    img[i]     = toByte(r);
    img[i + 1] = toByte(g);
    img[i + 2] = toByte(b);
}

/* Sends the pixel array to OpenGL and returns the texture id.
   gluBuild2DMipmaps also builds smaller copies (mipmaps) so distant
   surfaces do not sparkle. */
GLuint uploadTexture(unsigned char* img, int w, int h)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
    return id;
}

GLuint makeGrassTexture()
{
    const int N = 128;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            float n = (texRand() - 0.5f) * 36.0f;
            float blade = sinf(2.0f * PI_F * (x * 9) / N) * 5.0f
                        + sinf(2.0f * PI_F * (y * 5 + x * 3) / N) * 6.0f;
            putPixel(texBuf, N, x, y,
                     40.0f + n * 0.4f + blade * 0.3f,
                     158.0f + n + blade,
                     38.0f + n * 0.4f);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeSeatTexture()
{
    const int N = 64;                 /* 2 x 2 seats per texture */
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            int cx = x % 32, cy = y % 32;
            float v;
            if (cx < 3 || cx > 28 || cy < 3 || cy > 28)
                v = 40.0f;                                   /* gap between seats */
            else
            {
                v = (cy < 15) ? 238.0f : 205.0f;             /* back-rest / cushion */
                if (cx < 6 || cx > 25 || cy < 6) v -= 28.0f; /* darker rim */
                v += (texRand() - 0.5f) * 8.0f;
            }
            putPixel(texBuf, N, x, y, v, v, v);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeConcreteTexture()
{
    const int N = 128;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            float v = 158.0f + (texRand() - 0.5f) * 30.0f;
            if (texRand() < 0.02f) v -= 45.0f;               /* dark specks */
            if (x % 64 == 0 || y % 64 == 0) v -= 40.0f;      /* panel joints */
            putPixel(texBuf, N, x, y, v, v, v * 0.97f);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeBrickTexture()
{
    const int N = 128;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            int row = y / 16;
            int by  = y % 16;
            int xs  = (x + ((row % 2) ? 16 : 0)) % N;        /* every 2nd row shifted */
            int bx  = xs % 32;
            int id  = (xs / 32) + row * 4;
            float n = (texRand() - 0.5f) * 14.0f;

            if (by < 2 || bx < 2)
                putPixel(texBuf, N, x, y, 196.0f + n, 190.0f + n, 178.0f + n);   /* mortar */
            else
            {
                float tone = (float)((id * 37) % 23) / 23.0f * 34.0f;            /* each brick differs */
                putPixel(texBuf, N, x, y, 158.0f + tone + n, 74.0f + tone * 0.4f + n, 54.0f + n);
            }
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeMetalTexture()
{
    const int N = 64;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            float v = 172.0f + (float)(((y * 53) % 17) - 8) * 1.5f + (texRand() - 0.5f) * 10.0f;
            if (x % 32 == 0 || y % 32 == 0) v -= 55.0f;      /* panel seam */
            int rx = x % 32 - 4;
            int ry = y % 32 - 4;
            if (rx * rx + ry * ry <= 3) v -= 70.0f;          /* rivet */
            putPixel(texBuf, N, x, y, v * 0.96f, v * 0.98f, v);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeWoodTexture()
{
    const int N = 128;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            float t    = y + 6.0f * sinf(2.0f * PI_F * x * 2 / N);
            float ring = 0.5f + 0.5f * sinf(2.0f * PI_F * t * 6 / N);
            float n    = (texRand() - 0.5f) * 10.0f;
            putPixel(texBuf, N, x, y,
                     150.0f - 45.0f * ring + n,
                     96.0f  - 36.0f * ring + n * 0.7f,
                     52.0f  - 22.0f * ring + n * 0.5f);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeRoadTexture()
{
    const int N = 128;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
        {
            float v = 62.0f + (texRand() - 0.5f) * 22.0f;
            if (y >= 62 && y <= 65 && x >= 24 && x <= 104)
                putPixel(texBuf, N, x, y, 235.0f, 235.0f, 225.0f);   /* white centre dash */
            else
                putPixel(texBuf, N, x, y, v, v, v * 1.05f);
        }
    return uploadTexture(texBuf, N, N);
}

GLuint makeBallTexture()
{
    const int W = 128, H = 64;
    /* {centreX, centreY, radius} of the black patches */
    static const float spots[12][3] = {
        { 16, 32, 10 }, { 48, 32, 10 }, { 80, 32, 10 }, { 112, 32, 10 },
        {  0, 12,  8 }, { 32, 12,  8 }, { 64, 12,  8 }, {  96, 12,  8 },
        {  0, 52,  8 }, { 32, 52,  8 }, { 64, 52,  8 }, {  96, 52,  8 }
    };

    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            bool dark = (y < 3 || y > H - 4);
            for (int k = 0; k < 12; k++)
            {
                float dx = fabsf(x - spots[k][0]);
                if (dx > W / 2) dx = W - dx;                 /* wrap around the sphere */
                float dy = y - spots[k][1];
                if (dx * dx + dy * dy <= spots[k][2] * spots[k][2]) dark = true;
            }
            float v = dark ? 22.0f : 242.0f;
            putPixel(texBuf, W, x, y, v, v, v);
        }
    return uploadTexture(texBuf, W, H);
}

void initTextures()
{
    texGrass    = makeGrassTexture();
    texSeat     = makeSeatTexture();
    texConcrete = makeConcreteTexture();
    texBrick    = makeBrickTexture();
    texMetal    = makeMetalTexture();
    texWood     = makeWoodTexture();
    texRoad     = makeRoadTexture();
    texBall     = makeBallTexture();
}

/* Turn texturing ON for the next surfaces / OFF again.
   GL_MODULATE : final colour = texture colour x lit glColor, so we can tint
   one texture into many looks (red seats, blue seats ...). */
void useTexture(GLuint id)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void noTexture()
{
    glDisable(GL_TEXTURE_2D);
}

/* Textured horizontal floor made of nx*nz small quads.
   Many small quads (not one big quad) = better per-vertex lighting from
   the spotlights. Texture coordinate = world position / tile, so the
   picture repeats every 'tile' world units. */
void drawTexturedGrid(float x1, float z1, float x2, float z2, float y,
                      int nx, int nz, float tile)
{
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < nz; j++)
        {
            float xa = x1 + (x2 - x1) * i / nx;
            float xb = x1 + (x2 - x1) * (i + 1) / nx;
            float za = z1 + (z2 - z1) * j / nz;
            float zb = z1 + (z2 - z1) * (j + 1) / nz;

            glTexCoord2f(xa / tile, za / tile); glVertex3f(xa, y, za);
            glTexCoord2f(xb / tile, za / tile); glVertex3f(xb, y, za);
            glTexCoord2f(xb / tile, zb / tile); glVertex3f(xb, y, zb);
            glTexCoord2f(xa / tile, zb / tile); glVertex3f(xa, y, zb);
        }
    glEnd();
}

/* Textured box centred on the origin (glutSolidCube has NO texture
   coordinates, so we build the cube ourselves).
   Top face uses topTex/topCol, the other faces use sideTex/sideCol.
   Texture repeat = face size / tile, so the picture never looks stretched. */
void drawTexturedCube(float sx, float sy, float sz,
                      GLuint topTex,  const float* topCol,
                      GLuint sideTex, const float* sideCol, float tile)
{
    float x = sx / 2.0f, y = sy / 2.0f, z = sz / 2.0f;
    float ux = sx / tile, uy = sy / tile, uz = sz / tile;

    /* ---- top ---- */
    useTexture(topTex);
    glColor3fv(topCol);
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-x, y,  z);
        glTexCoord2f(ux,   0.0f); glVertex3f( x, y,  z);
        glTexCoord2f(ux,   uz  ); glVertex3f( x, y, -z);
        glTexCoord2f(0.0f, uz  ); glVertex3f(-x, y, -z);
    glEnd();

    /* ---- sides + bottom ---- */
    useTexture(sideTex);
    glColor3fv(sideCol);
    glBegin(GL_QUADS);
        /* +Z */
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-x, -y, z);
        glTexCoord2f(ux,   0.0f); glVertex3f( x, -y, z);
        glTexCoord2f(ux,   uy  ); glVertex3f( x,  y, z);
        glTexCoord2f(0.0f, uy  ); glVertex3f(-x,  y, z);
        /* -Z */
        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( x, -y, -z);
        glTexCoord2f(ux,   0.0f); glVertex3f(-x, -y, -z);
        glTexCoord2f(ux,   uy  ); glVertex3f(-x,  y, -z);
        glTexCoord2f(0.0f, uy  ); glVertex3f( x,  y, -z);
        /* +X */
        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(x, -y,  z);
        glTexCoord2f(uz,   0.0f); glVertex3f(x, -y, -z);
        glTexCoord2f(uz,   uy  ); glVertex3f(x,  y, -z);
        glTexCoord2f(0.0f, uy  ); glVertex3f(x,  y,  z);
        /* -X */
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-x, -y, -z);
        glTexCoord2f(uz,   0.0f); glVertex3f(-x, -y,  z);
        glTexCoord2f(uz,   uy  ); glVertex3f(-x,  y,  z);
        glTexCoord2f(0.0f, uy  ); glVertex3f(-x,  y, -z);
        /* bottom */
        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-x, -y, -z);
        glTexCoord2f(ux,   0.0f); glVertex3f( x, -y, -z);
        glTexCoord2f(ux,   uz  ); glVertex3f( x, -y,  z);
        glTexCoord2f(0.0f, uz  ); glVertex3f(-x, -y,  z);
    glEnd();
    noTexture();
}

/* Same idea as the old drawBox(): (cx,cz) = centre on the ground plane,
   baseY = bottom height. */
void drawBoxTex(float cx, float cz, float sx, float sz, float baseY, float height,
                GLuint topTex,  const float* topCol,
                GLuint sideTex, const float* sideCol, float tile)
{
    glPushMatrix();
        glTranslatef(cx, baseY + height / 2.0f, cz);
        drawTexturedCube(sx, height, sz, topTex, topCol, sideTex, sideCol, tile);
    glPopMatrix();
}

/* ======================================================================
   UTILITY : draw a horizontal ring/rectangle outline using GL_LINE_LOOP
   ====================================================================== */
void drawRectOutline(float x1, float z1, float x2, float z2, float y)
{
    glBegin(GL_LINE_LOOP);
        glVertex3f(x1, y, z1);
        glVertex3f(x2, y, z1);
        glVertex3f(x2, y, z2);
        glVertex3f(x1, y, z2);
    glEnd();
}

void drawArc(float cx, float cz, float radius, float startDeg, float endDeg, float y)
{
    glBegin(GL_LINE_STRIP);
    for (float a = startDeg; a <= endDeg; a += 2.0f)
    {
        float rad = a * PI_F / 180.0f;
        glVertex3f(cx + radius * cosf(rad), y, cz + radius * sinf(rad));
    }
    glEnd();
}

/* ======================================================================
   STEP 14 : GROUND (concrete plaza under the whole stadium + pitch-side area)
   ====================================================================== */
void drawGround()
{
    applyMaterialForObject("concrete");
    useTexture(texConcrete);
    glColor3f(0.78f, 0.78f, 0.78f);

    float gx = FL + OUTER_EXTENT + 26.0f;
    float gz = FW + OUTER_EXTENT + 26.0f;
    drawTexturedGrid(-gx, -gz, gx, gz, -0.05f, 24, 20, 10.0f);

    noTexture();
}

/* ======================================================================
   FOOTBALL FIELD  (STEP 14 : grass texture, stripes kept as colour tint)
   ====================================================================== */
void drawField()
{
    applyMaterialForObject("field");
    useTexture(texGrass);

    int   stripes = 20;
    float stripeWidth = (2 * FL) / stripes;

    for (int i = 0; i < stripes; i++)
    {
        float x1 = -FL + i * stripeWidth;
        float x2 = x1 + stripeWidth;

        if (i % 2 == 0) glColor3f(1.00f, 1.00f, 1.00f);   /* light stripe */
        else            glColor3f(0.82f, 0.88f, 0.82f);   /* dark stripe  */

        drawTexturedGrid(x1, -FW, x2, FW, 0.0f, 1, 8, 6.0f);
    }
    noTexture();
}

/* ======================================================================
   FIELD MARKINGS
   ====================================================================== */
void drawFieldMarkings()
{
    applyMaterialForObject("field");
    glNormal3f(0.0f, 1.0f, 0.0f);          /* lines face upward, so they are lit properly */
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    float y = 0.02f;

    drawRectOutline(-FL, -FW, FL, FW, y);

    glBegin(GL_LINES);
        glVertex3f(0.0f, y, -FW);
        glVertex3f(0.0f, y,  FW);
    glEnd();

    drawArc(0.0f, 0.0f, 8.0f, 0.0f, 360.0f, y);
    glBegin(GL_POINTS);
        glVertex3f(0.0f, y, 0.0f);
    glEnd();

    drawRectOutline(-FL, -22.0f, -FL + 18.0f, 22.0f, y);
    drawRectOutline(-FL, -10.0f, -FL + 6.0f,  10.0f, y);
    glBegin(GL_POINTS);
        glVertex3f(-FL + 12.0f, y, 0.0f);
    glEnd();
    drawArc(-FL, -FW, 2.0f, 0.0f,  90.0f, y);
    drawArc(-FL,  FW, 2.0f, -90.0f, 0.0f, y);

    drawRectOutline(FL, -22.0f, FL - 18.0f, 22.0f, y);
    drawRectOutline(FL, -10.0f, FL - 6.0f,  10.0f, y);
    glBegin(GL_POINTS);
        glVertex3f(FL - 12.0f, y, 0.0f);
    glEnd();
    drawArc(FL, -FW, 2.0f, 90.0f,  180.0f, y);
    drawArc(FL,  FW, 2.0f, 180.0f, 270.0f, y);
}

void drawCornerFlags()
{
    float corners[4][2] = {
        { -FL, -FW }, { -FL, FW }, { FL, -FW }, { FL, FW }
    };

    for (int i = 0; i < 4; i++)
    {
        glPushMatrix();
            glTranslatef(corners[i][0], 0.0f, corners[i][1]);

            glColor3f(1.0f, 1.0f, 1.0f);
            GLUquadric* q = gluNewQuadric();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, 0.05f, 0.05f, 1.5f, 8, 1);
            gluDeleteQuadric(q);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

            glColor3f(1.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, 1.4f, 0.0f);
            glBegin(GL_TRIANGLES);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.4f, 0.15f, 0.0f);
                glVertex3f(0.0f, 0.3f, 0.0f);
            glEnd();
        glPopMatrix();
    }
}

/* ======================================================================
   GOAL POST + NET
   ====================================================================== */
void drawGoalNet(float depth)
{
    /* net lines are drawn without lighting so they always stay
       clearly visible (white mesh) from every camera angle. */
    glDisable(GL_LIGHTING);
    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(1.0f);

    int meshH = 8;
    int meshW = 8;

    glBegin(GL_LINES);
    for (int i = 0; i <= meshW; i++)
    {
        float z = -GOAL_HALF_WIDTH + i * (2 * GOAL_HALF_WIDTH / meshW);
        glVertex3f(depth, 0.0f, z);
        glVertex3f(depth, GOAL_HEIGHT, z);
    }
    for (int j = 0; j <= meshH; j++)
    {
        float y = j * (GOAL_HEIGHT / meshH);
        glVertex3f(depth, y, -GOAL_HALF_WIDTH);
        glVertex3f(depth, y,  GOAL_HALF_WIDTH);
    }
    glEnd();

    glBegin(GL_LINES);
    for (int i = 0; i <= meshW; i++)
    {
        float z = -GOAL_HALF_WIDTH + i * (2 * GOAL_HALF_WIDTH / meshW);
        glVertex3f(0.0f,  GOAL_HEIGHT, z);
        glVertex3f(depth, GOAL_HEIGHT, z);
    }
    glEnd();

    glBegin(GL_LINES);
    for (int j = 0; j <= meshH; j++)
    {
        float y = j * (GOAL_HEIGHT / meshH);
        glVertex3f(0.0f,  y, -GOAL_HALF_WIDTH);
        glVertex3f(depth, y, -GOAL_HALF_WIDTH);

        glVertex3f(0.0f,  y,  GOAL_HALF_WIDTH);
        glVertex3f(depth, y,  GOAL_HALF_WIDTH);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}

void drawGoal(int direction)
{
    glPushMatrix();
        glTranslatef(direction * FL, 0.0f, 0.0f);
        float depth = direction * GOAL_DEPTH;

        glColor3f(1.0f, 1.0f, 1.0f);
        GLUquadric* q = gluNewQuadric();

        glPushMatrix();
            glTranslatef(0.0f, 0.0f, -GOAL_HALF_WIDTH);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, POST_RADIUS, POST_RADIUS, GOAL_HEIGHT, 10, 1);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(0.0f, 0.0f, GOAL_HALF_WIDTH);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(q, POST_RADIUS, POST_RADIUS, GOAL_HEIGHT, 10, 1);
        glPopMatrix();

        /* FIX 1 - GOAL POST BUG : the crossbar must run from the left
           post (z=-GOAL_HALF_WIDTH) to the right post (z=+GOAL_HALF_WIDTH),
           which is exactly the direction a GLU cylinder already extends
           in by default (its local +Z axis) once we translate to the
           left post's top. The old code added an extra
           glRotatef(90, 0,1,0) here, which swings that +Z axis around to
           point along world +X instead - so the crossbar shot off
           sideways (into/out of the pitch) rather than connecting the
           two posts. Removing that rotation fixes it. */
        glPushMatrix();
            glTranslatef(0.0f, GOAL_HEIGHT, -GOAL_HALF_WIDTH);
            gluCylinder(q, POST_RADIUS, POST_RADIUS, 2 * GOAL_HALF_WIDTH, 10, 1);
        glPopMatrix();

        gluDeleteQuadric(q);

        drawGoalNet(depth);
    glPopMatrix();
}

/* ======================================================================
   STADIUM STANDS  (STEP 2, textured in STEP 14)
   ====================================================================== */
/* untextured box - still used by the scoreboard */
void drawBox(float cx, float cz, float sx, float sz, float baseY, float height,
             float r, float g, float b)
{
    glPushMatrix();
        glTranslatef(cx, baseY + height / 2.0f, cz);
        glScalef(sx, height, sz);
        glColor3f(r, g, b);
        glutSolidCube(1.0f);
    glPopMatrix();
}

float touchStandLength() { return 2 * FL + 2 * OUTER_EXTENT; }
float endStandLength()   { return 2 * FW; }

void drawSeatingTiers()
{
    applyMaterialForObject("concrete");

    for (int i = 0; i < NUM_TIERS; i++)
    {
        float baseY   = i * TIER_HEIGHT;
        float tierPos = STAND_GAP + i * TIER_DEPTH + TIER_DEPTH / 2.0f;

        /* seat colour (tint) of this tier - the seat texture is light gray
           and gets multiplied by this colour */
        float seatCol[3];
        if (i == 0)      { seatCol[0] = 0.75f; seatCol[1] = 0.75f; seatCol[2] = 0.78f; }
        else if (i == 1) { seatCol[0] = 0.55f; seatCol[1] = 0.62f; seatCol[2] = 0.85f; }
        else             { seatCol[0] = 0.25f; seatCol[1] = 0.42f; seatCol[2] = 0.85f; }

        float wallCol[3] = { 0.85f, 0.85f, 0.85f };   /* concrete risers */

        drawBoxTex(0.0f,  FW + tierPos, touchStandLength(), TIER_DEPTH, baseY, TIER_HEIGHT,
                   texSeat, seatCol, texConcrete, wallCol, 4.0f);
        drawBoxTex(0.0f, -(FW + tierPos), touchStandLength(), TIER_DEPTH, baseY, TIER_HEIGHT,
                   texSeat, seatCol, texConcrete, wallCol, 4.0f);

        drawBoxTex( (FL + tierPos), 0.0f, TIER_DEPTH, endStandLength(), baseY, TIER_HEIGHT,
                   texSeat, seatCol, texConcrete, wallCol, 4.0f);
        drawBoxTex(-(FL + tierPos), 0.0f, TIER_DEPTH, endStandLength(), baseY, TIER_HEIGHT,
                   texSeat, seatCol, texConcrete, wallCol, 4.0f);
    }
}

void drawPillar(float x, float z, float topHeight)
{
    glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.75f, 0.75f, 0.80f);
        useTexture(texMetal);
        GLUquadric* q = gluNewQuadric();
        gluQuadricTexture(q, GL_TRUE);        /* ask GLU to generate texture coordinates */
        gluCylinder(q, 0.4f, 0.4f, topHeight, 10, 1);
        gluDeleteQuadric(q);
        noTexture();
    glPopMatrix();
}

void drawRoofPanel(float cx, float cy, float cz, float spanLen, float depth,
                   float thickness, float tiltDeg, bool alongX)
{
    float tint[3] = { 0.55f, 0.58f, 0.66f };   /* dark blue-gray metal */

    glPushMatrix();
        glTranslatef(cx, cy, cz);
        if (alongX)
            glRotatef(tiltDeg, 1.0f, 0.0f, 0.0f);
        else
            glRotatef(tiltDeg, 0.0f, 0.0f, 1.0f);

        if (alongX)
            drawTexturedCube(spanLen, thickness, depth, texMetal, tint, texMetal, tint, 8.0f);
        else
            drawTexturedCube(depth, thickness, spanLen, texMetal, tint, texMetal, tint, 8.0f);
    glPopMatrix();
}

void drawStadiumRoof()
{
    float topY      = NUM_TIERS * TIER_HEIGHT;
    float roofY     = topY + 2.0f;
    float roofDepth = TIER_DEPTH * 1.4f;
    float backEdge  = STAND_GAP + NUM_TIERS * TIER_DEPTH;

    drawRoofPanel(0.0f, roofY,  (FW + backEdge - roofDepth / 2.0f), touchStandLength() * 0.97f, roofDepth, 0.8f, -8.0f, true);
    drawRoofPanel(0.0f, roofY, -(FW + backEdge - roofDepth / 2.0f), touchStandLength() * 0.97f, roofDepth, 0.8f,  8.0f, true);
    drawRoofPanel( (FL + backEdge - roofDepth / 2.0f), roofY, 0.0f, endStandLength() * 0.97f, roofDepth, 0.8f, -8.0f, false);
    drawRoofPanel(-(FL + backEdge - roofDepth / 2.0f), roofY, 0.0f, endStandLength() * 0.97f, roofDepth, 0.8f,  8.0f, false);

    for (float x = -FL - OUTER_EXTENT + 10.0f; x <= FL + OUTER_EXTENT - 10.0f; x += 20.0f)
    {
        drawPillar(x,  (FW + backEdge), topY);
        drawPillar(x, -(FW + backEdge), topY);
    }
    for (float z = -FW - 8.0f; z <= FW + 8.0f; z += 16.0f)
    {
        drawPillar( (FL + backEdge), z, topY);
        drawPillar(-(FL + backEdge), z, topY);
    }
}

/* brick box : same tint on top and sides */
void drawBrickBox(float cx, float cz, float sx, float sz, float baseY, float height, float tint)
{
    float c[3] = { tint, tint, tint };
    drawBoxTex(cx, cz, sx, sz, baseY, height, texBrick, c, texBrick, c, 6.0f);
}

void drawExteriorWallSide(float centerX, float centerZ, float totalLen,
                          float thickness, float height, bool alongX)
{
    float gapWidth = 10.0f;
    float segLen   = (totalLen - gapWidth) / 2.0f;

    if (alongX)
    {
        float leftC  = centerX - gapWidth / 2.0f - segLen / 2.0f;
        float rightC = centerX + gapWidth / 2.0f + segLen / 2.0f;
        drawBrickBox(leftC,  centerZ, segLen, thickness, 0.0f, height, 1.0f);
        drawBrickBox(rightC, centerZ, segLen, thickness, 0.0f, height, 1.0f);
        drawBrickBox(centerX, centerZ, gapWidth, thickness, height * 0.85f, height * 0.15f, 0.85f);
    }
    else
    {
        float leftC  = centerZ - gapWidth / 2.0f - segLen / 2.0f;
        float rightC = centerZ + gapWidth / 2.0f + segLen / 2.0f;
        drawBrickBox(centerX, leftC,  thickness, segLen, 0.0f, height, 1.0f);
        drawBrickBox(centerX, rightC, thickness, segLen, 0.0f, height, 1.0f);
        drawBrickBox(centerX, centerZ, thickness, gapWidth, height * 0.85f, height * 0.15f, 0.85f);
    }
}

void drawStadiumExterior()
{
    float wallOffset = OUTER_EXTENT + 6.0f;
    float wallHeight = NUM_TIERS * TIER_HEIGHT * 1.1f;
    float wallThick  = 2.0f;

    float halfX = FL + wallOffset;
    float halfZ = FW + wallOffset;

    drawExteriorWallSide(0.0f,  halfZ, 2 * halfX, wallThick, wallHeight, true);
    drawExteriorWallSide(0.0f, -halfZ, 2 * halfX, wallThick, wallHeight, true);
    drawExteriorWallSide( halfX, 0.0f, 2 * halfZ, wallThick, wallHeight, false);
    drawExteriorWallSide(-halfX, 0.0f, 2 * halfZ, wallThick, wallHeight, false);
}

/* ======================================================================
   PLAYER TUNNEL  (new feature)
   ------------------------------------------------------------------
   A colourful blue/red players' entry tunnel, built to resemble a
   typical club tunnel photo: dark-blue skirting + top trim, a wide
   red panel in between carrying a round crest on each wall, a beige
   walk-mat with a gold centre line on the floor, brass handrails,
   bright ceiling lights, and a short staircase at the far end.
   It sits centred on the halfway line (X = 0) and runs from the pitch
   edge out to the open concourse gap in front of the first stand tier,
   lining up with the existing gap left in drawExteriorWallSide().
   Every helper below is NEW code; no existing function is changed.
   ====================================================================== */

/* One wall panel: blue skirt (bottom) + red main panel (middle) +
   blue trim (top), all facing inward toward the tunnel centre line. */
void drawTunnelWallSegment(float wallX, float z0, float z1, float floorY)
{
    float skirt = 0.4f;
    float trim  = 0.4f;
    float nrm = (wallX > 0.0f) ? -1.0f : 1.0f;
    glNormal3f(nrm, 0.0f, 0.0f);

    glColor3f(0.03f, 0.05f, 0.28f);                 /* skirt - dark blue */
    glBegin(GL_QUADS);
        glVertex3f(wallX, floorY, z0);
        glVertex3f(wallX, floorY, z1);
        glVertex3f(wallX, floorY + skirt, z1);
        glVertex3f(wallX, floorY + skirt, z0);
    glEnd();

    glColor3f(0.72f, 0.06f, 0.08f);                 /* main panel - red */
    glBegin(GL_QUADS);
        glVertex3f(wallX, floorY + skirt, z0);
        glVertex3f(wallX, floorY + skirt, z1);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT - trim, z1);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT - trim, z0);
    glEnd();

    glColor3f(0.03f, 0.05f, 0.28f);                 /* top trim - dark blue */
    glBegin(GL_QUADS);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT - trim, z0);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT - trim, z1);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT, z1);
        glVertex3f(wallX, floorY + TUNNEL_HEIGHT, z0);
    glEnd();
}

/* A simple round emblem (gold ring + red centre + a small white star),
   mounted flat on a side wall, generic/unbranded - just decoration. */
void drawTunnelCrest(float wallX, float y, float z)
{
    float signX = (wallX > 0.0f) ? 1.0f : -1.0f;

    glDisable(GL_LIGHTING);
    glPushMatrix();
        glTranslatef(wallX - signX * 0.02f, y, z);
        glRotatef(90.0f * signX, 0.0f, 1.0f, 0.0f);

        GLUquadric* q = gluNewQuadric();
        glColor3f(0.90f, 0.75f, 0.20f);
        gluDisk(q, 0.0f, 0.55f, 20, 1);
        glColor3f(0.80f, 0.08f, 0.08f);
        gluDisk(q, 0.0f, 0.40f, 20, 1);
        gluDeleteQuadric(q);

        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex3f(0.0f, 0.0f, 0.03f);
            for (int i = 0; i <= 10; i++)
            {
                float a = i * 36.0f * PI_F / 180.0f;
                float r = (i % 2 == 0) ? 0.22f : 0.09f;
                glVertex3f(cosf(a) * r, sinf(a) * r, 0.03f);
            }
        glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* Floor of the flat section: dark-blue outer floor, a beige centre
   walk-mat, and a thin gold centre line - matching the reference photo. */
void drawTunnelFloorFlat(float z0, float z1, float y)
{
    float halfW = TUNNEL_HALF_WIDTH;
    glNormal3f(0.0f, 1.0f, 0.0f);

    glColor3f(0.05f, 0.08f, 0.35f);
    glBegin(GL_QUADS);
        glVertex3f(-halfW, y, z0);
        glVertex3f( halfW, y, z0);
        glVertex3f( halfW, y, z1);
        glVertex3f(-halfW, y, z1);
    glEnd();

    float matHalf = halfW * 0.42f;
    glColor3f(0.72f, 0.65f, 0.50f);
    glBegin(GL_QUADS);
        glVertex3f(-matHalf, y + 0.01f, z0);
        glVertex3f( matHalf, y + 0.01f, z0);
        glVertex3f( matHalf, y + 0.01f, z1);
        glVertex3f(-matHalf, y + 0.01f, z1);
    glEnd();

    glColor3f(0.85f, 0.68f, 0.15f);
    glBegin(GL_QUADS);
        glVertex3f(-0.05f, y + 0.02f, z0);
        glVertex3f( 0.05f, y + 0.02f, z0);
        glVertex3f( 0.05f, y + 0.02f, z1);
        glVertex3f(-0.05f, y + 0.02f, z1);
    glEnd();
}

/* Ceiling of the flat section, plus a row of bright unlit rectangles
   along the centre line to read as fluorescent ceiling lights. */
void drawTunnelCeiling(float z0, float z1, float y)
{
    float halfW = TUNNEL_HALF_WIDTH;
    glNormal3f(0.0f, -1.0f, 0.0f);
    glColor3f(0.05f, 0.07f, 0.30f);
    glBegin(GL_QUADS);
        glVertex3f(-halfW, y, z0);
        glVertex3f(-halfW, y, z1);
        glVertex3f( halfW, y, z1);
        glVertex3f( halfW, y, z0);
    glEnd();

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.92f);
    for (float z = z0 + 0.6f; z < z1; z += 1.4f)
    {
        glBegin(GL_QUADS);
            glVertex3f(-0.6f, y - 0.03f, z - 0.35f);
            glVertex3f( 0.6f, y - 0.03f, z - 0.35f);
            glVertex3f( 0.6f, y - 0.03f, z + 0.35f);
            glVertex3f(-0.6f, y - 0.03f, z + 0.35f);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

/* Brass/gold handrail running from (x,y0,z0) to (x,y1,z1) - used both
   for the flat section (y0==y1) and along the staircase (y1>y0). */
void drawTunnelRailing(float x, float z0, float z1, float y0, float y1)
{
    glColor3f(0.80f, 0.65f, 0.25f);

    float dz = z1 - z0;
    float dy = y1 - y0;
    float len = sqrtf(dz * dz + dy * dy);
    if (len < 0.001f) return;
    float angleDeg = atan2f(dy, dz) * 180.0f / PI_F;

    glPushMatrix();
        glTranslatef(x, y0, z0);
        glRotatef(-angleDeg, 1.0f, 0.0f, 0.0f);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.05f, 0.05f, len, 8, 1);
        gluDeleteQuadric(q);
    glPopMatrix();

    /* two support posts, one at each end of this rail segment */
    glPushMatrix();
        glTranslatef(x, y0 - 0.45f, z0);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* p1 = gluNewQuadric();
        gluCylinder(p1, 0.04f, 0.04f, 0.45f, 6, 1);
        gluDeleteQuadric(p1);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(x, y1 - 0.45f, z1);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* p2 = gluNewQuadric();
        gluCylinder(p2, 0.04f, 0.04f, 0.45f, 6, 1);
        gluDeleteQuadric(p2);
    glPopMatrix();
}

/* Staircase at the far end of the tunnel: plain concrete-grey steps,
   each built by reusing the existing drawBox() helper (unchanged). */
void drawTunnelStairs(float z0, float z1, float y0, float y1, int steps)
{
    float stepDepth = (z1 - z0) / steps;
    float stepRise  = (y1 - y0) / steps;
    float treadWidth = TUNNEL_HALF_WIDTH * 2.0f;

    for (int i = 0; i < steps; i++)
    {
        float sz = z0 + i * stepDepth;
        float sy = y0 + i * stepRise;
        drawBox(0.0f, sz + stepDepth / 2.0f, treadWidth, stepDepth, sy, stepRise, 0.55f, 0.55f, 0.58f);
    }
}

/* Master function: assembles the flat corridor (floor, ceiling, walls,
   crests, railings) and the staircase into the full players' tunnel. */
void drawPlayerTunnel()
{
    float y0 = 0.0f;
    float y1 = TUNNEL_TOP_Y;

    applyMaterialForObject("plastic");

    /* --- flat walkway section --- */
    drawTunnelFloorFlat(TUNNEL_Z0, TUNNEL_Z_FLAT_END, y0);
    drawTunnelCeiling(TUNNEL_Z0, TUNNEL_Z_FLAT_END, y0 + TUNNEL_HEIGHT);
    drawTunnelWallSegment(-TUNNEL_HALF_WIDTH, TUNNEL_Z0, TUNNEL_Z_FLAT_END, y0);
    drawTunnelWallSegment( TUNNEL_HALF_WIDTH, TUNNEL_Z0, TUNNEL_Z_FLAT_END, y0);

    drawTunnelCrest(-TUNNEL_HALF_WIDTH, y0 + TUNNEL_HEIGHT * 0.55f, TUNNEL_Z0 + TUNNEL_FLAT_LEN * 0.5f);
    drawTunnelCrest( TUNNEL_HALF_WIDTH, y0 + TUNNEL_HEIGHT * 0.55f, TUNNEL_Z0 + TUNNEL_FLAT_LEN * 0.5f);

    drawTunnelRailing(-TUNNEL_HALF_WIDTH * 0.55f, TUNNEL_Z0, TUNNEL_Z_FLAT_END, 0.9f, 0.9f);
    drawTunnelRailing( TUNNEL_HALF_WIDTH * 0.55f, TUNNEL_Z0, TUNNEL_Z_FLAT_END, 0.9f, 0.9f);

    /* --- staircase section, leading up toward the stand concourse --- */
    drawTunnelStairs(TUNNEL_Z_FLAT_END, TUNNEL_Z1, y0, y1, TUNNEL_STEPS);
    drawTunnelWallSegment(-TUNNEL_HALF_WIDTH, TUNNEL_Z_FLAT_END, TUNNEL_Z1, y0);
    drawTunnelWallSegment( TUNNEL_HALF_WIDTH, TUNNEL_Z_FLAT_END, TUNNEL_Z1, y0);

    drawTunnelRailing(-TUNNEL_HALF_WIDTH * 0.55f, TUNNEL_Z_FLAT_END, TUNNEL_Z1, 0.9f, 0.9f + y1);
    drawTunnelRailing( TUNNEL_HALF_WIDTH * 0.55f, TUNNEL_Z_FLAT_END, TUNNEL_Z1, 0.9f, 0.9f + y1);
}

/* ======================================================================
   VIP ROOM  (new feature)
   ------------------------------------------------------------------
   A colourful VIP lounge that floats above the -Z exterior wall,
   held up on four metal pillars (reusing the existing drawPillar()
   helper, unchanged). Four of its five sides are solid coloured walls;
   the outward-facing (far) wall is left OPEN in the middle - framed by
   a dark window frame and a faint tinted glass pane - so the already-
   drawn outside world (road, cars, trees, buildings, sky, moon) is
   genuinely visible through the opening, the same way a real window
   works, rather than a painted picture. Every helper below is NEW
   code; no existing function is changed.
   ====================================================================== */

/* Small generic helpers for flat rectangles lying in each of the three
   coordinate planes - used to build the room's floor/ceiling/walls. */
void drawFlatQuadXZ(float x1, float z1, float x2, float z2, float y, float ny)
{
    glNormal3f(0.0f, ny, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x1, y, z1);
        glVertex3f(x2, y, z1);
        glVertex3f(x2, y, z2);
        glVertex3f(x1, y, z2);
    glEnd();
}

void drawFlatQuadXY(float x1, float y1, float x2, float y2, float z, float nz)
{
    glNormal3f(0.0f, 0.0f, nz);
    glBegin(GL_QUADS);
        glVertex3f(x1, y1, z);
        glVertex3f(x2, y1, z);
        glVertex3f(x2, y2, z);
        glVertex3f(x1, y2, z);
    glEnd();
}

void drawFlatQuadYZ(float y1, float z1, float y2, float z2, float x, float nx)
{
    glNormal3f(nx, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x, y1, z1);
        glVertex3f(x, y2, z1);
        glVertex3f(x, y2, z2);
        glVertex3f(x, y1, z2);
    glEnd();
}

/* Four metal legs holding the room up off the ground, at its corners -
   built with the existing drawPillar() helper (unchanged). */
void drawVIPSupports()
{
    applyMaterialForObject("metal");
    float px = VIP_WIDTH / 2.0f - 1.0f;
    float zNear = VIP_NEAR_Z + 0.5f;
    float zFar  = VIP_FAR_Z  - 0.5f;

    drawPillar(-px, zNear, VIP_BASE_Y);
    drawPillar( px, zNear, VIP_BASE_Y);
    drawPillar(-px, zFar,  VIP_BASE_Y);
    drawPillar( px, zFar,  VIP_BASE_Y);
}

/* A gold "VIP" plaque, mounted just above the window on the outward
   face, using glutStrokeCharacter the same way drawPanelText() does
   for the scoreboard (but this wall already faces along Z, so no
   extra rotation is needed to lay the text flat against it). */
void drawVIPSign()
{
    glDisable(GL_LIGHTING);
    glColor3f(0.90f, 0.75f, 0.20f);
    glPushMatrix();
        glTranslatef(-1.6f, VIP_BASE_Y + VIP_HEIGHT - 0.75f, VIP_FAR_Z + 0.05f);
        glScalef(0.011f, 0.011f, 0.011f);
        const char* text = "VIP";
        for (const char* c = text; *c != '\0'; c++)
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* The five walls + floor + ceiling. The far (outward) wall is built as
   four solid strips AROUND a rectangular gap, so the gap is a real
   opening, not a texture. */
void drawVIPStructure()
{
    float halfW = VIP_WIDTH / 2.0f;
    float topY  = VIP_BASE_Y + VIP_HEIGHT;

    applyMaterialForObject("plastic");

    /* floor - red carpet */
    glColor3f(0.55f, 0.05f, 0.08f);
    drawFlatQuadXZ(-halfW, VIP_FAR_Z, halfW, VIP_NEAR_Z, VIP_BASE_Y, 1.0f);

    /* ceiling - cream/gold tint */
    glColor3f(0.85f, 0.80f, 0.65f);
    drawFlatQuadXZ(-halfW, VIP_FAR_Z, halfW, VIP_NEAR_Z, topY, -1.0f);

    /* left + right walls - deep blue */
    glColor3f(0.08f, 0.12f, 0.42f);
    drawFlatQuadYZ(VIP_BASE_Y, VIP_FAR_Z, topY, VIP_NEAR_Z, -halfW, 1.0f);
    drawFlatQuadYZ(VIP_BASE_Y, VIP_FAR_Z, topY, VIP_NEAR_Z,  halfW, -1.0f);

    /* near wall (facing the pitch) - four gold strips framing a wide,
       clear MATCH-VIEW window so the actual football match is visible
       through a real opening, the same way the outward window works. */
    float nwx1 = -halfW + 2.0f, nwx2 = halfW - 2.0f;
    float nwy1 = VIP_BASE_Y + 1.5f, nwy2 = topY - 0.5f;   /* sill sits above the sofa back */

    glColor3f(0.70f, 0.55f, 0.15f);
    drawFlatQuadXY(-halfW, VIP_BASE_Y, nwx1, topY, VIP_NEAR_Z, 1.0f);   /* left strip   */
    drawFlatQuadXY(nwx2, VIP_BASE_Y, halfW, topY, VIP_NEAR_Z, 1.0f);   /* right strip  */
    drawFlatQuadXY(nwx1, VIP_BASE_Y, nwx2, nwy1, VIP_NEAR_Z, 1.0f);    /* bottom strip */
    drawFlatQuadXY(nwx1, nwy2, nwx2, topY, VIP_NEAR_Z, 1.0f);          /* top strip    */

    /* dark metal frame around the match-view opening (no cross mullion,
       so the pitch view stays unobstructed) */
    applyMaterialForObject("metal");
    glColor3f(0.12f, 0.12f, 0.14f);
    float nft = 0.25f;
    drawFlatQuadXY(nwx1 - nft, nwy2 - nft, nwx2 + nft, nwy2, VIP_NEAR_Z - 0.02f, 1.0f);
    drawFlatQuadXY(nwx1 - nft, nwy1, nwx2 + nft, nwy1 + nft, VIP_NEAR_Z - 0.02f, 1.0f);
    drawFlatQuadXY(nwx1 - nft, nwy1, nwx1, nwy2, VIP_NEAR_Z - 0.02f, 1.0f);
    drawFlatQuadXY(nwx2, nwy1, nwx2 + nft, nwy2, VIP_NEAR_Z - 0.02f, 1.0f);

    /* very faint, near-clear glass, so the match stays easy to watch */
    applyMaterialForObject("glass");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.80f, 0.92f, 0.97f, 0.10f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
        glVertex3f(nwx1, nwy1, VIP_NEAR_Z - 0.04f);
        glVertex3f(nwx2, nwy1, VIP_NEAR_Z - 0.04f);
        glVertex3f(nwx2, nwy2, VIP_NEAR_Z - 0.04f);
        glVertex3f(nwx1, nwy2, VIP_NEAR_Z - 0.04f);
    glEnd();
    glDisable(GL_BLEND);

    applyMaterialForObject("plastic");

    /* far wall - four maroon strips framing a rectangular window opening */
    float fx1 = -halfW + 2.5f, fx2 = halfW - 2.5f;
    float fy1 = VIP_BASE_Y + 0.8f, fy2 = topY - 0.8f;

    glColor3f(0.55f, 0.10f, 0.12f);
    drawFlatQuadXY(-halfW, VIP_BASE_Y, fx1, topY, VIP_FAR_Z, -1.0f);   /* left strip   */
    drawFlatQuadXY(fx2, VIP_BASE_Y, halfW, topY, VIP_FAR_Z, -1.0f);   /* right strip  */
    drawFlatQuadXY(fx1, VIP_BASE_Y, fx2, fy1, VIP_FAR_Z, -1.0f);      /* bottom strip */
    drawFlatQuadXY(fx1, fy2, fx2, topY, VIP_FAR_Z, -1.0f);            /* top strip    */

    /* dark metal window frame + cross mullions, just inside the opening */
    applyMaterialForObject("metal");
    glColor3f(0.12f, 0.12f, 0.14f);
    float ft = 0.25f;
    drawFlatQuadXY(fx1 - ft, fy2 - ft, fx2 + ft, fy2, VIP_FAR_Z + 0.02f, -1.0f);
    drawFlatQuadXY(fx1 - ft, fy1, fx2 + ft, fy1 + ft, VIP_FAR_Z + 0.02f, -1.0f);
    drawFlatQuadXY(fx1 - ft, fy1, fx1, fy2, VIP_FAR_Z + 0.02f, -1.0f);
    drawFlatQuadXY(fx2, fy1, fx2 + ft, fy2, VIP_FAR_Z + 0.02f, -1.0f);

    float midX = (fx1 + fx2) / 2.0f;
    float midY = (fy1 + fy2) / 2.0f;
    drawFlatQuadXY(midX - ft * 0.5f, fy1, midX + ft * 0.5f, fy2, VIP_FAR_Z + 0.02f, -1.0f);
    drawFlatQuadXY(fx1, midY - ft * 0.5f, fx2, midY + ft * 0.5f, VIP_FAR_Z + 0.02f, -1.0f);

    /* faint tinted glass pane, recessed slightly inside the frame -
       real alpha blending, so the outside world still shows through it */
    applyMaterialForObject("glass");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.65f, 0.85f, 0.95f, 0.28f);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glBegin(GL_QUADS);
        glVertex3f(fx1, fy1, VIP_FAR_Z + 0.04f);
        glVertex3f(fx2, fy1, VIP_FAR_Z + 0.04f);
        glVertex3f(fx2, fy2, VIP_FAR_Z + 0.04f);
        glVertex3f(fx1, fy2, VIP_FAR_Z + 0.04f);
    glEnd();
    glDisable(GL_BLEND);
}

/* Colourful furniture, reusing the existing drawBox() helper (unchanged),
   plus a small strip of bright ceiling lights. */
void drawVIPFurniture()
{
    applyMaterialForObject("plastic");
    float midZ = (VIP_NEAR_Z + VIP_FAR_Z) / 2.0f;

    drawBox(0.0f, VIP_NEAR_Z - 1.4f, 9.0f, 1.6f, VIP_BASE_Y, 1.3f, 0.55f, 0.06f, 0.10f);   /* red sofa       */
    drawBox(0.0f, midZ, 3.2f, 1.8f, VIP_BASE_Y, 0.55f, 0.75f, 0.60f, 0.20f);               /* gold table     */
    drawBox(-4.5f, midZ - 1.0f, 1.3f, 1.3f, VIP_BASE_Y, 1.0f, 0.10f, 0.20f, 0.78f);        /* blue armchair  */
    drawBox( 4.5f, midZ - 1.0f, 1.3f, 1.3f, VIP_BASE_Y, 1.0f, 0.10f, 0.70f, 0.30f);        /* green armchair */

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.97f, 0.85f);
    for (float z = VIP_FAR_Z + 1.5f; z < VIP_NEAR_Z - 1.0f; z += 2.5f)
        drawFlatQuadXZ(-1.0f, z - 0.4f, 1.0f, z + 0.4f, VIP_BASE_Y + VIP_HEIGHT - 0.05f, -1.0f);
    glEnable(GL_LIGHTING);
}

/* Extra colour on top of the sofa - 4 differently-coloured cushions,
   built with the existing drawBox() helper (unchanged). */
void drawVIPCushions()
{
    float sofaZ = VIP_NEAR_Z - 1.4f;
    float cushionY = VIP_BASE_Y + 1.3f;
    float cCol[4][3] = {
        { 0.95f, 0.85f, 0.15f },
        { 0.15f, 0.65f, 0.85f },
        { 0.85f, 0.25f, 0.55f },
        { 0.30f, 0.80f, 0.35f }
    };

    applyMaterialForObject("plastic");
    for (int i = 0; i < 4; i++)
    {
        float x = -3.3f + i * 2.2f;
        drawBox(x, sofaZ - 0.3f, 0.7f, 0.7f, cushionY, 0.35f, cCol[i][0], cCol[i][1], cCol[i][2]);
    }
}

/* Deep-red curtains flanking the window, plus a gold curtain rod. */
void drawVIPCurtains()
{
    float halfW = VIP_WIDTH / 2.0f;
    float fx1 = -halfW + 2.5f, fx2 = halfW - 2.5f;
    float cw = 1.0f;

    applyMaterialForObject("plastic");
    glColor3f(0.65f, 0.05f, 0.10f);
    drawFlatQuadXY(fx1 - cw - 0.3f, VIP_BASE_Y + 0.2f, fx1 - 0.3f, VIP_BASE_Y + VIP_HEIGHT - 0.2f, VIP_FAR_Z + 0.06f, -1.0f);
    drawFlatQuadXY(fx2 + 0.3f, VIP_BASE_Y + 0.2f, fx2 + cw + 0.3f, VIP_BASE_Y + VIP_HEIGHT - 0.2f, VIP_FAR_Z + 0.06f, -1.0f);

    applyMaterialForObject("gold");
    glColor3f(0.85f, 0.68f, 0.15f);
    glPushMatrix();
        glTranslatef(fx1 - cw - 0.3f, VIP_BASE_Y + VIP_HEIGHT - 0.15f, VIP_FAR_Z + 0.07f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.06f, 0.06f, (fx2 + cw + 0.3f) - (fx1 - cw - 0.3f), 8, 1);
        gluDeleteQuadric(q);
    glPopMatrix();
}

/* A wall-mounted TV on the right wall, with a bright unlit "screen glow". */
void drawVIPTV()
{
    float halfW = VIP_WIDTH / 2.0f;
    float midZ  = (VIP_NEAR_Z + VIP_FAR_Z) / 2.0f;

    applyMaterialForObject("plastic");
    glColor3f(0.05f, 0.05f, 0.06f);
    drawFlatQuadYZ(VIP_BASE_Y + 2.3f, midZ - 1.3f, VIP_BASE_Y + 4.3f, midZ + 1.3f, halfW - 0.04f, -1.0f);

    glDisable(GL_LIGHTING);
    glColor3f(0.15f, 0.55f, 0.85f);
    drawFlatQuadYZ(VIP_BASE_Y + 2.45f, midZ - 1.15f, VIP_BASE_Y + 4.15f, midZ + 1.15f, halfW - 0.05f, -1.0f);
    glEnable(GL_LIGHTING);
}

/* A small mini-bar counter (with a gold trim) along the left wall,
   plus a few bottles and two stools. */
void drawVIPBar()
{
    float halfW = VIP_WIDTH / 2.0f;
    float barZ1 = VIP_FAR_Z + 2.0f;
    float barZ2 = VIP_FAR_Z + 5.0f;
    float barMidZ = (barZ1 + barZ2) / 2.0f;
    float barX = -halfW + 0.9f;

    applyMaterialForObject("wood");
    drawBox(barX, barMidZ, 1.4f, barZ2 - barZ1, VIP_BASE_Y, 1.1f, 0.42f, 0.22f, 0.10f);

    applyMaterialForObject("gold");
    drawBox(barX, barMidZ, 1.5f, (barZ2 - barZ1) + 0.1f, VIP_BASE_Y + 1.05f, 0.10f, 0.85f, 0.68f, 0.15f);

    applyMaterialForObject("glass");
    float bottleCol[3][3] = {
        { 0.10f, 0.55f, 0.20f },
        { 0.55f, 0.30f, 0.08f },
        { 0.15f, 0.55f, 0.75f }
    };
    for (int i = 0; i < 3; i++)
    {
        float bz = barZ1 + 0.8f + i * 1.1f;
        glColor3f(bottleCol[i][0], bottleCol[i][1], bottleCol[i][2]);
        glPushMatrix();
            glTranslatef(barX, VIP_BASE_Y + 1.15f, bz);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, 0.08f, 0.08f, 0.35f, 8, 1);
            gluDeleteQuadric(q);
        glPopMatrix();
    }

    applyMaterialForObject("plastic");
    glColor3f(0.85f, 0.10f, 0.15f);
    for (int i = 0; i < 2; i++)
    {
        float sz = barZ1 + 1.2f + i * 2.0f;
        glPushMatrix();
            glTranslatef(barX + 1.4f, VIP_BASE_Y + 0.3f, sz);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, 0.28f, 0.28f, 0.6f, 10, 1);
            gluDeleteQuadric(q);
        glPopMatrix();
    }
}

/* A simple potted plant (trunk pot + a few foliage spheres). */
void drawVIPPlant(float x, float z)
{
    applyMaterialForObject("wood");
    glColor3f(0.35f, 0.20f, 0.10f);
    glPushMatrix();
        glTranslatef(x, VIP_BASE_Y, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.35f, 0.28f, 0.6f, 10, 1);
        gluDeleteQuadric(q);
    glPopMatrix();

    applyMaterialForObject("field");
    glColor3f(0.10f, 0.55f, 0.18f);
    glPushMatrix();
        glTranslatef(x, VIP_BASE_Y + 0.9f, z);
        glutSolidSphere(0.55, 10, 10);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(x + 0.3f, VIP_BASE_Y + 1.15f, z + 0.2f);
        glutSolidSphere(0.35, 8, 8);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(x - 0.28f, VIP_BASE_Y + 1.05f, z - 0.25f);
        glutSolidSphere(0.32, 8, 8);
    glPopMatrix();
}

/* A round, three-tone decorative rug on the carpet, under the table. */
void drawVIPRug()
{
    float midZ = (VIP_NEAR_Z + VIP_FAR_Z) / 2.0f;

    glDisable(GL_LIGHTING);
    glPushMatrix();
        glTranslatef(0.0f, VIP_BASE_Y + 0.015f, midZ);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* q = gluNewQuadric();

        glColor3f(0.85f, 0.68f, 0.15f);
        gluDisk(q, 0.0f, 3.0f, 24, 1);

        glTranslatef(0.0f, 0.0f, 0.005f);
        glColor3f(0.55f, 0.05f, 0.08f);
        gluDisk(q, 0.0f, 2.2f, 24, 1);

        glTranslatef(0.0f, 0.0f, 0.005f);
        glColor3f(0.90f, 0.90f, 0.85f);
        gluDisk(q, 0.0f, 1.3f, 24, 1);

        gluDeleteQuadric(q);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* Two framed "paintings" on the right wall, clear of the TV in the middle. */
void drawVIPWallArt()
{
    float halfW = VIP_WIDTH / 2.0f;

    applyMaterialForObject("wood");
    glColor3f(0.30f, 0.18f, 0.08f);
    drawFlatQuadYZ(VIP_BASE_Y + 3.0f, VIP_NEAR_Z - 3.0f, VIP_BASE_Y + 4.6f, VIP_NEAR_Z - 1.2f, halfW - 0.04f, -1.0f);
    glDisable(GL_LIGHTING);
    glColor3f(0.15f, 0.45f, 0.70f);
    drawFlatQuadYZ(VIP_BASE_Y + 3.15f, VIP_NEAR_Z - 2.85f, VIP_BASE_Y + 4.45f, VIP_NEAR_Z - 1.35f, halfW - 0.05f, -1.0f);
    glEnable(GL_LIGHTING);

    applyMaterialForObject("wood");
    glColor3f(0.30f, 0.18f, 0.08f);
    drawFlatQuadYZ(VIP_BASE_Y + 3.0f, VIP_FAR_Z + 1.2f, VIP_BASE_Y + 4.6f, VIP_FAR_Z + 3.0f, halfW - 0.04f, -1.0f);
    glDisable(GL_LIGHTING);
    glColor3f(0.75f, 0.20f, 0.20f);
    drawFlatQuadYZ(VIP_BASE_Y + 3.15f, VIP_FAR_Z + 1.35f, VIP_BASE_Y + 4.45f, VIP_FAR_Z + 2.85f, halfW - 0.05f, -1.0f);
    glEnable(GL_LIGHTING);
}

/* Three colourful hanging pendant lights along the ceiling centre line. */
void drawVIPPendantLights()
{
    float topY = VIP_BASE_Y + VIP_HEIGHT;
    float col[3][3] = {
        { 1.0f, 0.85f, 0.30f },
        { 0.30f, 0.85f, 1.0f },
        { 1.0f, 0.40f, 0.60f }
    };

    glDisable(GL_LIGHTING);
    int idx = 0;
    for (float z = VIP_FAR_Z + 2.0f; z < VIP_NEAR_Z - 1.5f; z += 3.0f, idx++)
    {
        glColor3f(0.25f, 0.25f, 0.25f);
        glPushMatrix();
            glTranslatef(0.0f, topY - 0.6f, z);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, 0.02f, 0.02f, 0.6f, 6, 1);
            gluDeleteQuadric(q);
        glPopMatrix();

        int c = idx % 3;
        glColor3f(col[c][0], col[c][1], col[c][2]);
        glPushMatrix();
            glTranslatef(0.0f, topY - 1.2f, z);
            glutSolidSphere(0.22, 10, 10);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

/* A small gold trophy on a wooden shelf, near a corner of the room. */
void drawVIPTrophyShelf()
{
    float halfW = VIP_WIDTH / 2.0f;
    float shelfX = halfW - 1.6f;
    float shelfZ = VIP_NEAR_Z - 0.6f;

    applyMaterialForObject("wood");
    drawBox(shelfX, shelfZ, 1.2f, 0.5f, VIP_BASE_Y + 1.6f, 0.12f, 0.35f, 0.20f, 0.10f);

    applyMaterialForObject("gold");
    glColor3f(0.90f, 0.75f, 0.20f);
    glPushMatrix();
        glTranslatef(shelfX, VIP_BASE_Y + 1.72f, shelfZ);
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, 0.10f, 0.16f, 0.30f, 10, 1);
            gluDeleteQuadric(q);
        glPopMatrix();
        glTranslatef(0.0f, 0.34f, 0.0f);
        glutSolidSphere(0.16, 10, 10);
    glPopMatrix();
}

/* Master decoration function: gathers every extra colourful element -
   cushions, curtains, TV, mini-bar, plants, rug, wall art, pendant
   lights, and the trophy shelf - into the finished, lived-in VIP room. */
void drawVIPDecor()
{
    float halfW = VIP_WIDTH / 2.0f;

    drawVIPCushions();
    drawVIPCurtains();
    drawVIPTV();
    drawVIPBar();
    drawVIPPlant(-halfW + 1.2f, VIP_FAR_Z + 1.2f);
    drawVIPPlant( halfW - 1.2f, VIP_FAR_Z + 1.2f);
    drawVIPRug();
    drawVIPWallArt();
    drawVIPPendantLights();
    drawVIPTrophyShelf();
}

/* Master function: supports, structure (with the real open window),
   furniture, and the VIP sign - the complete VIP room. */
void drawVIPRoom()
{
    drawVIPSupports();
    drawVIPStructure();
    drawVIPFurniture();
    drawVIPDecor();
    drawVIPSign();
}

/* ======================================================================
   STEP 20 : OPTIMIZATION
   The crowd calls drawSpectator() hundreds of times, every single frame.
   glutSolidCube()/glutSolidSphere() rebuild their vertex data (with
   trig calls) EVERY time they are called, which is wasted work since a
   spectator's *shape* never changes - only its position and colour do.
   A display list compiles that shape once; after that, glCallList()
   just replays the cached vertex data, which is much cheaper. */
GLuint spectatorList = 0;

void buildSpectatorDisplayList()
{
    spectatorList = glGenLists(1);
    glNewList(spectatorList, GL_COMPILE);
        glPushMatrix();
            glScalef(0.6f, 0.9f, 0.6f);
            glutSolidCube(1.0f);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(0.0f, 0.65f, 0.0f);
            glutSolidSphere(0.28, 6, 6);
        glPopMatrix();
    glEndList();
}

/* ======================================================================
   CROWD  (STEP 3, optimized in STEP 20 with a display list)
   ====================================================================== */
void drawSpectator(float x, float y, float z, float r, float g, float b)
{
    glPushMatrix();
        glTranslatef(x, y, z);
        glColor3f(r, g, b);
        glCallList(spectatorList);
    glPopMatrix();
}

void crowdColor(int idx, float &r, float &g, float &b)
{
    float palette[5][3] = {
        { 0.80f, 0.10f, 0.10f },
        { 0.95f, 0.95f, 0.95f },
        { 0.10f, 0.20f, 0.80f },
        { 0.95f, 0.85f, 0.10f },
        { 0.30f, 0.30f, 0.32f }
    };
    int pick = (idx * 7) % 5;
    r = palette[pick][0];
    g = palette[pick][1];
    b = palette[pick][2];
}

void drawCrowd()
{
    float spacing = 3.0f;

    for (int i = 0; i < NUM_TIERS; i++)
    {
        float baseY   = i * TIER_HEIGHT;
        float tierPos = STAND_GAP + i * TIER_DEPTH + TIER_DEPTH / 2.0f;
        float crowdY  = baseY + TIER_HEIGHT + 0.55f;

        float touchLimit = FL + OUTER_EXTENT - 5.0f;
        int idx = 0;
        for (float x = -touchLimit; x <= touchLimit; x += spacing, idx++)
        {
            float r, g, b;
            crowdColor(idx + i * 17, r, g, b);
            drawSpectator(x, crowdY,  (FW + tierPos), r, g, b);

            crowdColor(idx + i * 23 + 3, r, g, b);
            drawSpectator(x, crowdY, -(FW + tierPos), r, g, b);
        }

        float endLimit = FW - 2.0f;
        idx = 0;
        for (float z = -endLimit; z <= endLimit; z += spacing, idx++)
        {
            float r, g, b;
            crowdColor(idx + i * 11 + 1, r, g, b);
            drawSpectator( (FL + tierPos), crowdY, z, r, g, b);

            crowdColor(idx + i * 29 + 5, r, g, b);
            drawSpectator(-(FL + tierPos), crowdY, z, r, g, b);
        }
    }
}

/* ======================================================================
   TEAM BANNERS  (STEP 3)
   ====================================================================== */
void drawBanner(float cx, float fixedZ, float width, float baseY, float height,
                float r1, float g1, float b1, float r2, float g2, float b2)
{
    float x1 = cx - width / 2.0f;
    float xm = cx;
    float x2 = cx + width / 2.0f;

    /* the banner faces the pitch, so its normal points toward it */
    glNormal3f(0.0f, 0.0f, (fixedZ > 0.0f) ? -1.0f : 1.0f);

    glBegin(GL_QUADS);
        glColor3f(r1, g1, b1);
        glVertex3f(x1, baseY,          fixedZ);
        glVertex3f(xm, baseY,          fixedZ);
        glVertex3f(xm, baseY + height, fixedZ);
        glVertex3f(x1, baseY + height, fixedZ);

        glColor3f(r2, g2, b2);
        glVertex3f(xm, baseY,          fixedZ);
        glVertex3f(x2, baseY,          fixedZ);
        glVertex3f(x2, baseY + height, fixedZ);
        glVertex3f(xm, baseY + height, fixedZ);
    glEnd();
}

void drawBanners()
{
    float topBaseY = (NUM_TIERS - 1) * TIER_HEIGHT + 1.0f;
    float bannerH  = TIER_HEIGHT * 0.55f;
    float frontZ   = FW + STAND_GAP + (NUM_TIERS - 1) * TIER_DEPTH - 0.05f;

    float positionsX[4] = { -FL * 0.55f, -FL * 0.15f, FL * 0.15f, FL * 0.55f };

    for (int i = 0; i < 4; i++)
    {
        if (i % 2 == 0)
        {
            drawBanner(positionsX[i],  frontZ, 14.0f, topBaseY, bannerH, 0.85f, 0.1f, 0.1f, 1.0f, 1.0f, 1.0f);
            drawBanner(positionsX[i], -frontZ, 14.0f, topBaseY, bannerH, 0.1f, 0.2f, 0.85f, 1.0f, 0.9f, 0.1f);
        }
        else
        {
            drawBanner(positionsX[i],  frontZ, 14.0f, topBaseY, bannerH, 1.0f, 1.0f, 1.0f, 0.85f, 0.1f, 0.1f);
            drawBanner(positionsX[i], -frontZ, 14.0f, topBaseY, bannerH, 1.0f, 0.9f, 0.1f, 0.1f, 0.2f, 0.85f);
        }
    }
}

/* ======================================================================
   STADIUM FLAGS  (STEP 3)
   ====================================================================== */
void drawStadiumFlag(float x, float y, float z, float r, float g, float b)
{
    glPushMatrix();
        glTranslatef(x, y, z);

        glColor3f(0.85f, 0.85f, 0.85f);
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluCylinder(q, 0.12f, 0.12f, 4.0f, 8, 1);
            gluDeleteQuadric(q);
        glPopMatrix();

        glColor3f(r, g, b);
        glTranslatef(0.0f, 3.6f, 0.0f);
        glBegin(GL_TRIANGLES);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(2.2f, 0.5f, 0.0f);
            glVertex3f(0.0f, 1.2f, 0.0f);
        glEnd();
    glPopMatrix();
}

void drawStadiumFlags()
{
    float topY  = NUM_TIERS * TIER_HEIGHT;
    float roofY = topY + 2.0f;

    float fx = FL + OUTER_EXTENT + 3.0f;
    float fz = FW + OUTER_EXTENT + 3.0f;
    float flagBaseY = roofY + 3.0f;

    drawStadiumFlag( fx, flagBaseY,  fz, 0.85f, 0.1f, 0.1f);
    drawStadiumFlag( fx, flagBaseY, -fz, 0.1f, 0.2f, 0.85f);
    drawStadiumFlag(-fx, flagBaseY,  fz, 0.1f, 0.2f, 0.85f);
    drawStadiumFlag(-fx, flagBaseY, -fz, 0.85f, 0.1f, 0.1f);
}

/* ======================================================================
   ELECTRONIC SCOREBOARD  (STEP 3)
   ====================================================================== */
void drawPanelText(float x, float y, float z, float charHeight, const char* text)
{
    float scale = charHeight / 119.05f;
    float widthFontUnits = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)text);
    float widthWorld = widthFontUnits * scale;

    glDisable(GL_LIGHTING);            /* glowing LED text, not lit by lights */
    glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(-widthWorld / 2.0f, 0.0f, 0.0f);
        glScalef(scale, scale, scale);
        for (const char* c = text; *c != '\0'; c++)
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawScoreboard()
{
    float topY  = NUM_TIERS * TIER_HEIGHT;
    float roofY = topY + 2.0f;

    float boardW     = 34.0f;
    float boardH     = 9.0f;
    float boardThick = 1.2f;
    float boardBaseY = roofY + 6.0f;
    float boardX     = -(FL + OUTER_EXTENT + 18.0f);

    glColor3f(0.15f, 0.15f, 0.15f);
    drawBox(boardX, -(boardW / 2.0f - 2.0f), 1.0f, 1.0f, 0.0f, boardBaseY, 0.15f, 0.15f, 0.15f);
    drawBox(boardX,  (boardW / 2.0f - 2.0f), 1.0f, 1.0f, 0.0f, boardBaseY, 0.15f, 0.15f, 0.15f);

    drawBox(boardX, 0.0f, boardThick * 0.6f, boardW + 1.5f, boardBaseY - 0.6f, boardH + 1.2f, 0.95f, 0.85f, 0.10f);

    float panelX = boardX + 0.5f;
    drawBox(panelX, 0.0f, boardThick, boardW, boardBaseY, boardH, 0.03f, 0.05f, 0.08f);

    glColor3f(1.0f, 1.0f, 1.0f);
    float faceX = panelX + boardThick / 2.0f + 0.05f;

    char line1[64];
    sprintf(line1, "TEAM A   %d - %d   TEAM B", teamAScore, teamBScore);
    drawPanelText(faceX, boardBaseY + boardH * 0.62f, 0.0f, 1.6f, line1);
    drawPanelText(faceX, boardBaseY + boardH * 0.25f, 0.0f, 1.6f, matchTimeText);
}

/* GOAL message + restart message (bitmap font, screen-facing) */
void drawGoalMessage()
{
    char msg[40];
    sprintf(msg, "***  GOAL !  TEAM %s SCORES  ***", (celebrationTeam == 0) ? "A" : "B");

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.85f, 0.05f);
    glRasterPos3f(-11.0f, 15.0f, 0.0f);
    for (const char* c = msg; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    glEnable(GL_LIGHTING);
}

void drawRestartMessage()
{
    if (!restartPending) return;

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.90f, 0.10f);
    glRasterPos3f(-9.0f, 12.0f, 0.0f);
    for (const char* c = restartMessage; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    glEnable(GL_LIGHTING);
}

/* ======================================================================
   PLAYERS  (STEP 4/5, kicking animation STEP 8, celebration STEP 9)
   ====================================================================== */
void drawCylinderZ(float radius, float length, int slices)
{
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, radius, radius, length, slices, 1);

    gluDisk(q, 0.0f, radius, slices, 1);

    glPushMatrix();
        glTranslatef(0.0f, 0.0f, length);
        gluDisk(q, 0.0f, radius, slices, 1);
    glPopMatrix();

    gluDeleteQuadric(q);
}

void drawLimbDown(float radius, float length)
{
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        drawCylinderZ(radius, length, 10);
    glPopMatrix();
}

void drawPlayer(float x, float z, float facingDeg,
                float shirtR, float shirtG, float shirtB,
                float shortsR, float shortsG, float shortsB,
                int isGK, float phase, float legAmplitude,
                bool isKicking, float kickProgress,
                bool isCelebrating)
{
    const float SKIN_R = 0.86f, SKIN_G = 0.68f, SKIN_B = 0.52f;

    float leftLegAngle, rightLegAngle, leftArmAngle, rightArmAngle;
    float bounce = 0.0f;

    if (isCelebrating)
    {
        leftArmAngle  = 165.0f;
        rightArmAngle = -165.0f;
        leftLegAngle  = 0.0f;
        rightLegAngle = 0.0f;
        bounce = fabsf(sinf(animTime * 5.0f + x * 0.3f + z * 0.3f)) * 0.35f;
    }
    else if (isKicking)
    {
        float kickSwing = -35.0f + kickProgress * 110.0f;
        rightLegAngle = kickSwing;
        leftLegAngle  = -8.0f;
        rightArmAngle = -kickSwing * 0.25f;
        leftArmAngle  =  kickSwing * 0.25f;
    }
    else
    {
        float legSwing = sinf(phase) * legAmplitude;
        leftLegAngle  =  legSwing;
        rightLegAngle = -legSwing;
        leftArmAngle  = -legSwing;
        rightArmAngle =  legSwing;
    }

    glPushMatrix();

        glTranslatef(x, bounce, z);
        glRotatef(facingDeg, 0.0f, 1.0f, 0.0f);

        /* LEFT LEG */
        glPushMatrix();
            glTranslatef(0.0f, 0.95f, 0.18f);
            glRotatef(leftLegAngle, 0.0f, 0.0f, 1.0f);

            glColor3f(SKIN_R, SKIN_G, SKIN_B);
            drawLimbDown(0.11f, 0.80f);

            glPushMatrix();
                glTranslatef(0.08f, -0.86f, 0.0f);
                glColor3f(0.08f, 0.08f, 0.10f);
                glScalef(0.34f, 0.13f, 0.20f);
                glutSolidCube(1.0f);
            glPopMatrix();
        glPopMatrix();

        /* RIGHT LEG */
        glPushMatrix();
            glTranslatef(0.0f, 0.95f, -0.18f);
            glRotatef(rightLegAngle, 0.0f, 0.0f, 1.0f);

            glColor3f(SKIN_R, SKIN_G, SKIN_B);
            drawLimbDown(0.11f, 0.80f);

            glPushMatrix();
                glTranslatef(0.08f, -0.86f, 0.0f);
                glColor3f(0.08f, 0.08f, 0.10f);
                glScalef(0.34f, 0.13f, 0.20f);
                glutSolidCube(1.0f);
            glPopMatrix();
        glPopMatrix();

        /* SHORTS / HIPS */
        glPushMatrix();
            glTranslatef(0.0f, 1.08f, 0.0f);
            glColor3f(shortsR, shortsG, shortsB);
            glScalef(0.34f, 0.42f, 0.54f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* TORSO (jersey) */
        glPushMatrix();
            glTranslatef(0.0f, 1.65f, 0.0f);
            glColor3f(shirtR, shirtG, shirtB);
            glScalef(0.32f, 0.78f, 0.56f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* LEFT ARM */
        glPushMatrix();
            glTranslatef(0.0f, 1.94f, 0.34f);
            if (isGK)
                glRotatef(-28.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(leftArmAngle, 0.0f, 0.0f, 1.0f);

            glColor3f(shirtR, shirtG, shirtB);
            drawLimbDown(0.095f, 0.22f);

            glColor3f(SKIN_R, SKIN_G, SKIN_B);
            glPushMatrix();
                glTranslatef(0.0f, -0.22f, 0.0f);
                drawLimbDown(0.085f, 0.48f);
            glPopMatrix();

            glPushMatrix();
                glTranslatef(0.0f, -0.74f, 0.0f);
                if (isGK) glColor3f(0.95f, 0.95f, 0.95f);
                glutSolidSphere(0.11, 8, 8);
            glPopMatrix();
        glPopMatrix();

        /* RIGHT ARM */
        glPushMatrix();
            glTranslatef(0.0f, 1.94f, -0.34f);
            if (isGK)
                glRotatef(28.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(rightArmAngle, 0.0f, 0.0f, 1.0f);

            glColor3f(shirtR, shirtG, shirtB);
            drawLimbDown(0.095f, 0.22f);

            glColor3f(SKIN_R, SKIN_G, SKIN_B);
            glPushMatrix();
                glTranslatef(0.0f, -0.22f, 0.0f);
                drawLimbDown(0.085f, 0.48f);
            glPopMatrix();

            glPushMatrix();
                glTranslatef(0.0f, -0.74f, 0.0f);
                if (isGK) glColor3f(0.95f, 0.95f, 0.95f);
                glutSolidSphere(0.11, 8, 8);
            glPopMatrix();
        glPopMatrix();

        /* NECK + HEAD */
        glPushMatrix();
            glTranslatef(0.0f, 2.04f, 0.0f);
            glColor3f(SKIN_R, SKIN_G, SKIN_B);
            glPushMatrix();
                glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
                drawCylinderZ(0.09f, 0.12f, 8);
            glPopMatrix();

            glPushMatrix();
                glTranslatef(0.0f, 0.32f, 0.0f);
                glutSolidSphere(0.22, 12, 12);

                glColor3f(0.10f, 0.08f, 0.06f);
                glPushMatrix();
                    glTranslatef(-0.02f, 0.06f, 0.0f);
                    glScalef(0.95f, 0.65f, 1.0f);
                    glutSolidSphere(0.225, 12, 12);
                glPopMatrix();
            glPopMatrix();
        glPopMatrix();

    glPopMatrix();
}

void drawControlMarker(float x, float z)
{
    float bob = 0.15f * sinf(animTime * 2.0f);
    glPushMatrix();
        glTranslatef(x, 3.1f + bob, z);
        glColor3f(1.0f, 0.85f, 0.10f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, 0.0f, 0.14f, 0.30f, 10, 1);
        gluDeleteQuadric(q);
    glPopMatrix();
}

/* FORMATIONS (4-4-2) - used to INITIALISE player positions and by the AI. */
float teamAFormation[11][2] = {
    { -47.0f,   0.0f },
    { -34.0f, -20.0f }, { -34.0f, -7.0f },
    { -34.0f,   7.0f }, { -34.0f, 20.0f },
    { -18.0f, -22.0f }, { -18.0f, -8.0f },
    { -18.0f,   8.0f }, { -18.0f, 22.0f },
    {  -6.0f, -10.0f }, {  -6.0f, 10.0f }
};

float teamBFormation[11][2] = {
    {  47.0f,   0.0f },
    {  34.0f,  20.0f }, {  34.0f,  7.0f },
    {  34.0f,  -7.0f }, {  34.0f, -20.0f },
    {  18.0f,  22.0f }, {  18.0f,  8.0f },
    {  18.0f,  -8.0f }, {  18.0f, -22.0f },
    {   6.0f,  10.0f }, {   6.0f, -10.0f }
};

/* STEP 5 : LIVE PLAYER STATE */
float playerX[2][11];
float playerZ[2][11];
float playerFacing[2][11];
float playerPhase[2][11];
bool  playerIsMoving[2][11];

int controlledTeam  = 0;
int controlledIndex = 9;

bool keyW = false, keyA = false, keyS = false, keyD = false;
bool controlledIsMoving = false;

const float PLAYER_SPEED = 0.42f;

void initPlayerPositions()
{
    for (int i = 0; i < 11; i++)
    {
        playerX[0][i] = teamAFormation[i][0];
        playerZ[0][i] = teamAFormation[i][1];
        playerFacing[0][i] = 0.0f;
        playerPhase[0][i]  = i * 0.7f;
        playerIsMoving[0][i] = false;

        playerX[1][i] = teamBFormation[i][0];
        playerZ[1][i] = teamBFormation[i][1];
        playerFacing[1][i] = 180.0f;
        playerPhase[1][i]  = i * 0.9f + 1.3f;
        playerIsMoving[1][i] = false;
    }
}

/* ----------------------------------------------------------------------
   STEP 9 : GOAL DETECTION + CELEBRATION
   ---------------------------------------------------------------------- */
void startCelebration(int scoringTeam)
{
    celebrating           = true;
    celebrationTeam       = scoringTeam;
    celebrationTicksLeft  = CELEBRATION_TICKS_TOTAL;

    ballVX = 0.0f;
    ballVZ = 0.0f;

    passInProgress = false;
    kickingTeam    = -1;
    kickingIndex   = -1;
    kickTicksLeft  = 0;

    possessingTeam  = -1;
    possessingIndex = -1;

    restartPending = false;
    restartType = RESTART_NONE;
    restartTeam = -1;
    restartTicksLeft = 0;
    restartMessage[0] = '\0';
}

void resetAfterGoal()
{
    initPlayerPositions();

    ballX = 0.0f;
    ballY = BALL_RADIUS;
    ballZ = 0.0f;
    ballVX = 0.0f;
    ballVY = 0.0f;
    ballVZ = 0.0f;

    passInProgress  = false;
    possessingTeam  = -1;
    possessingIndex = -1;
    lastTouchTeam   = -1;
}

/* ----------------------------------------------------------------------
   RESTART  (throw-in / goal kick / corner) - placeRestartTaker() runs
   ONCE, when the restart starts, so neither the ball nor the taker creep
   away from the restart spot during the pause.
   ---------------------------------------------------------------------- */
void placeRestartTaker()
{
    /* goal kick -> goalkeeper;  throw-in / corner -> nearest outfield player */
    int taker = 0;
    if (restartType != GOAL_KICK)
    {
        float best = 1.0e9f;
        for (int i = 1; i < 11; i++)
        {
            float dx = playerX[restartTeam][i] - ballX;
            float dz = playerZ[restartTeam][i] - ballZ;
            float d  = dx * dx + dz * dz;
            if (d < best) { best = d; taker = i; }
        }
    }

    /* face the centre of the pitch */
    float dirX = -ballX;
    float dirZ = -ballZ;
    float len  = sqrtf(dirX * dirX + dirZ * dirZ);
    if (len < 0.01f) { dirX = 1.0f; dirZ = 0.0f; len = 1.0f; }
    dirX /= len;
    dirZ /= len;

    float facing = atan2f(-dirZ, dirX) * 180.0f / PI_F;
    if (facing < 0.0f) facing += 360.0f;

    /* taker stands just BEHIND the ball (ball is 0.55 in front of him) */
    playerX[restartTeam][taker] = ballX - dirX * 0.55f;
    playerZ[restartTeam][taker] = ballZ - dirZ * 0.55f;
    playerFacing[restartTeam][taker] = facing;

    controlledTeam  = restartTeam;
    controlledIndex = taker;

    possessingTeam  = restartTeam;
    possessingIndex = taker;
    lastTouchTeam   = restartTeam;

    ballY = BALL_RADIUS;
}

void startRestart(RestartType type, int team)
{
    restartType = type;
    restartTeam = team;
    restartPending = true;
    restartTicksLeft = RESTART_TICKS;

    ballVX = 0.0f;
    ballVY = 0.0f;
    ballVZ = 0.0f;

    passInProgress = false;
    possessingTeam = -1;
    possessingIndex = -1;
    kickingTeam = -1;
    kickingIndex = -1;
    kickTicksLeft = 0;

    if (type == THROW_IN)
        sprintf(restartMessage, "THROW-IN - TEAM %s", team == 0 ? "A" : "B");
    else if (type == GOAL_KICK)
        sprintf(restartMessage, "GOAL KICK - TEAM %s", team == 0 ? "A" : "B");
    else if (type == CORNER_KICK)
        sprintf(restartMessage, "CORNER KICK - TEAM %s", team == 0 ? "A" : "B");

    placeRestartTaker();
}

/* Decide THROW-IN / GOAL KICK / CORNER KICK when the ball leaves play. */
void checkOutOfPlay()
{
    if (restartPending || celebrating) return;

    /* SIDELINE -> THROW-IN. The other team takes it. */
    if (ballZ > FW || ballZ < -FW)
    {
        int team = (lastTouchTeam == 0) ? 1 : 0;

        ballZ = (ballZ > FW) ? FW - 0.1f : -FW + 0.1f;
        ballX = (ballX > FL) ? FL - 0.5f : (ballX < -FL ? -FL + 0.5f : ballX);

        startRestart(THROW_IN, team);
        return;
    }

    /* GOAL LINE -> GOAL KICK or CORNER KICK. */
    if (ballX > FL)
    {
        /* Team B defends +FL. */
        if (lastTouchTeam == 0)
        {
            ballX = FL - 12.0f;
            ballZ = 0.0f;
            startRestart(GOAL_KICK, 1);
        }
        else
        {
            ballX = FL - 0.5f;
            ballZ = (ballZ >= 0.0f) ? FW - 0.5f : -FW + 0.5f;
            startRestart(CORNER_KICK, 0);
        }
        return;
    }

    if (ballX < -FL)
    {
        /* Team A defends -FL. */
        if (lastTouchTeam == 1)
        {
            ballX = -FL + 12.0f;
            ballZ = 0.0f;
            startRestart(GOAL_KICK, 0);
        }
        else
        {
            ballX = -FL + 0.5f;
            ballZ = (ballZ >= 0.0f) ? FW - 0.5f : -FW + 0.5f;
            startRestart(CORNER_KICK, 1);
        }
    }
}

/* Goal only counts inside the goal mouth and below the crossbar. */
void checkGoal()
{
    if (celebrating || restartPending) return;

    bool insideMouth   = fabsf(ballZ) < GOAL_HALF_WIDTH - BALL_RADIUS * 0.3f;
    bool belowCrossbar = ballY < GOAL_HEIGHT;

    if (ballX <= -FL + BALL_RADIUS && insideMouth && belowCrossbar)
    {
        teamBScore++;
        startCelebration(1);
    }
    else if (ballX >= FL - BALL_RADIUS && insideMouth && belowCrossbar)
    {
        teamAScore++;
        startCelebration(0);
    }
}

bool updateControlledPlayer()
{
    if (celebrating || restartPending) return false;
    if (!keyW && !keyA && !keyS && !keyD) return false;

    float facing = playerFacing[controlledTeam][controlledIndex];
    float rad = facing * PI_F / 180.0f;

    float forwardX =  cosf(rad);
    float forwardZ = -sinf(rad);
    float rightX   = -sinf(rad);
    float rightZ   = -cosf(rad);

    float nx = playerX[controlledTeam][controlledIndex];
    float nz = playerZ[controlledTeam][controlledIndex];

    if (keyW) { nx += forwardX * PLAYER_SPEED; nz += forwardZ * PLAYER_SPEED; }
    if (keyS) { nx -= forwardX * PLAYER_SPEED; nz -= forwardZ * PLAYER_SPEED; }
    if (keyD) { nx += rightX   * PLAYER_SPEED; nz += rightZ   * PLAYER_SPEED; }
    if (keyA) { nx -= rightX   * PLAYER_SPEED; nz -= rightZ   * PLAYER_SPEED; }

    float limitX = FL + 6.0f;
    float limitZ = FW + 6.0f;
    if (nx >  limitX) nx =  limitX;
    if (nx < -limitX) nx = -limitX;
    if (nz >  limitZ) nz =  limitZ;
    if (nz < -limitZ) nz = -limitZ;

    playerX[controlledTeam][controlledIndex] = nx;
    playerZ[controlledTeam][controlledIndex] = nz;

    return true;
}

/* ======================================================================
   STEP 10 : SIMPLE AI  -  steering helpers
   ====================================================================== */
void stepToward2D(float &px, float &pz, float tx, float tz, float speed, bool &moved)
{
    float dx = tx - px;
    float dz = tz - pz;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 0.05f) { moved = false; return; }

    float step = (speed > dist) ? dist : speed;
    px += (dx / dist) * step;
    pz += (dz / dist) * step;
    moved = true;

    float limitX = FL + 6.0f;
    float limitZ = FW + 6.0f;
    if (px >  limitX) px =  limitX;
    if (px < -limitX) px = -limitX;
    if (pz >  limitZ) pz =  limitZ;
    if (pz < -limitZ) pz = -limitZ;
}

void stepTowardFacing(float &px, float &pz, float &facing, float tx, float tz, float speed, bool &moved)
{
    float dx = tx - px;
    float dz = tz - pz;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 0.05f) { moved = false; return; }

    float step = (speed > dist) ? dist : speed;
    float dirX = dx / dist;
    float dirZ = dz / dist;
    px += dirX * step;
    pz += dirZ * step;

    facing = atan2f(-dirZ, dirX) * 180.0f / PI_F;
    if (facing < 0.0f) facing += 360.0f;
    moved = true;

    float limitX = FL + 6.0f;
    float limitZ = FW + 6.0f;
    if (px >  limitX) px =  limitX;
    if (px < -limitX) px = -limitX;
    if (pz >  limitZ) pz =  limitZ;
    if (pz < -limitZ) pz = -limitZ;
}

/* GOALKEEPER AI */
void updateGoalkeeperAI(int team)
{
    if (team == controlledTeam && controlledIndex == 0)
    {
        playerIsMoving[team][0] = false;
        return;
    }

    float goalX = (team == 0) ? -FL : FL;
    float advanceDir = (team == 0) ? 1.0f : -1.0f;

    float homeX = goalX + advanceDir * 1.5f;

    float targetZ = ballZ;
    float sideLimit = GOAL_HALF_WIDTH - 0.6f;
    if (targetZ >  sideLimit) targetZ =  sideLimit;
    if (targetZ < -sideLimit) targetZ = -sideLimit;

    bool ballDangerous = fabsf(ballX - goalX) < GK_DANGER_RANGE;
    float targetX = ballDangerous ? (goalX + advanceDir * 3.0f) : homeX;

    bool moved = false;
    stepToward2D(playerX[team][0], playerZ[team][0], targetX, targetZ, GK_SPEED, moved);

    playerFacing[team][0] = (team == 0) ? 0.0f : 180.0f;
    playerIsMoving[team][0] = moved;
}

/* OUTFIELD AI. The pass RECEIVER runs toward the incoming ball. */
void updateOutfieldAI(int team)
{
    float distToBall[11];
    if (team != controlledTeam)
    {
        for (int i = 1; i < 11; i++)
        {
            float dx = ballX - playerX[team][i];
            float dz = ballZ - playerZ[team][i];
            distToBall[i] = sqrtf(dx * dx + dz * dz);
        }
    }

    for (int i = 1; i < 11; i++)
    {
        if (team == controlledTeam && i == controlledIndex)
        {
            playerIsMoving[team][i] = false;
            continue;
        }

        float px = playerX[team][i];
        float pz = playerZ[team][i];
        float formX = (team == 0) ? teamAFormation[i][0] : teamBFormation[i][0];
        float formZ = (team == 0) ? teamAFormation[i][1] : teamBFormation[i][1];

        float dxBall = ballX - px;
        float dzBall = ballZ - pz;
        float distanceToBall = sqrtf(dxBall * dxBall + dzBall * dzBall);

        float targetX, targetZ, speed;

        bool isReceiver = (passInProgress && team == passTargetTeam && i == passTargetIndex);

        if (isReceiver)
        {
            /* the player who is going to receive the pass moves to the ball */
            targetX = ballX;
            targetZ = ballZ;
            speed = AI_CHASE_SPEED;
        }
        else if (team != controlledTeam)
        {
            int closerCount = 0;
            for (int j = 1; j < 11; j++)
                if (j != i && distToBall[j] < distToBall[i]) closerCount++;

            bool isChaser = (closerCount < 3) && (distanceToBall < AI_CHASE_RANGE);

            if (isChaser)
            {
                targetX = ballX;
                targetZ = ballZ;
                speed = AI_CHASE_SPEED;
            }
            else
            {
                targetX = formX;
                targetZ = formZ * 0.6f + ballZ * 0.4f;
                speed = AI_RETURN_SPEED;
            }
        }
        else
        {
            if (distanceToBall < 10.0f)
            {
                targetX = formX * 0.6f + ballX * 0.4f;
                targetZ = formZ * 0.6f + ballZ * 0.4f;
                speed = AI_SUPPORT_SPEED;
            }
            else
            {
                targetX = formX;
                targetZ = formZ;
                speed = AI_RETURN_SPEED;
            }
        }

        bool moved = false;
        float facing = playerFacing[team][i];
        stepTowardFacing(playerX[team][i], playerZ[team][i], facing, targetX, targetZ, speed, moved);
        if (moved) playerFacing[team][i] = facing;
        playerIsMoving[team][i] = moved;
    }
}

void drawTeam(int teamId)
{
    for (int i = 0; i < 11; i++)
    {
        float px = playerX[teamId][i];
        float pz = playerZ[teamId][i];
        float facing = playerFacing[teamId][i];

        float shR, shG, shB;
        float stR, stG, stB;
        int   isGK = (i == 0) ? 1 : 0;

        if (teamId == 0)
        {
            if (isGK) { shR = 0.10f; shG = 0.85f; shB = 0.25f;
                        stR = 0.06f; stG = 0.06f; stB = 0.06f; }
            else      { shR = 0.85f; shG = 0.10f; shB = 0.10f;
                        stR = 0.95f; stG = 0.95f; stB = 0.95f; }
        }
        else
        {
            if (isGK) { shR = 0.95f; shG = 0.80f; shB = 0.10f;
                        stR = 0.06f; stG = 0.06f; stB = 0.06f; }
            else      { shR = 0.10f; shG = 0.22f; shB = 0.85f;
                        stR = 0.10f; stG = 0.22f; stB = 0.85f; }
        }

        float amplitude = playerIsMoving[teamId][i] ? 28.0f : 8.0f;
        bool isControlledPlayer = (teamId == controlledTeam && i == controlledIndex);

        bool  isKicking = (teamId == kickingTeam && i == kickingIndex && kickTicksLeft > 0);
        float kickProgress = 0.0f;
        if (isKicking)
            kickProgress = 1.0f - (float)kickTicksLeft / (float)KICK_TICKS_TOTAL;

        bool isCelebratingPlayer = (celebrating && teamId == celebrationTeam);

        drawPlayer(px, pz, facing, shR, shG, shB, stR, stG, stB, isGK,
                   playerPhase[teamId][i], amplitude, isKicking, kickProgress,
                   isCelebratingPlayer);

        if (isControlledPlayer)
            drawControlMarker(px, pz);
    }
}

void drawAllPlayers()
{
    drawTeam(0);
    drawTeam(1);
}

/* ======================================================================
   FOOTBALL  (black/white pattern comes from a TEXTURE)
   ====================================================================== */
void drawFootball(float x, float y, float z)
{
    glPushMatrix();
        glTranslatef(x, y, z);

        glRotatef(ballRotZ, 0.0f, 0.0f, 1.0f);
        glRotatef(ballRotX, 1.0f, 0.0f, 0.0f);

        applyMaterialForObject("ball");    /* FIX 3 : now includes a small emissive glow */
        glColor3f(1.0f, 1.0f, 1.0f);
        useTexture(texBall);

        GLUquadric* q = gluNewQuadric();
        gluQuadricTexture(q, GL_TRUE);
        gluQuadricNormals(q, GLU_SMOOTH);
        gluSphere(q, BALL_RADIUS, 24, 16);
        gluDeleteQuadric(q);

        noTexture();
    glPopMatrix();
}

/* ----------------------------------------------------------------------
   tryPass()
   - ignored while a restart pause or another pass is running
   - the goalkeeper (index 0) is never chosen as a target
   - a teammate standing outside the pitch is never chosen
   - the ball is launched toward the target; updateBall() keeps
     re-aiming it every tick (homing pass)
   ---------------------------------------------------------------------- */
void tryPass()
{
    if (celebrating || restartPending || passInProgress) return;
    if (!(possessingTeam == controlledTeam && possessingIndex == controlledIndex))
        return;

    float px = playerX[controlledTeam][controlledIndex];
    float pz = playerZ[controlledTeam][controlledIndex];
    float facing = playerFacing[controlledTeam][controlledIndex];
    float rad = facing * PI_F / 180.0f;
    float forwardX =  cosf(rad);
    float forwardZ = -sinf(rad);

    int   bestIdx = -1;
    float bestScore = -1000.0f;

    int   nearestIdx = -1;
    float nearestDist = 1.0e9f;

    for (int i = 1; i < 11; i++)              /* i = 0 is the goalkeeper : skipped */
    {
        if (i == controlledIndex) continue;

        float tx = playerX[controlledTeam][i];
        float tz = playerZ[controlledTeam][i];

        /* target must be inside the pitch */
        if (fabsf(tx) > FL - 1.0f || fabsf(tz) > FW - 1.0f) continue;

        float dx = tx - px;
        float dz = tz - pz;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist < 0.01f) continue;

        if (dist < nearestDist) { nearestDist = dist; nearestIdx = i; }

        float dirX = dx / dist;
        float dirZ = dz / dist;
        float alignment = dirX * forwardX + dirZ * forwardZ;

        if (alignment > 0.3f)
        {
            float score = alignment * 40.0f - dist;
            if (score > bestScore) { bestScore = score; bestIdx = i; }
        }
    }

    if (bestIdx == -1) bestIdx = nearestIdx;
    if (bestIdx == -1) return;

    float tx = playerX[controlledTeam][bestIdx];
    float tz = playerZ[controlledTeam][bestIdx];
    float ddx = tx - ballX;
    float ddz = tz - ballZ;
    float ddist = sqrtf(ddx * ddx + ddz * ddz);
    if (ddist < 0.01f) ddist = 0.01f;

    ballVX = (ddx / ddist) * PASS_SPEED;
    ballVZ = (ddz / ddist) * PASS_SPEED;
    ballVY = 0.0f;

    passInProgress  = true;
    passTargetTeam  = controlledTeam;
    passTargetIndex = bestIdx;
    passTicksLeft   = PASS_MAX_TICKS;
    lastTouchTeam   = controlledTeam;

    possessingTeam  = -1;
    possessingIndex = -1;
}

/* tryShoot() - LEFT MOUSE CLICK */
void tryShoot()
{
    if (celebrating || restartPending) return;
    if (!(possessingTeam == controlledTeam && possessingIndex == controlledIndex))
        return;

    float facing = playerFacing[controlledTeam][controlledIndex];

    float rad = facing * PI_F / 180.0f;
    float forwardX =  cosf(rad);
    float forwardZ = -sinf(rad);

    lastTouchTeam = controlledTeam;

    ballVX = forwardX * SHOOT_SPEED;
    ballVZ = forwardZ * SHOOT_SPEED;
    ballVY = SHOOT_LIFT;

    passInProgress = false;

    possessingTeam  = -1;
    possessingIndex = -1;

    kickingTeam   = controlledTeam;
    kickingIndex  = controlledIndex;
    kickTicksLeft = KICK_TICKS_TOTAL;
}

/* ----------------------------------------------------------------------
   updateBall()  - called once per timer tick.

   The pass ball is re-aimed at the receiver's CURRENT position every
   tick ("homing pass"), the receiver runs toward the ball
   (updateOutfieldAI), and a safety timer cancels a pass that
   takes too long, so it can no longer miss / fly off the pitch.
   ---------------------------------------------------------------------- */
void updateBall()
{
    /* during a restart pause the ball simply waits at the restart spot */
    if (restartPending) return;

    float oldBallX = ballX;
    float oldBallZ = ballZ;

    /* --- GRAVITY / VERTICAL TRAJECTORY --- */
    if (ballY > BALL_RADIUS || ballVY != 0.0f)
    {
        ballVY += GRAVITY;
        ballY  += ballVY;
        if (ballY <= BALL_RADIUS)
        {
            ballY  = BALL_RADIUS;
            ballVY = 0.0f;
        }
    }
    bool grounded = (ballY <= BALL_RADIUS + 0.001f);

    if (!celebrating)
    {
        if (passInProgress)
        {
            float tx = playerX[passTargetTeam][passTargetIndex];
            float tz = playerZ[passTargetTeam][passTargetIndex];
            float dx = tx - ballX;
            float dz = tz - ballZ;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist <= RECEIVE_RANGE)
            {
                /* pass completed : receiver becomes the controlled player */
                controlledTeam  = passTargetTeam;
                controlledIndex = passTargetIndex;
                passInProgress  = false;

                /* receiver turns to face the opponent's goal */
                playerFacing[controlledTeam][controlledIndex] = (controlledTeam == 0) ? 0.0f : 180.0f;

                float rf = playerFacing[controlledTeam][controlledIndex] * PI_F / 180.0f;
                float ffX =  cosf(rf);
                float ffZ = -sinf(rf);
                ballX = playerX[controlledTeam][controlledIndex] + ffX * 0.55f;
                ballZ = playerZ[controlledTeam][controlledIndex] + ffZ * 0.55f;

                possessingTeam  = controlledTeam;
                possessingIndex = controlledIndex;
                lastTouchTeam   = controlledTeam;
                ballVX = 0.0f;
                ballVZ = 0.0f;
            }
            else
            {
                /* homing : aim at the receiver's current position */
                ballVX = (dx / dist) * PASS_SPEED;
                ballVZ = (dz / dist) * PASS_SPEED;
                ballX += ballVX;
                ballZ += ballVZ;

                passTicksLeft--;
                if (passTicksLeft <= 0)
                    passInProgress = false;      /* safety : give up, ball becomes free */
            }
        }
        else
        {
            float px = playerX[controlledTeam][controlledIndex];
            float pz = playerZ[controlledTeam][controlledIndex];
            float dx = px - ballX;
            float dz = pz - ballZ;
            float dist = sqrtf(dx * dx + dz * dz);

            bool nowPossessed = grounded && (dist < POSSESS_RANGE);

            if (nowPossessed)
            {
                possessingTeam  = controlledTeam;
                possessingIndex = controlledIndex;
                lastTouchTeam   = controlledTeam;

                float facing = playerFacing[controlledTeam][controlledIndex];
                float rad = facing * PI_F / 180.0f;
                float forwardX =  cosf(rad);
                float forwardZ = -sinf(rad);

                ballX = px + forwardX * 0.55f;
                ballZ = pz + forwardZ * 0.55f;
            }
            else
            {
                possessingTeam  = -1;
                possessingIndex = -1;

                if (grounded)
                {
                    ballVX *= BALL_FRICTION;
                    ballVZ *= BALL_FRICTION;
                    if (fabsf(ballVX) < 0.002f) ballVX = 0.0f;
                    if (fabsf(ballVZ) < 0.002f) ballVZ = 0.0f;
                }

                ballX += ballVX;
                ballZ += ballVZ;
            }
        }
    }

    /* --- ROTATION / SPIN --- */
    float movedX = ballX - oldBallX;
    float movedZ = ballZ - oldBallZ;

    ballRotZ +=  (movedX / BALL_RADIUS) * (180.0f / PI_F);
    ballRotX += -(movedZ / BALL_RADIUS) * (180.0f / PI_F);

    if (ballRotZ > 360.0f) ballRotZ -= 360.0f;
    if (ballRotZ < -360.0f) ballRotZ += 360.0f;
    if (ballRotX > 360.0f) ballRotX -= 360.0f;
    if (ballRotX < -360.0f) ballRotX += 360.0f;

    if (!passInProgress && possessingTeam == controlledTeam && possessingIndex == controlledIndex)
    {
        ballVX = movedX;
        ballVZ = movedZ;
    }

    checkGoal();

    if (!celebrating)
        checkOutOfPlay();
}

/* ======================================================================
   CAMERA SETUP  (STEP 11 : viewing transformation)
   ====================================================================== */
void setupCamera()
{
    float targetX = 0.0f;
    float targetY = camTargetY;
    float targetZ = 0.0f;

    if (cameraMode == 1)
    {
        targetX = playerX[controlledTeam][controlledIndex];
        targetY = 2.5f;
        targetZ = playerZ[controlledTeam][controlledIndex];
    }
    else if (cameraMode == 2)
    {
        targetX = FL;
        targetY = 3.0f;
        targetZ = 0.0f;
    }
    else if (cameraMode == 3)
    {
        targetX = 0.0f;
        targetY = 0.0f;
        targetZ = 0.0f;
    }
    else if (cameraMode == 4)
    {
        /* VIP Room View : always look at the ball, as if watching the
           match through the room's pitch-facing window. */
        targetX = ballX;
        targetY = ballY + 1.0f;
        targetZ = ballZ;
    }

    float rad = camAngle * PI_F / 180.0f;
    float pitch = camPitch * PI_F / 180.0f;

    float eyeX = targetX + camDist * cosf(pitch) * cosf(rad);
    float eyeZ = targetZ + camDist * cosf(pitch) * sinf(rad);
    float eyeY = targetY + camDist * sinf(pitch);

    if (cameraMode == 3)
    {
        eyeX = 0.0f;
        eyeY = 260.0f;
        eyeZ = 0.1f;
    }
    else if (cameraMode == 4)
    {
        /* fixed seat inside the VIP room, near the outward window,
           looking forward through the room and out the match-view window */
        eyeX = 0.0f;
        eyeY = VIP_BASE_Y + 2.0f;
        eyeZ = VIP_FAR_Z + 3.0f;
    }

    gluLookAt(eyeX, eyeY, eyeZ,
              targetX, targetY, targetZ,
              0.0, 1.0, 0.0);
}

/* ======================================================================
   STEP 12 : FLOODLIGHT TOWERS + REAL OPENGL LIGHT SOURCES
   ====================================================================== */
void drawFloodlightTower(float x, float z)
{
    float topY = NUM_TIERS * TIER_HEIGHT + 13.0f;
    float towerTint[3] = { 0.45f, 0.48f, 0.55f };

    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        /* Tall metal tower (textured with the metal texture) */
        glPushMatrix();
            glTranslatef(0.0f, topY / 2.0f, 0.0f);
            drawTexturedCube(1.0f, topY, 1.0f, texMetal, towerTint, texMetal, towerTint, 6.0f);
        glPopMatrix();

        /* Four support legs */
        for (int i = -1; i <= 1; i += 2)
        {
            for (int j = -1; j <= 1; j += 2)
            {
                glPushMatrix();
                    glTranslatef(i * 1.4f, topY / 2.0f, j * 1.4f);
                    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                    glColor3f(0.55f, 0.57f, 0.62f);
                    useTexture(texMetal);
                    GLUquadric* q = gluNewQuadric();
                    gluQuadricTexture(q, GL_TRUE);
                    gluCylinder(q, 0.18f, 0.10f, topY, 8, 1);
                    gluDeleteQuadric(q);
                    noTexture();
                glPopMatrix();
            }
        }

        /* Lamp frame */
        glPushMatrix();
            glTranslatef(0.0f, topY, 0.0f);
            glColor3f(0.12f, 0.12f, 0.14f);
            glScalef(5.0f, 1.2f, 1.0f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* Visible bright lamp heads */
        glColor3f(1.0f, 1.0f, 0.75f);
        for (int i = -2; i <= 2; i++)
        {
            glPushMatrix();
                glTranslatef(i * 0.9f, topY - 0.05f, 0.55f);
                glutSolidSphere(0.28f, 10, 8);
            glPopMatrix();
        }
    glPopMatrix();
}

void drawFloodlights()
{
    float sideX = FL + OUTER_EXTENT - 4.0f;
    float sideZ = FW + OUTER_EXTENT - 4.0f;

    drawFloodlightTower( sideX,  sideZ);
    drawFloodlightTower(-sideX,  sideZ);
    drawFloodlightTower( sideX, -sideZ);
    drawFloodlightTower(-sideX, -sideZ);
}

void setupFillLight()
{
    GLfloat position[] = { 0.0f, 100.0f, 0.0f, 1.0f };

    /* STEP 19 : this light behaves like the sun - strong and bright by day,
       almost off at night so the floodlights + moon are what you notice. */
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    ambient[3] = diffuse[3] = specular[3] = 1.0f;

    if (isNightMode)
    {
        ambient[0]  = ambient[1]  = 0.04f; ambient[2]  = 0.06f;
        diffuse[0]  = diffuse[1]  = 0.07f; diffuse[2]  = 0.10f;
        specular[0] = specular[1] = specular[2] = 0.05f;
    }
    else
    {
        ambient[0]  = ambient[1]  = ambient[2]  = 0.22f;
        diffuse[0]  = diffuse[1]  = 0.52f; diffuse[2] = 0.48f;
        specular[0] = specular[1] = specular[2] = 0.25f;
    }

    glEnable(GL_LIGHT4);
    glLightfv(GL_LIGHT4, GL_POSITION, position);
    glLightfv(GL_LIGHT4, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT4, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT4, GL_SPECULAR, specular);
    glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION, 0.0005f);
    glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.00001f);
}

void setupFloodlights()
{
    if (!floodlightsOn)
    {
        for (int i = 0; i < 4; i++)
            glDisable(GL_LIGHT0 + i);
        return;
    }

    GLfloat positions[4][4] = {
        {  85.0f, 55.0f,  75.0f, 1.0f },
        { -85.0f, 55.0f,  75.0f, 1.0f },
        {  85.0f, 55.0f, -75.0f, 1.0f },
        { -85.0f, 55.0f, -75.0f, 1.0f }
    };

    for (int i = 0; i < 4; i++)
    {
        GLenum light = GL_LIGHT0 + i;
        GLfloat ambient[]  = { 0.22f, 0.22f, 0.24f, 1.0f };
        GLfloat diffuse[]  = { 0.72f, 0.72f, 0.68f, 1.0f };
        GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat direction[] = { -positions[i][0], -positions[i][1], -positions[i][2] };

        glEnable(light);
        glLightfv(light, GL_POSITION, positions[i]);
        glLightfv(light, GL_AMBIENT, ambient);
        glLightfv(light, GL_DIFFUSE, diffuse);
        glLightfv(light, GL_SPECULAR, specular);
        glLightfv(light, GL_SPOT_DIRECTION, direction);
        glLightf(light, GL_SPOT_CUTOFF, 45.0f);
        glLightf(light, GL_SPOT_EXPONENT, 12.0f);
        glLightf(light, GL_CONSTANT_ATTENUATION, 0.65f);
        glLightf(light, GL_LINEAR_ATTENUATION, 0.0015f);
        glLightf(light, GL_QUADRATIC_ATTENUATION, 0.00012f);
    }
}

/* ======================================================================
   FIX 2 : SKY DOME (replaces the flat glClearColor "sky") + CLOUDS
   ------------------------------------------------------------------
   drawSkyDome() draws a big unlit hemisphere around the whole scene,
   coloured with a real gradient - lighter near the horizon, deeper
   blue (or, at night, deeper navy/black) toward the zenith. It is
   rendered FIRST, with depth testing and depth writing both switched
   off, so it always sits behind every other object without disturbing
   the depth buffer for anything drawn afterward.
   ====================================================================== */
void drawSkyDome()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    noTexture();

    float topR, topG, topB, horizR, horizG, horizB;
    if (isNightMode)
    {
        /* deep navy overhead, a slightly lighter blue-purple at the horizon */
        topR = 0.01f; topG = 0.015f; topB = 0.05f;
        horizR = 0.05f; horizG = 0.06f; horizB = 0.16f;
    }
    else
    {
        /* rich azure overhead, soft pale blue near the horizon */
        topR = 0.25f; topG = 0.55f; topB = 0.88f;
        horizR = 0.72f; horizG = 0.86f; horizB = 0.97f;
    }

    const float R = 480.0f;
    const int slices = 24;
    const int stacks  = 8;

    for (int i = 0; i < stacks; i++)
    {
        float phi1 = (PI_F * 0.5f) * (float)i     / (float)stacks;  /* 0 = horizon */
        float phi2 = (PI_F * 0.5f) * (float)(i+1) / (float)stacks;

        float y1 = sinf(phi1) * R, r1 = cosf(phi1) * R;
        float y2 = sinf(phi2) * R, r2 = cosf(phi2) * R;

        float t1 = (float)i     / (float)stacks;
        float t2 = (float)(i+1) / (float)stacks;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; j++)
        {
            float th = 2.0f * PI_F * (float)j / (float)slices;
            float cx = cosf(th), cz = sinf(th);

            glColor3f(horizR + (topR - horizR) * t2, horizG + (topG - horizG) * t2, horizB + (topB - horizB) * t2);
            glVertex3f(cx * r2, y2, cz * r2);

            glColor3f(horizR + (topR - horizR) * t1, horizG + (topG - horizG) * t1, horizB + (topB - horizB) * t1);
            glVertex3f(cx * r1, y1, cz * r1);
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

/* One fluffy cloud = a small cluster of scaled, overlapping spheres -
   the same "several primitives grouped together" idea used for the
   tree's foliage, just puffier and wider. Drawn unlit (flat white),
   matching the moon/star's simple, clearly-readable academic style. */
void drawCloudPuff(float x, float y, float z, float scale)
{
    glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(scale, scale * 0.55f, scale * 0.85f);
        glutSolidSphere(1.0, 10, 8);
    glPopMatrix();
}

void drawCloud(float cx, float cy, float cz, float size)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        drawCloudPuff( 0.00f * size, 0.00f, 0.00f * size, size);
        drawCloudPuff( 0.85f * size, 0.12f, 0.15f * size, size * 0.75f);
        drawCloudPuff(-0.90f * size, 0.08f, -0.10f * size, size * 0.70f);
        drawCloudPuff( 0.30f * size, 0.30f, -0.35f * size, size * 0.60f);
        drawCloudPuff(-0.35f * size, 0.28f, 0.40f * size, size * 0.55f);
    glPopMatrix();
}

void initClouds()
{
    for (int i = 0; i < NUM_CLOUDS; i++)
    {
        cloudX[i]    = -ENV_HALF_X * 0.75f + texRand() * (ENV_HALF_X * 1.5f);
        cloudY[i]    = 95.0f + texRand() * 45.0f;
        cloudZ[i]    = -ENV_HALF_Z * 0.75f + texRand() * (ENV_HALF_Z * 1.5f);
        cloudSize[i] = 7.0f + texRand() * 6.0f;
    }
}

void drawClouds()
{
    if (isNightMode) return;   /* night sky is reserved for the moon + stars */

    glDisable(GL_LIGHTING);
    glColor3f(0.99f, 0.99f, 1.0f);

    float span = ENV_HALF_X * 1.5f;
    for (int i = 0; i < NUM_CLOUDS; i++)
    {
        float x = cloudX[i] + cloudDriftX;
        while (x >  ENV_HALF_X * 0.75f) x -= span;
        while (x < -ENV_HALF_X * 0.75f) x += span;
        drawCloud(x, cloudY[i], cloudZ[i], cloudSize[i]);
    }
    glEnable(GL_LIGHTING);
}

/* ======================================================================
   GLUT CALLBACKS
   ====================================================================== */
void drawFloodlightStatus()
{
    glDisable(GL_LIGHTING);
    noTexture();
    glColor3f(1.0f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glRasterPos2f(20, 665);
    const char* msg = floodlightsOn ? "Floodlights: ON (F)" : "Floodlights: OFF (F)";
    for (const char* p = msg; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

    glRasterPos2f(20, 642);
    const char* modeMsg = isNightMode ? "Mode: NIGHT (N / M)" : "Mode: DAY (N / M)";
    for (const char* p = modeMsg; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

/* STEP 20 : small always-available controls legend (bottom-left corner),
   toggled with 'H' so it can be hidden for a clean screenshot/video. */
void drawControlsHelp()
{
    if (!showHelp) return;

    glDisable(GL_LIGHTING);
    noTexture();
    glColor3f(1.0f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 700);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    const char* lines[] = {
        "CONTROLS  (H to hide)",
        "WASD: move   Arrows: turn/snap   V: switch player",
        "SPACE: pass   Left click: shoot",
        "Mouse drag (right btn): look around   Wheel / PgUp,PgDn: zoom",
        "1 Free  2 Follow  3 Overview  4 Goal  5 VIP Room",
        "F: floodlights   N: night   M: day   ESC: quit"
    };
    int numLines = 6;

    for (int i = 0; i < numLines; i++)
    {
        glRasterPos2f(20, 20 + (numLines - 1 - i) * 18);
        for (const char* p = lines[i]; *p; ++p)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

/* ======================================================================
   STEP 18 : OUTSIDE ENVIRONMENT
   Order (near -> far from the stadium wall):
     plaza (already drawn by drawGround()) -> road loop -> moving cars
     -> ring of trees / street lights -> small buildings -> parking area
   ====================================================================== */

/* big flat green field around everything, drawn UNDER the plaza / road,
   so the world doesn't just stop at a sharp edge */
void drawOutsideGround()
{
    setFieldMaterial();
    glColor3f(0.30f, 0.50f, 0.27f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(-ENV_HALF_X, -0.08f, -ENV_HALF_Z);
        glVertex3f( ENV_HALF_X, -0.08f, -ENV_HALF_Z);
        glVertex3f( ENV_HALF_X, -0.08f,  ENV_HALF_Z);
        glVertex3f(-ENV_HALF_X, -0.08f,  ENV_HALF_Z);
    glEnd();
}

/* Rectangular ring road around the stadium, made of 4 straight, textured
   strips plus 4 corner squares (kept separate so they never z-fight). */
void drawRoadLoop()
{
    applyMaterialForObject("concrete");
    useTexture(texRoad);
    glColor3f(1.0f, 1.0f, 1.0f);

    float y  = 0.015f;
    float hw = ROAD_WIDTH / 2.0f;

    drawTexturedGrid(-ROAD_HALF_X + hw,  ROAD_HALF_Z - hw,  ROAD_HALF_X - hw,  ROAD_HALF_Z + hw, y, 20, 2, 8.0f); /* top    */
    drawTexturedGrid(-ROAD_HALF_X + hw, -ROAD_HALF_Z - hw,  ROAD_HALF_X - hw, -ROAD_HALF_Z + hw, y, 20, 2, 8.0f); /* bottom */
    drawTexturedGrid( ROAD_HALF_X - hw, -ROAD_HALF_Z + hw,  ROAD_HALF_X + hw,  ROAD_HALF_Z - hw, y, 2, 20, 8.0f); /* right  */
    drawTexturedGrid(-ROAD_HALF_X - hw, -ROAD_HALF_Z + hw, -ROAD_HALF_X + hw,  ROAD_HALF_Z - hw, y, 2, 20, 8.0f); /* left   */

    drawTexturedGrid( ROAD_HALF_X - hw,  ROAD_HALF_Z - hw,  ROAD_HALF_X + hw,  ROAD_HALF_Z + hw, y, 2, 2, 8.0f); /* corners */
    drawTexturedGrid( ROAD_HALF_X - hw, -ROAD_HALF_Z - hw,  ROAD_HALF_X + hw, -ROAD_HALF_Z + hw, y, 2, 2, 8.0f);
    drawTexturedGrid(-ROAD_HALF_X - hw,  ROAD_HALF_Z - hw, -ROAD_HALF_X + hw,  ROAD_HALF_Z + hw, y, 2, 2, 8.0f);
    drawTexturedGrid(-ROAD_HALF_X - hw, -ROAD_HALF_Z - hw, -ROAD_HALF_X + hw, -ROAD_HALF_Z + hw, y, 2, 2, 8.0f);

    noTexture();
}

/* Converts a distance travelled along the rectangular road loop into a
   world (x,z) position and a facing angle - this is what makes the cars
   drive around the stadium in a loop. Same facing formula as the player /
   AI code (facing = atan2(-dirZ,dirX)) so glRotatef(facing,0,1,0) works. */
void getRoadPosition(float dist, float &x, float &z, float &heading)
{
    float d = fmodf(dist, ROAD_PERIMETER);
    if (d < 0.0f) d += ROAD_PERIMETER;

    float segA = 2.0f * ROAD_HALF_Z;   /* length of the +X edge (north -> south) */
    float segB = 2.0f * ROAD_HALF_X;   /* length of the -Z edge (east  -> west ) */

    if (d < segA)
    {
        x = ROAD_HALF_X;
        z = ROAD_HALF_Z - d;
        heading = 90.0f;
    }
    else if (d < segA + segB)
    {
        float t = d - segA;
        x = ROAD_HALF_X - t;
        z = -ROAD_HALF_Z;
        heading = 180.0f;
    }
    else if (d < segA + segB + segA)
    {
        float t = d - segA - segB;
        x = -ROAD_HALF_X;
        z = -ROAD_HALF_Z + t;
        heading = 270.0f;
    }
    else
    {
        float t = d - segA - segB - segA;
        x = -ROAD_HALF_X + t;
        z = ROAD_HALF_Z;
        heading = 0.0f;
    }
}

/* Simple hierarchical car : body -> cabin -> 4 wheels. */
void drawCar(float x, float y, float z, float facing, float r, float g, float b)
{
    glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(facing, 0.0f, 1.0f, 0.0f);

        /* body */
        applyMaterialForObject("plastic");
        glColor3f(r, g, b);
        glPushMatrix();
            glTranslatef(0.0f, 0.45f, 0.0f);
            glScalef(1.8f, 0.55f, 0.9f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* cabin (slightly darker tint of the body colour) */
        glColor3f(r * 0.55f + 0.15f, g * 0.55f + 0.15f, b * 0.55f + 0.15f);
        glPushMatrix();
            glTranslatef(-0.10f, 0.85f, 0.0f);
            glScalef(0.9f, 0.42f, 0.78f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* 4 wheels, hung from the corners of the body (hierarchical) */
        applyMaterialForObject("metal");
        glColor3f(0.07f, 0.07f, 0.07f);
        float wx[2] = { 0.62f, -0.62f };
        float wz[2] = { 0.50f, -0.50f };
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                glPushMatrix();
                    glTranslatef(wx[i], 0.22f, wz[j]);
                    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
                    GLUquadric* q = gluNewQuadric();
                    gluCylinder(q, 0.22f, 0.22f, 0.18f, 10, 1);
                    gluDeleteQuadric(q);
                glPopMatrix();
            }
        }

        /* headlights */
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.85f);
        glPushMatrix();
            glTranslatef(0.90f, 0.45f, 0.30f);
            glutSolidSphere(0.08, 6, 6);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.90f, 0.45f, -0.30f);
            glutSolidSphere(0.08, 6, 6);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawMovingCars()
{
    for (int i = 0; i < NUM_CARS; i++)
    {
        float x, z, heading;
        getRoadPosition(carDist[i], x, z, heading);
        drawCar(x, 0.0f, z, heading, carColorR[i], carColorG[i], carColorB[i]);
    }
}

void initCars()
{
    for (int i = 0; i < NUM_CARS; i++)
    {
        carDist[i]   = (ROAD_PERIMETER / NUM_CARS) * i;
        carSpeed[i]  = 0.55f + texRand() * 0.35f;
        carColorR[i] = 0.25f + texRand() * 0.65f;
        carColorG[i] = 0.25f + texRand() * 0.65f;
        carColorB[i] = 0.25f + texRand() * 0.65f;
    }
}

void updateCars()
{
    for (int i = 0; i < NUM_CARS; i++)
    {
        carDist[i] += carSpeed[i];
        if (carDist[i] > ROAD_PERIMETER) carDist[i] -= ROAD_PERIMETER;
    }
}

/* ======================================================================
   STEP 19 : MOON, STARS, DAY/NIGHT
   (the flat glClearColor "sky" has been replaced by drawSkyDome() above)
   ====================================================================== */

/* Scatter NUM_STARS points on a big dome high above the stadium, using
   the same tiny PRNG (texRand) that already builds the textures - so no
   extra library is needed and the star field is different every run. */
void initStars()
{
    for (int i = 0; i < NUM_STARS; i++)
    {
        float az   = texRand() * 360.0f;                 /* angle around the stadium   */
        float elev = 18.0f + texRand() * 65.0f;           /* how high in the sky (deg)  */
        float dist = 260.0f + texRand() * 120.0f;

        float azRad   = az   * PI_F / 180.0f;
        float elevRad = elev * PI_F / 180.0f;

        starX[i] = dist * cosf(elevRad) * cosf(azRad);
        starY[i] = 40.0f + dist * sinf(elevRad);
        starZ[i] = dist * cosf(elevRad) * sinf(azRad);
    }
}

/* Small white dots, unlit so they stay visible against the dark night
   sky from any camera angle. Only drawn in night mode. */
void drawStars()
{
    if (!isNightMode) return;

    glDisable(GL_LIGHTING);
    glColor3f(0.95f, 0.95f, 1.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
        for (int i = 0; i < NUM_STARS; i++)
            glVertex3f(starX[i], starY[i], starZ[i]);
    glEnd();
    glPointSize(5.0f);              /* restore the point size used by field-marking dots */
    glEnable(GL_LIGHTING);
}

/* A simple hierarchical moon : a pale sphere plus a few darker "crater"
   spheres stuck to its surface. Drawn unlit (like the LED scoreboard
   text) so it glows steadily regardless of the floodlights. */
void drawMoon()
{
    if (!isNightMode) return;

    glDisable(GL_LIGHTING);
    glPushMatrix();
        glTranslatef(150.0f, 190.0f, -250.0f);

        glColor3f(0.90f, 0.90f, 0.84f);
        glutSolidSphere(9.0, 20, 20);

        glColor3f(0.72f, 0.72f, 0.68f);
        glPushMatrix(); glTranslatef( 3.2f,  2.0f, 7.6f); glutSolidSphere(1.6, 8, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(-3.6f, -1.4f, 7.8f); glutSolidSphere(1.1, 8, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.4f, -3.6f, 8.0f); glutSolidSphere(1.3, 8, 8); glPopMatrix();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* Sets the overall scene brightness (global ambient) for the current
   mode. Called once per frame from display(). The background colour
   itself now comes from drawSkyDome(), not from glClearColor. */
void setupSkyAndAmbient()
{
    if (isNightMode)
    {
        glClearColor(0.01f, 0.015f, 0.05f, 1.0f);         /* matches the sky dome's zenith, just in case of any gap */
        GLfloat globalAmbient[] = { 0.07f, 0.07f, 0.09f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    }
    else
    {
        glClearColor(0.25f, 0.55f, 0.88f, 1.0f);
        GLfloat globalAmbient[] = { 0.30f, 0.30f, 0.30f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    }
}

/* Simple hierarchical tree : trunk (wood texture, cylinder) -> 3 leaf
   spheres on top, so it reads as a tree from any camera angle. */
void drawTree(float x, float z)
{
    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        applyMaterialForObject("wood");
        glColor3f(0.55f, 0.55f, 0.55f);   /* tint - texture supplies the real colour */
        useTexture(texWood);
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluQuadricTexture(q, GL_TRUE);
            gluCylinder(q, 0.32f, 0.22f, 3.2f, 10, 1);
            gluDeleteQuadric(q);
        glPopMatrix();
        noTexture();

        applyMaterialForObject("field");
        glColor3f(0.12f, 0.45f, 0.14f);
        glPushMatrix();
            glTranslatef(0.0f, 3.6f, 0.0f);
            glutSolidSphere(1.35, 10, 10);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.6f, 4.25f, 0.35f);
            glutSolidSphere(0.95, 8, 8);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(-0.65f, 4.15f, -0.30f);
            glutSolidSphere(1.0, 8, 8);
        glPopMatrix();
    glPopMatrix();
}

/* Simple hierarchical street light : pole -> arm -> bulb. */
void drawStreetLamp(float x, float z)
{
    float poleH = 6.5f;

    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        applyMaterialForObject("metal");
        glColor3f(0.18f, 0.18f, 0.20f);
        useTexture(texMetal);
        glPushMatrix();
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GLUquadric* q = gluNewQuadric();
            gluQuadricTexture(q, GL_TRUE);
            gluCylinder(q, 0.12f, 0.09f, poleH, 8, 1);
            gluDeleteQuadric(q);
        glPopMatrix();
        noTexture();

        /* horizontal arm, hung from the top of the pole */
        glPushMatrix();
            glTranslatef(0.45f, poleH - 0.05f, 0.0f);
            glColor3f(0.18f, 0.18f, 0.20f);
            glScalef(0.9f, 0.10f, 0.10f);
            glutSolidCube(1.0f);
        glPopMatrix();

        /* bulb - drawn unlit so it clearly reads as a lamp.
           STEP 19 : bright yellow glow at night, dim "switched off" glass by day. */
        glDisable(GL_LIGHTING);
        if (isNightMode)
            glColor3f(1.0f, 0.95f, 0.60f);
        else
            glColor3f(0.55f, 0.55f, 0.50f);
        glPushMatrix();
            glTranslatef(0.85f, poleH - 0.18f, 0.0f);
            glutSolidSphere(0.22, 8, 8);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    glPopMatrix();
}

/* Alternate trees / street lights along the ring just outside the road. */
void drawTreesAndStreetLights()
{
    float spacing = 16.0f;
    int idx;

    idx = 0;
    for (float x = -TREE_RING_X; x <= TREE_RING_X; x += spacing, idx++)
    {
        if (idx % 2 == 0) drawTree(x,  TREE_RING_Z); else drawStreetLamp(x,  TREE_RING_Z);
        if (idx % 2 == 0) drawStreetLamp(x, -TREE_RING_Z); else drawTree(x, -TREE_RING_Z);
    }

    idx = 0;
    for (float z = -TREE_RING_Z + spacing; z < TREE_RING_Z; z += spacing, idx++)
    {
        if (idx % 2 == 0) drawStreetLamp(TREE_RING_X, z); else drawTree(TREE_RING_X, z);
        if (idx % 2 == 0) drawTree(-TREE_RING_X, z); else drawStreetLamp(-TREE_RING_X, z);
    }
}

/* A few small buildings further back than the tree ring. */
void drawSmallBuildings()
{
    float bx = ROAD_HALF_X + 40.0f;
    float bz = ROAD_HALF_Z + 40.0f;

    drawBrickBox( bx,  bz, 14.0f, 14.0f, 0.0f, 10.0f, 1.00f);
    drawBrickBox(-bx,  bz, 10.0f, 10.0f, 0.0f,  7.0f, 0.90f);
    drawBrickBox( bx, -bz, 11.0f, 13.0f, 0.0f,  8.0f, 0.95f);
    drawBrickBox(-bx, -bz, 16.0f, 11.0f, 0.0f, 12.5f, 1.05f);

    /* flat concrete roof caps so the buildings don't look like bare boxes */
    float roofCol[3] = { 0.55f, 0.55f, 0.55f };
    drawBoxTex( bx,  bz, 14.4f, 14.4f, 10.0f, 0.4f, texConcrete, roofCol, texConcrete, roofCol, 6.0f);
    drawBoxTex(-bx,  bz, 10.4f, 10.4f,  7.0f, 0.4f, texConcrete, roofCol, texConcrete, roofCol, 6.0f);
    drawBoxTex( bx, -bz, 11.4f, 13.4f,  8.0f, 0.4f, texConcrete, roofCol, texConcrete, roofCol, 6.0f);
    drawBoxTex(-bx, -bz, 16.4f, 11.4f, 12.5f, 0.4f, texConcrete, roofCol, texConcrete, roofCol, 6.0f);
}

/* Parking area : an asphalt pad, painted bay lines, and a handful of
   parked (stationary) cars, placed just outside the road on the south side. */
void drawParkingArea()
{
    float cx = 0.0f;
    float cz = -(ROAD_HALF_Z + 32.0f);
    float w  = 46.0f;
    float d  = 20.0f;

    applyMaterialForObject("concrete");
    useTexture(texRoad);
    glColor3f(0.80f, 0.80f, 0.80f);
    drawTexturedGrid(cx - w / 2.0f, cz - d / 2.0f, cx + w / 2.0f, cz + d / 2.0f, 0.015f, 8, 4, 8.0f);
    noTexture();

    /* painted parking-bay lines */
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    for (int i = -4; i <= 4; i++)
    {
        float lx = cx + i * (w / 9.0f);
        glBegin(GL_LINES);
            glVertex3f(lx, 0.02f, cz - d / 2.0f + 1.0f);
            glVertex3f(lx, 0.02f, cz + d / 2.0f - 1.0f);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    /* static parked cars, one per alternate bay */
    float carR[4] = { 0.75f, 0.15f, 0.85f, 0.10f };
    float carG[4] = { 0.10f, 0.35f, 0.85f, 0.55f };
    float carB[4] = { 0.10f, 0.75f, 0.15f, 0.85f };

    int c = 0;
    for (int i = -3; i <= 3; i += 2, c++)
    {
        float lx = cx + i * (w / 9.0f) + (w / 18.0f);
        drawCar(lx, 0.0f, cz - d / 4.0f, 0.0f, carR[c % 4], carG[c % 4], carB[c % 4]);
    }
}

/* Master function for this step - draws everything outside the stadium
   wall, in back-to-front visual order. */
void drawOutsideEnvironment()
{
    drawOutsideGround();
    drawRoadLoop();
    drawTreesAndStreetLights();
    drawSmallBuildings();
    drawParkingArea();
    drawMovingCars();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setupSkyAndAmbient();                      /* global ambient for day/night; fallback clear colour */

    glLoadIdentity();
    setupCamera();

    /* FIX 2 : gradient sky dome drawn first, behind everything else */
    drawSkyDome();

    setupFloodlights();
    setupFillLight();

    /* STEP 19 : moon + stars, drawn first so they sit behind everything else */
    drawMoon();
    drawStars();
    drawClouds();                              /* FIX 2 : drifting day-sky clouds */

    /* --- STEP 18 : outside world drawn first (it sits at/under y=0) --- */
    drawOutsideEnvironment();

    /* --- ground, pitch, markings, goals --- */
    drawGround();                              /* concrete texture */
    drawField();                               /* grass texture    */
    drawFieldMarkings();
    drawCornerFlags();

    applyMaterialForObject("metal");           /* goal post : metallic */
    drawGoal(-1);
    drawGoal(1);

    /* --- stadium structure (each group has its own material) --- */
    drawSeatingTiers();                        /* seat + concrete textures */

    applyMaterialForObject("metal");
    drawStadiumRoof();                         /* metal texture */

    applyMaterialForObject("brick");
    drawStadiumExterior();                     /* brick texture */

    drawPlayerTunnel();                        /* NEW : colourful blue/red player tunnel */

    applyMaterialForObject("metal");
    drawFloodlights();                         /* metal texture */

    drawVIPRoom();                             /* NEW : colourful VIP room with real open window */

    /* --- people, banners, scoreboard --- */
    applyMaterialForObject("plastic");
    drawCrowd();
    drawBanners();
    drawStadiumFlags();
    drawScoreboard();

    drawAllPlayers();
    drawFootball(ballX, ballY, ballZ);         /* football texture + emissive glow */

    if (celebrating)
        drawGoalMessage();

    if (restartPending)
        drawRestartMessage();

    drawFloodlightStatus();
    drawControlsHelp();
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 1.0, 900.0);
    glMatrixMode(GL_MODELVIEW);
}

void specialKeys(int key, int x, int y)
{
    const float TURN_SPEED = 6.0f;

    bool frozen = (celebrating || restartPending);
    float &f = playerFacing[controlledTeam][controlledIndex];

    switch (key)
    {
        case GLUT_KEY_LEFT:
            if (!frozen) f += TURN_SPEED;
            break;

        case GLUT_KEY_RIGHT:
            if (!frozen) f -= TURN_SPEED;
            break;

        case GLUT_KEY_UP:
            if (!frozen) f += 90.0f;
            break;

        case GLUT_KEY_DOWN:
            if (!frozen) f -= 90.0f;
            break;

        case GLUT_KEY_PAGE_UP:
            camDist -= 5.0f;
            if (camDist < 8.0f) camDist = 8.0f;
            break;

        case GLUT_KEY_PAGE_DOWN:
            camDist += 5.0f;
            if (camDist > 400.0f) camDist = 400.0f;
            break;
    }

    while (f >= 360.0f) f -= 360.0f;
    while (f < 0.0f)    f += 360.0f;

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{
    /* F = Floodlights ON/OFF */
    if (key == 'f' || key == 'F')
    {
        floodlightsOn = !floodlightsOn;

        if (!floodlightsOn)
        {
            for (int i = 0; i < 4; i++)
                glDisable(GL_LIGHT0 + i);
        }
        else
        {
            for (int i = 0; i < 4; i++)
                glEnable(GL_LIGHT0 + i);
        }

        glutPostRedisplay();
        return;
    }

    /* STEP 19 : N = Night mode, M = Day mode ('D' is taken by player movement) */
    if (key == 'n' || key == 'N')
    {
        isNightMode  = true;
        floodlightsOn = true;              /* night -> floodlights ON               */
        for (int i = 0; i < 4; i++) glEnable(GL_LIGHT0 + i);
        glutPostRedisplay();
        return;
    }
    if (key == 'm' || key == 'M')
    {
        isNightMode  = false;
        floodlightsOn = false;             /* day -> floodlights reduced/off        */
        for (int i = 0; i < 4; i++) glDisable(GL_LIGHT0 + i);
        glutPostRedisplay();
        return;
    }
    if (key == 'h' || key == 'H')
    {
        showHelp = !showHelp;
        glutPostRedisplay();
        return;
    }

    switch (key)
    {
        case 27: exit(0); break;

        case 'w': case 'W': keyW = true; break;
        case 'a': case 'A': keyA = true; break;
        case 's': case 'S': keyS = true; break;
        case 'd': case 'D': keyD = true; break;

        case 'v': case 'V':
            controlledIndex = (controlledIndex + 1) % 11;
            break;

        case ' ':
            tryPass();
            break;

        /* STEP 11 : camera modes */
        case '1': cameraMode = 0; break;
        case '2': cameraMode = 1; camDist = 18.0f; break;
        case '3': cameraMode = 2; camDist = 35.0f; camAngle = 180.0f; camPitch = 15.0f; break;
        case '4': cameraMode = 3; break;
        case '5': cameraMode = 4; break;   /* NEW : VIP Room View - watch the match through the room's window */

        /* Keyboard camera controls remain as backup */
        case 'i': case 'I': camPitch += 2.0f; if (camPitch > 89.0f) camPitch = 89.0f; break;
        case 'k': case 'K': camPitch -= 2.0f; if (camPitch < -10.0f) camPitch = -10.0f; break;
        case 'j': case 'J': camAngle -= 3.0f; break;
        case 'l': case 'L': camAngle += 3.0f; break;
    }
    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 'w': case 'W': keyW = false; break;
        case 'a': case 'A': keyA = false; break;
        case 's': case 'S': keyS = false; break;
        case 'd': case 'D': keyD = false; break;
    }
}

/* LEFT MOUSE CLICK -> shoot;  RIGHT MOUSE DRAG -> camera (STEP 11) */
void mouseHandler(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
        tryShoot();

    if (button == GLUT_RIGHT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            rightMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        }
        else if (state == GLUT_UP)
        {
            rightMouseDown = false;
        }
    }

    /* Mouse wheel = zoom */
    if (state == GLUT_DOWN && (button == 3 || button == 4))
    {
        if (button == 3) camDist -= 5.0f;
        if (button == 4) camDist += 5.0f;
        if (camDist < 8.0f) camDist = 8.0f;
        if (camDist > 400.0f) camDist = 400.0f;
    }

    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    if (!rightMouseDown || cameraMode == 3 || cameraMode == 4) return;

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    camAngle += dx * 0.5f;
    camPitch -= dy * 0.5f;

    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -10.0f) camPitch = -10.0f;

    if (camAngle > 360.0f) camAngle -= 360.0f;
    if (camAngle < -360.0f) camAngle += 360.0f;

    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

/* ----------------------------------------------------------------------
   ANIMATION + MOVEMENT + BALL TIMER  (~33 times per second)
   ---------------------------------------------------------------------- */
void timer(int value)
{
    animTime += 0.10f;
    if (animTime > 100000.0f) animTime = 0.0f;

    controlledIsMoving = updateControlledPlayer();
    playerIsMoving[controlledTeam][controlledIndex] = controlledIsMoving;

    if (!celebrating && !restartPending)
    {
        updateGoalkeeperAI(0);
        updateGoalkeeperAI(1);
        updateOutfieldAI(0);
        updateOutfieldAI(1);
    }

    for (int t = 0; t < 2; t++)
    {
        for (int i = 0; i < 11; i++)
        {
            if (playerIsMoving[t][i])
                playerPhase[t][i] += 0.45f;
            else
                playerPhase[t][i] += 0.05f;
        }
    }

    if (kickTicksLeft > 0)
    {
        kickTicksLeft--;
        if (kickTicksLeft == 0) { kickingTeam = -1; kickingIndex = -1; }
    }

    /* OUT OF PLAY : short pause, then play resumes from the restart spot. */
    if (restartPending)
    {
        restartTicksLeft--;
        if (restartTicksLeft <= 0)
        {
            restartPending = false;
            restartType = RESTART_NONE;
            restartMessage[0] = '\0';
            restartTeam = -1;
        }
    }

    if (celebrating)
    {
        celebrationTicksLeft--;
        if (celebrationTicksLeft <= 0)
        {
            celebrating     = false;
            celebrationTeam = -1;
            resetAfterGoal();
        }
    }

    updateBall();

    /* STEP 18 : advance the moving cars along the road loop */
    updateCars();

    /* FIX 2 : slowly drift the clouds sideways across the sky */
    cloudDriftX += 0.015f;

    glutPostRedisplay();
    glutTimerFunc(30, timer, 0);
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    GLfloat globalAmbient[] = { 0.12f, 0.12f, 0.12f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

    glPointSize(5.0f);
    initPlayerPositions();

    initTextures();               /* build all textures once */
    initCars();                   /* STEP 18 : place the moving cars on the road loop */
    initStars();                  /* STEP 19 : scatter the night-sky star field */
    initClouds();                 /* FIX 2   : scatter the day-sky clouds */
    buildSpectatorDisplayList();  /* STEP 20 : compile the crowd shape once */
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("3D Football Stadium ");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseHandler);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(30, timer, 0);

    glutMainLoop();
    return 0;
}
