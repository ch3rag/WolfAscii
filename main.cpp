#include <iostream>
#include <Windows.h>
#include <Math.h>
#include <chrono>
#include <vector>
#include <algorithm>

#define PI 3.14159
int cols = 120;
int rows = 40;
float playerX = 8.0f;
float playerY = 8.0f;
float playerAngle = PI / 2;
float playerFOV = PI / 4.0;
float playSpeed = 5.0f;
int mapWidth = 16;
int mapHeight = 16;
float maxRayCastDistance = 16.0f;
int main(int argc, char ** argv) {

    // CREATE A SCREEN BUFFER
    wchar_t * screen = new wchar_t[cols * rows];
    HANDLE consoleHandle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(consoleHandle);
    DWORD charsWritten;

    std::wstring map;

    map += L"################";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..........##..#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#....###########";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    // CREATE TWO TIME POINTS TO CALCULATE TIME ELAPASED IN A SINGLE ITERATION OF GAME LOOP
    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();

    while(1) {

        tp2 = std::chrono::system_clock::now();
        std::chrono::duration <float> elapsed = tp2 - tp1;
        tp1 = tp2;
        float elapsedTime = elapsed.count();

        // Virtual Key Codes
        // A - 0x41
        // S - 0x53
        // D - 0x44
        // w - 0x57

        // ALSO SCALE THE MOVEMENT AND ROTATION ACCORDING TO TIME ELAPSED
        if(GetAsyncKeyState(0x41) & 0x8000) {
            playerAngle -= (playSpeed * 0.75f) * elapsedTime;
        }

        if(GetAsyncKeyState(0x44) & 0x8000) {
            playerAngle += (playSpeed * 0.75f) * elapsedTime;
        }

        if(GetAsyncKeyState(0x57) & 0x8000) {
            playerX += cosf(playerAngle) * 5.0f * elapsedTime;
            playerY += sinf(playerAngle) * 5.0f * elapsedTime;

            // COLLISION DETECTION

            if(map[(int)playerY * mapWidth + (int)playerX] == '#') {
                playerX -= cosf(playerAngle) * 5.0f * elapsedTime;
                playerY -= sinf(playerAngle) * 5.0f * elapsedTime;
            }
        }

        if(GetAsyncKeyState(0x53) & 0x8000) {
            playerX -= cosf(playerAngle) * 5.0f * elapsedTime;
            playerY -= sinf(playerAngle) * 5.0f * elapsedTime;

            // COLLISION DETECTION

            if(map[(int)playerY * mapWidth + (int)playerX] == '#') {
                playerX += cosf(playerAngle) * 5.0f * elapsedTime;
                playerY += sinf(playerAngle) * 5.0f * elapsedTime;
            }
        }

        for (int x = 0 ; x < cols ; x++) {
            
            // COMPUTE ANGLE FOR RAYCASTNG
            // WE WILL CAST 120(NUMBER OF COLS) RAYS INSIDE OUR PLAYER'S FIELD OF VIEW
            // EACH RAY WILL COMPUTE HOW MUCH DISTANCE IT TRAVELS BEFORE HITTING THE WALL

            float rayAngle = (playerAngle - (playerFOV / 2.0f)) + ((float)(x) / (float)cols) * playerFOV;
            float distanceToWall = 0.0f; 
            bool isHitWall = false;
            bool isHitBoundary = false;

            // COMPUTE UNIT VECTOR FROM  RAYANGLE
            float rayX = cosf(rayAngle);
            float rayY = sinf(rayAngle);

            while(!isHitWall && distanceToWall < maxRayCastDistance) {

                distanceToWall += 0.1f;
                // CALCULATE (X,Y) OF RAY'S CURRENT POSITION ACCORING TO GROWTH OF DISTANCE AND IN THE DIRECTION OF UNIT VECTOR 
                // TRANSLATE THE COMPUTED (X, Y) TO PLAYER'S CURRENT POSITION
                int dX = ((float)playerX + rayX * distanceToWall);
                int dY = ((float)playerY + rayY * distanceToWall); 

                // RAYS OUT OF BOUNDS
                if(dX < 0 || dX >= mapWidth || dY < 0 || dY >= mapHeight) {
                    isHitWall = true;
                    distanceToWall = maxRayCastDistance;
                    
                } else if(map[(int)(dY * mapWidth + dX)] == '#') { //HIT A WALL 
                    isHitWall = true;

                    // TEST IF THE RAY BEING CASTED IS NEAR THE CORNER OF SINGLE WALL BLOCK
                    // FOLLOWING VECTOR STORES <MAGNITUDE, DOT> PAIR
                    std::vector <std::pair<float, float>> pair;


                    for(int ty = 0 ; ty < 2 ; ty++) {
                        for(int tx = 0 ; tx < 2 ; tx++) {
                            float vx = (float)dX + tx - playerX;
                            float vy = (float)dY + ty - playerY;
                            float mag = sqrt(vx * vx + vy * vy);
                            float dot = (rayX * vx / mag) + (rayY * vy / mag);
                            pair.push_back(std::make_pair(mag, dot));
                        } 
                    }

                    // SORT THE PAIRS ORDERED BY MAGNITUDE IN ASCENDING ORDER
                    // HENCE SELECTING TWO SHORTEST RAYS WHOSE ANGLE IS LEAST W.R.T PERFECT CORNERS
                    // BOUND CONSTRAIN CONTROLS HOW TIGHT THE ANGLE SHOULD BE TO BE CONSIDERED AS A CORNER
                    std::sort(pair.begin(), pair.end(), [](const std::pair <float, float> &left, const std::pair <float, float> &right) { return left.first < right.first; }); 
                    float boundConstrain = 0.01f;

                    // ONLY TWO CORNERS CAN BE SEEN FOR NOW
                    if(acos(pair.at(0).second) < boundConstrain) isHitBoundary = true;
                    if(acos(pair.at(1).second) < boundConstrain) isHitBoundary = true;
                }
            }
            
            // AS DISTANCE TO WALL TENDS TO INCRESE CEIL HEIGHT ALSO TENDS TO INCREASE 
            int ceilHeight = (float)(rows / 2.0) - rows / ((float) distanceToWall);
            int floorHeight =  rows - ceilHeight;
            
            wchar_t shade = ' ';

            if(distanceToWall < maxRayCastDistance / 4.0f) {
                shade = 0x2588;
            } else if(distanceToWall < maxRayCastDistance / 3.0f) {
                shade = 0x2593;
            } else if(distanceToWall < maxRayCastDistance / 2.0f) {
                shade = 0x2592;
            } else if(distanceToWall < maxRayCastDistance) {
                shade = 0x2591;
            } else {
                shade = ' ';
            }
            
            if (isHitBoundary) shade = ' ';

            // UPDATE CONSOLE BUFFER ACCORDING TO COMPUTATION
            for(int y = 0 ; y < rows ; y++) {
                if(y <= ceilHeight) {
                    screen[y * cols + x] = ' ';
                } else if(y > ceilHeight && y <= floorHeight) {
                    screen[y * cols + x] = shade;
                } else {
                    float proportion = 1.0f - (y - (float)rows / 2.0f) / ((float) rows / 2.0f);

                    if (proportion < 0.25)          shade = '#';
                    else if (proportion < 0.5)      shade = 'X';
                    else if (proportion < 0.75)     shade = '.';
                    else if (proportion < 0.9)      shade = '-';
                    else                            shade = ' ';
                    screen[y * cols + x] = shade;
                }
            }
        }   

        screen[rows * cols - 1] = '\0';
        WriteConsoleOutputCharacterW(consoleHandle, screen, rows * cols, {0, 0}, &charsWritten);
    }

    return 0;
}

