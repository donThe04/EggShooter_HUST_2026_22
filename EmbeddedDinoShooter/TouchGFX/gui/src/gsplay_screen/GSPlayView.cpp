#include <gui/gsplay_screen/GSPlayView.hpp>
#include <images/BitmapDatabase.hpp>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include <math.h>

extern uint32_t gSeed;

extern TIM_HandleTypeDef htim3;

#define NOTE_DO   262
#define NOTE_RE   294
#define NOTE_MI   330
#define NOTE_FA   349
#define NOTE_SOL  392
#define NOTE_LA   440
#define NOTE_SI   494

extern "C" void Joystick_Read(uint8_t *outX, uint8_t *outY);
extern "C" uint8_t Button_IsPressed(void);

// Tốc độ bay của viên bóng (pixel/tick)
static const float BULLET_SPEED = 4.0f;

// Số quả tối thiểu trong 1 nhóm cùng màu để nổ
static const int MIN_GROUP_TO_POP = 3;

GSPlayView::GSPlayView()
{
    yOffset = -100;
    isDropping = true;
    tickCounter = 0;

    isBulletFlying = false;
    bulletX = 0.0f;
    bulletY = 0.0f;
    bulletVX = 0.0f;
    bulletVY = 0.0f;
    bulletColor = 0;
    wasButtonPressed = false;

    currentEggColor = 0;
    nextEggColor = 0;

    scrollTickCounter = 0;
    isGameOver = false;
    currentScore = 0;
}

void GSPlayView::pauseClicked()
{
    isPaused = true;
    pauseContainer.setVisible(true);
    pauseContainer.invalidate();
}

void GSPlayView::continueClicked()
{
    isPaused = false;

    pauseContainer.setVisible(false);
    pauseContainer.invalidate();
}

void GSPlayView::ResetGame()
{
    isPaused = false;
    isGameOver = false;
    currentScore = 0;

    yOffset = -100;
    isDropping = true;
    tickCounter = 0;
    scrollTickCounter = 0;

    isBulletFlying = false;

    bulletX = 0.0f;
    bulletY = 0.0f;
    bulletVX = 0.0f;
    bulletVY = 0.0f;

    bullet.setVisible(false);
    bullet.invalidate();

    wasButtonPressed = false;

    pauseContainer.setVisible(false);
    pauseContainer.invalidate();

    gameoverContainer.setVisible(false);
    gameoverContainer.invalidate();

    // =========================
    // Khởi tạo lại lưới
    // =========================
    InitializeGrid();

    // =========================
    // Reset bóng hiện tại
    // =========================
    currentEggColor = (Random() >> 24) % 4;
    SetEggBitmap(currentEgg, currentEggColor);

    // Reset bóng kế tiếp
    RollNextEgg();

    // =========================
    // Render lại
    // =========================
    RenderGrid();
    UpdateAim();
    UpdateScoreUI();
}

void GSPlayView::restartClicked()
{
    ResetGame();
}

void GSPlayView::exitClicked()
{
    application().gotoGSMenuScreenNoTransition();
}

void GSPlayView::restartgameoverClicked()
{
    ResetGame();
}

void GSPlayView::exitgameoverClicked()
{
    application().gotoGSMenuScreenNoTransition();
}

//Âm thanh
void GSPlayView::playTone(uint32_t freq, uint32_t durationMs)
{
    uint32_t period = 1000000 / freq;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period - 1);

    __HAL_TIM_SET_COMPARE(
        &htim3,
        TIM_CHANNEL_2,
        period / 2
    );

    HAL_TIM_PWM_Start(
        &htim3,
        TIM_CHANNEL_2
    );

    HAL_Delay(durationMs);

    HAL_TIM_PWM_Stop(
        &htim3,
        TIM_CHANNEL_2
    );
}

void GSPlayView::playGameOverSound()
{
    uint32_t melody[] =
    {
        NOTE_LA,
        NOTE_SOL,
        NOTE_FA,
        NOTE_MI,
        NOTE_RE
    };

    uint32_t duration[] =
    {
        200,
        200,
        200,
        200,
        500
    };

    for(int i = 0; i < 5; i++)
    {
        playTone(melody[i], duration[i]);
        HAL_Delay(50);
    }
}

uint32_t GSPlayView::Random()
{
    gSeed = gSeed * 1103515245 + 12345;
    return gSeed;
}

void GSPlayView::SetEggBitmap(touchgfx::Image& img, uint8_t color)
{
    switch(color)
    {
    case 0:
        img.setBitmap(touchgfx::Bitmap(BITMAP_GREEN_ID));
        break;
    case 1:
        img.setBitmap(touchgfx::Bitmap(BITMAP_PINK_ID));
        break;
    case 2:
        img.setBitmap(touchgfx::Bitmap(BITMAP_YELLOW_ID));
        break;
    default:
        img.setBitmap(touchgfx::Bitmap(BITMAP_BUBBLEPOINT_ID));
        break;
    }
    img.invalidate();
}

void GSPlayView::RollNextEgg()
{
    Random();
    nextEggColor = (Random() >> 24) % 4;
    SetEggBitmap(nextEgg, nextEggColor);
}

void GSPlayView::setupScreen()
{
    GSPlayViewBase::setupScreen();

    InitializeGrid();

    int index = 0;

    for(int r = 0; r < ROWS; r++)
    {
        for(int c = 0; c < COLS; c++)
        {
            containerGrid.add(eggs[index]);
            index++;
        }
    }
    for(int i = 0; i < 8; i++)
    {
        aimDots[i].setBitmap(
            touchgfx::Bitmap(BITMAP_DOTS_ID)
        );

        add(aimDots[i]);
    }
    UpdateAim();

    RenderGrid();

    // ----- KHỞI TẠO VIÊN BÓNG BẮN (đang bay) -----
    bullet.setBitmap(touchgfx::Bitmap(BITMAP_GREEN_ID));
    bullet.setVisible(false);
    add(bullet);

    // ----- KHỞI TẠO MÀU QUẢ HIỆN TẠI VÀ KẾ TIẾP -----
    Random();
    currentEggColor = (Random() >> 24) % 4;
    SetEggBitmap(currentEgg, currentEggColor);

    RollNextEgg();

    isPaused = false;

    pauseContainer.setVisible(false);
    pauseContainer.invalidate();

    currentScore = 0;
    UpdateScoreUI();
}

void GSPlayView::RenderGrid()
{
    int index = 0;

    for(int r = 0; r < ROWS; r++)
    {
        for(int c = 0; c < COLS; c++)
        {
            // Ô trống (đã nổ) -> ẩn hẳn, không vẽ
            if(grid[r][c] == EMPTY_CELL)
            {
                eggs[index].setVisible(false);
                eggs[index].invalidate();
                index++;
                continue;
            }

            switch(grid[r][c])
            {
            case 0:
                eggs[index].setBitmap(touchgfx::Bitmap(BITMAP_GREEN_ID));
                break;

            case 1:
                eggs[index].setBitmap(touchgfx::Bitmap(BITMAP_PINK_ID));
                break;

            case 2:
                eggs[index].setBitmap(touchgfx::Bitmap(BITMAP_YELLOW_ID));
                break;

            default:
                eggs[index].setBitmap(touchgfx::Bitmap(BITMAP_BUBBLEPOINT_ID));
                break;
            }

            bool odd = ((r + (oddOffset ? 1 : 0)) % 2);

            int x = c * CELL_W + (odd ? 16 : 0);
            int y = r * CELL_H + yOffset;

            eggs[index].setXY(x, y);

            if(y > -32 && y < 200)
            {
                eggs[index].setVisible(true);
            }
            else
            {
                eggs[index].setVisible(false);
            }

            eggs[index].invalidate();

            index++;
        }
    }

    containerGrid.invalidate();
}

void GSPlayView::handleTickEvent()
{
    // ----- DỪNG TOÀN BỘ GAME NẾU ĐÃ THUA (trứng chạm đáy) -----
    if(isGameOver)
        return;

    if(isPaused)
        return;
    // ----- ĐỌC JOYSTICK -----
    uint8_t joyX, joyY;
    Joystick_Read(&joyX, &joyY);

    float normalized = joyX / 255.0f;
    gunAngle = 150.0f - normalized * 120.0f;

    if(gunAngle < 30) gunAngle = 30;
    if(gunAngle > 150) gunAngle = 150;

    UpdateAim();

    // ----- KIỂM TRA NÚT BẮN -----
    uint8_t isPressed = Button_IsPressed();

    if(isPressed && !wasButtonPressed)
    {
        FireBullet();
        //Âm thanh bắn
        playTone(700,50);
    }
    wasButtonPressed = isPressed;

    // ----- CẬP NHẬT VIÊN BÓNG ĐANG BAY -----
    UpdateBullet();

    // ----- HIỆU ỨNG TRƯỢT LƯỚI VÀO MÀN HÌNH LÚC BẮT ĐẦU GAME -----
    if(isDropping)
    {
        tickCounter++;

        if(tickCounter >= 7)
        {
            tickCounter = 0;

            yOffset += 2;

            if(yOffset >= FINAL_OFFSET)
            {
                yOffset = FINAL_OFFSET;
                isDropping = false;
            }

            RenderGrid();
        }

        return; // trong lúc đang trượt vào, chưa spawn hàng mới
    }

    // ----- ĐÃ TRƯỢT VÀO XONG: TIẾP TỤC CUỘN XUỐNG LIÊN TỤC, MƯỢT TỪNG PIXEL -----
    scrollTickCounter++;

    if(scrollTickCounter >= SCROLL_TICK_INTERVAL)
    {
        scrollTickCounter = 0;

        yOffset += 1; // cuộn thêm 1px mỗi lần -> mắt thấy trứng trôi đều, không giật cục

        if(yOffset >= CELL_H)
        {
            // Đã cuộn đủ 1 hàng -> đẩy dữ liệu lưới xuống 1 hàng, sinh hàng mới trên đỉnh,
            // rồi lùi yOffset lại đúng phần dư để việc cuộn tiếp tục liền mạch, không bị giật.
            yOffset -= CELL_H;
            AddNewRow();
        }

        RenderGrid();
    }
}

void GSPlayView::UpdateAim()
{
    float rad = gunAngle * 3.1415926f / 180.0f;

    int startX = currentEgg.getX() + 0.8*currentEgg.getWidth() / 2;
    int startY = currentEgg.getY() + 0.8*currentEgg.getHeight() / 2;

    for(int i = 0; i < 8; i++)
    {
        int distance = (i + 1) * 20;

        int x = startX + cosf(rad) * distance;
        int y = startY - sinf(rad) * distance;

        aimDots[i].setXY(x, y);
        aimDots[i].invalidate();
    }
}

void GSPlayView::FireBullet()
{
    if(isBulletFlying)
        return;

    if(isGameOver)
        return;

    float rad = gunAngle * 3.1415926f / 180.0f;

    bulletX = 104.0f;
    bulletY = 250.0f;

    bulletVX = cosf(rad) * BULLET_SPEED;
    bulletVY = -sinf(rad) * BULLET_SPEED;

    // Lưu màu của viên đạn NGAY LÚC NÀY (trước khi currentEggColor bị đổi sang quả mới)
    bulletColor = currentEggColor;

    SetEggBitmap(bullet, bulletColor);

    bullet.setXY((int)bulletX, (int)bulletY);
    bullet.setVisible(true);
    bullet.invalidate();

    isBulletFlying = true;

    currentEggColor = nextEggColor;
    SetEggBitmap(currentEgg, currentEggColor);

    RollNextEgg();
}

void GSPlayView::UpdateBullet()
{
    if(!isBulletFlying)
        return;

    float oldX = bulletX;
    float oldY = bulletY;

    bulletX += bulletVX;
    bulletY += bulletVY;

    int hitRow, hitCol;

    if(CheckBulletCollision(hitRow, hitCol))
    {
    	//Âm thanh va chạm
    	playTone(400,100);
    	int tr = -1, tc = -1;

        // Bước 1: thử tìm ô trống trong 6 ô lân cận trực tiếp
        if(!FindSnapCell(hitRow, hitCol, tr, tc))
        {
            // Bước 2 (an toàn): không tìm thấy lân cận trực tiếp trống
            // -> tìm ô trống GẦN NHẤT trên toàn lưới (tuyệt đối không ghi đè
            //    lên quả đang có, tránh làm mất dữ liệu / thay màu quả cũ)
            float bestDist = 1e9f;
            bool foundAny = false;

            for(int r = 0; r < ROWS; r++)
            {
                for(int c = 0; c < COLS; c++)
                {
                    if(grid[r][c] != EMPTY_CELL)
                        continue;

                    float cx, cy;
                    CellToPixelCenter(r, c, cx, cy);

                    float dx = (bulletX + 16.0f) - cx;
                    float dy = (bulletY + 16.0f) - cy;
                    float distSq = dx * dx + dy * dy;

                    if(distSq < bestDist)
                    {
                        bestDist = distSq;
                        tr = r;
                        tc = c;
                        foundAny = true;
                    }
                }
            }

            // Trường hợp cực hiếm: lưới đã đặc kín 100% (không còn ô trống nào)
            // -> lúc này mới đành chấp nhận hủy viên đạn (không ghi đè quả cũ).
            if(!foundAny)
            {
                isBulletFlying = false;
                bullet.setVisible(false);
                bullet.invalidate();
                return;
            }
        }

        // 1. commit vào grid
        grid[tr][tc] = bulletColor;

        // 2. snap về đúng tâm ô (CHUẨN HOÁ POSITION)
        float cx, cy;
        CellToPixelCenter(tr, tc, cx, cy);

        bulletX = cx - 16.0f;
        bulletY = cy - 16.0f;

        bullet.setXY((int)bulletX, (int)bulletY);
        bullet.invalidate();

        // 3. tắt bullet
        isBulletFlying = false;
        bullet.setVisible(false);

        // 4. Kiểm tra nhóm cùng màu để nổ
        int groupRows[ROWS * COLS];
        int groupCols[ROWS * COLS];
        int groupCount = FindConnectedGroup(tr, tc, bulletColor,
                                            groupRows, groupCols);

        if(groupCount >= MIN_GROUP_TO_POP)
        {
            RemoveGroup(groupRows, groupCols, groupCount);
            playTone(900,80);
            currentScore += groupCount * 10;
            UpdateScoreUI();
        }

        // 5. Chỉ render đúng 1 lần
        RenderGrid();

        // 6. Kiểm tra thua: nếu quả vừa dính nằm ở hàng đáy cùng -> DỪNG GAME
        if(CheckGameOver())
        {
            isGameOver = true;
            presenter->saveHighScore(currentScore);
            UpdateScoreUI();
            gameoverContainer.setVisible(true);
            gameoverContainer.invalidate();
        }

        return;
    }


    // Va chạm trái/phải màn hình -> nảy lại (đơn giản hoá: đảo vận tốc X)
    if(bulletX < 0.0f)
    {
        bulletX = 0.0f;
        bulletVX = -bulletVX;
    }
    else if(bulletX > 240.0f - 32.0f)
    {
        bulletX = 240.0f - 32.0f;
        bulletVX = -bulletVX;
    }

    // Kiểm tra va chạm với lưới hoặc "trần" trên cùng
//    if(CheckBulletCollision())
//    {
//        // Đã xử lý xong (dính lưới hoặc nổ nhóm), dừng viên đạn
//        isBulletFlying = false;
////        bullet.setVisible(false);
////        bullet.invalidate();
//        return;
//    }

    // Phòng hờ: nếu vì lý do gì đó viên đạn vượt qua cả vùng trần mà chưa được
    // CheckBulletCollision() bắt được (ví dụ yOffset thay đổi giữa lúc bắn),
    // ép buộc gắn nó vào hàng 0 ngay tại đây, không để biến mất vô ích.
    if(bulletY < (float)yOffset - (float)CELL_H)
    {
        int forceCol;
        float colOffset = 0.0f; // hàng 0 luôn offset = 0
        forceCol = (int)roundf((bulletX - colOffset) / (float)CELL_W);
        if(forceCol < 0) forceCol = 0;
        if(forceCol >= COLS) forceCol = COLS - 1;

        // Tìm hàng gần trần nhất còn trống tại cột này
        int forceRow = 0;
        while(forceRow < ROWS && grid[forceRow][forceCol] != EMPTY_CELL)
        {
            forceRow++;
        }

        if(forceRow < ROWS)
        {
            grid[forceRow][forceCol] = bulletColor;
            RenderGrid();

            int groupRows[ROWS * COLS];
            int groupCols[ROWS * COLS];
            int groupCount = FindConnectedGroup(forceRow, forceCol, grid[forceRow][forceCol], groupRows, groupCols);

            if(groupCount >= MIN_GROUP_TO_POP)
            {
                RemoveGroup(groupRows, groupCols, groupCount);
                currentScore += groupCount * 10;
                UpdateScoreUI();
                RenderGrid();
            }
        }

        isBulletFlying = false;
        bullet.setVisible(false);
        bullet.invalidate();
        return;
    }

    bullet.setXY((int)bulletX, (int)bulletY);
    bullet.invalidate();
}

// ========================= LOGIC VA CHẠM + NỔ BÓNG =========================

void GSPlayView::PixelToCell(float x, float y, int &outRow, int &outCol)
{
    // Hàng được tính trước (vì offset cột phụ thuộc vào hàng chẵn/lẻ)
    int row = (int)roundf((y - yOffset) / (float)CELL_H);

    if(row < 0) row = 0;
    if(row >= ROWS) row = ROWS - 1;

    bool odd = ((row + (oddOffset ? 1 : 0)) % 2);

    float colOffset = odd ? 16.0f : 0.0f;
    int col = (int)roundf((x - colOffset) / (float)CELL_W);

    if(col < 0) col = 0;
    if(col >= COLS) col = COLS - 1;

    outRow = row;
    outCol = col;
}

void GSPlayView::CellToPixelCenter(int row, int col, float &outX, float &outY)
{
	bool odd = ((row + (oddOffset ? 1 : 0)) % 2);

	float colOffset = odd ? 16.0f : 0.0f;

    outX = containerGrid.getX()
         + col * CELL_W
         + colOffset
         + CELL_W / 2.0f;

    outY = containerGrid.getY()
         + row * CELL_H
         + yOffset
         + CELL_H / 2.0f;
}


bool GSPlayView::FindSnapCell(int hitRow, int hitCol,
                              int &targetRow, int &targetCol)
{
    int neighborRows[6], neighborCols[6];
    int nCount = GetNeighbors(hitRow, hitCol, neighborRows, neighborCols);

    float bestDist = 1e9f;
    bool found = false;

    for(int i = 0; i < nCount; i++)
    {
        int nr = neighborRows[i];
        int nc = neighborCols[i];

        // chỉ lấy ô trống
        if(grid[nr][nc] != EMPTY_CELL)
            continue;

        float cx, cy;
        CellToPixelCenter(nr, nc, cx, cy);

        float dx = (bulletX + 16.0f) - cx;
        float dy = (bulletY + 16.0f) - cy;

        float distSq = dx * dx + dy * dy;

        if(distSq < bestDist)
        {
            bestDist = distSq;
            targetRow = nr;
            targetCol = nc;
            found = true;
        }
    }

    return found;
}

int GSPlayView::GetNeighbors(int row, int col,
                             int outRows[6], int outCols[6])
{
    int count = 0;

    // Trái, phải
    static const int sameRowOffsets[2][2] =
    {
        {0, -1},
        {0,  1}
    };

    // Xác định hàng này đang lệch hay không
    bool isOddRow = ((row + (oddOffset ? 1 : 0)) % 2) != 0;

    int diagOffsets[2];

    if(!isOddRow)
    {
        // Hàng chẵn
        diagOffsets[0] = -1;
        diagOffsets[1] = 0;
    }
    else
    {
        // Hàng lẻ
        diagOffsets[0] = 0;
        diagOffsets[1] = 1;
    }

    // Trái, phải
    for(int i = 0; i < 2; i++)
    {
        int nr = row + sameRowOffsets[i][0];
        int nc = col + sameRowOffsets[i][1];

        if(nr >= 0 && nr < ROWS &&
           nc >= 0 && nc < COLS)
        {
            outRows[count] = nr;
            outCols[count] = nc;
            count++;
        }
    }

    // Hai ô phía trên
    for(int i = 0; i < 2; i++)
    {
        int nr = row - 1;
        int nc = col + diagOffsets[i];

        if(nr >= 0 && nr < ROWS &&
           nc >= 0 && nc < COLS)
        {
            outRows[count] = nr;
            outCols[count] = nc;
            count++;
        }
    }

    // Hai ô phía dưới
    for(int i = 0; i < 2; i++)
    {
        int nr = row + 1;
        int nc = col + diagOffsets[i];

        if(nr >= 0 && nr < ROWS &&
           nc >= 0 && nc < COLS)
        {
            outRows[count] = nr;
            outCols[count] = nc;
            count++;
        }
    }

    return count;
}
int GSPlayView::FindConnectedGroup(int row, int col, uint8_t color, int outRows[ROWS * COLS], int outCols[ROWS * COLS])
{
    bool visited[ROWS][COLS];
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS; c++)
            visited[r][c] = false;

    // Stack thủ công cho DFS (tránh đệ quy, an toàn cho stack MCU nhỏ)
    int stackRows[ROWS * COLS];
    int stackCols[ROWS * COLS];
    int stackTop = 0;

    stackRows[stackTop] = row;
    stackCols[stackTop] = col;
    stackTop++;
    visited[row][col] = true;

    int count = 0;

    while(stackTop > 0)
    {
        stackTop--;
        int curR = stackRows[stackTop];
        int curC = stackCols[stackTop];

        outRows[count] = curR;
        outCols[count] = curC;
        count++;

        int neighborRows[6], neighborCols[6];
        int nCount = GetNeighbors(curR, curC, neighborRows, neighborCols);

        for(int i = 0; i < nCount; i++)
        {
            int nr = neighborRows[i];
            int nc = neighborCols[i];

            if(visited[nr][nc])
                continue;

            if(grid[nr][nc] == color)
            {
                visited[nr][nc] = true;
                stackRows[stackTop] = nr;
                stackCols[stackTop] = nc;
                stackTop++;
            }
        }
    }

    return count;
}

void GSPlayView::RemoveGroup(int rows[], int cols[], int count)
{
    for(int i = 0; i < count; i++)
    {
        int r = rows[i];
        int c = cols[i];
        grid[r][c] = EMPTY_CELL;
    }
}

bool GSPlayView::CheckBulletCollision(int &hitRow, int &hitCol)
{
    float bulletCX = bulletX + 16.0f;
    float bulletCY = bulletY + 16.0f;

    // Bán kính va chạm ~ đúng kích thước thật của quả trứng (đường kính ~32px -> bán kính ~16px),
    // cộng thêm chút dung sai (2px) để không quá gắt. Trước đây để 32.0f (gấp đôi kích thước thật)
    // khiến viên đạn bị "hút dính" cả khi nhắm đúng vào khe hở giữa 2 quả -> bắn xuyên khe rất khó.
    float collideRadius = 18.0f;

    float bestDistSq = 1e9f;
    bool found = false;

    hitRow = -1;
    hitCol = -1;

    for(int r = 0; r < ROWS; r++)
    {
        for(int c = 0; c < COLS; c++)
        {
            if(grid[r][c] == EMPTY_CELL)
                continue;

            float cx, cy;
            CellToPixelCenter(r, c, cx, cy);

            float dx = bulletCX - cx;
            float dy = bulletCY - cy;

            float distSq = dx * dx + dy * dy;

            if(distSq <= collideRadius * collideRadius)
            {
                if(distSq < bestDistSq)
                {
                    bestDistSq = distSq;
                    hitRow = r;
                    hitCol = c;
                    found = true;
                }
            }
        }
    }

    return found;
}

// Thêm 1 hàng trứng mới từ trên xuống: đẩy toàn bộ lưới xuống 1 hàng,
// sinh hàng 0 (hàng mới) ngẫu nhiên, rồi kiểm tra thua nếu hàng đáy đã có trứng.
void GSPlayView::AddNewRow()
{
    // 1. Đẩy toàn bộ các hàng xuống 1 hàng (hàng cuối cùng bị đẩy ra khỏi lưới)
    for(int r = ROWS - 1; r > 0; r--)
    {
        for(int c = 0; c < COLS; c++)
        {
            grid[r][c] = grid[r - 1][c];
        }
    }

    // 2. Sinh hàng 0 (hàng mới trên cùng) tuân theo quy tắc cụm cùng màu tối đa 2 quả
    for(int c = 0; c < COLS; c++)
    {
        uint8_t validColors[4];
        int validCount = 0;

        for (uint8_t color = 0; color < 4; color++)
        {
            grid[0][c] = color;

            int dummyRows[ROWS * COLS];
            int dummyCols[ROWS * COLS];
            int groupSize = FindConnectedGroup(0, c, color, dummyRows, dummyCols);

            if (groupSize <= 2)
            {
                validColors[validCount++] = color;
            }
        }

        // Tạm thời đặt lại thành trống
        grid[0][c] = EMPTY_CELL;

        if (validCount > 0)
        {
            uint8_t chosenIndex = (Random() >> 16) % validCount;
            grid[0][c] = validColors[chosenIndex];
        }
        else
        {
            uint8_t bestColor = 0;
            int minSize = 999;
            for (uint8_t color = 0; color < 4; color++)
            {
                grid[0][c] = color;
                int dummyRows[ROWS * COLS];
                int dummyCols[ROWS * COLS];
                int groupSize = FindConnectedGroup(0, c, color, dummyRows, dummyCols);
                if (groupSize < minSize)
                {
                    minSize = groupSize;
                    bestColor = color;
                }
            }
            grid[0][c] = bestColor;
        }
    }

    // (Không cần gọi RenderGrid() ở đây nữa - handleTickEvent() sẽ gọi lại
    //  ngay sau khi hàm này return, tránh render trùng lặp trong cùng 1 tick.)

    // 3. Kiểm tra thua: nếu hàng đáy cùng (ROWS - 1) đã có trứng -> DỪNG GAME
    if(CheckGameOver())
    {
        isGameOver = true;

        presenter->saveHighScore(currentScore);
        UpdateScoreUI();
        gameoverContainer.setVisible(true);
        gameoverContainer.invalidate();
    }

    oddOffset = !oddOffset;
}

// Trả về true nếu hàng đáy cùng (ROWS - 1) đã có ít nhất 1 quả trứng -> thua
bool GSPlayView::CheckGameOver()
{
    for(int c = 0; c < COLS; c++)
    {
        if(grid[ROWS - 1][c] != EMPTY_CELL)
        {
        	playGameOverSound();
        	return true;
        }
    }
    return false;
}

void GSPlayView::tearDownScreen()
{
    GSPlayViewBase::tearDownScreen();
}

void GSPlayView::UpdateScoreUI()
{
    Score.invalidate();
    totalScore.invalidate();

    touchgfx::Unicode::snprintf(ScoreBuffer, SCORE_SIZE, "%d", currentScore);
    touchgfx::Unicode::snprintf(totalScoreBuffer, TOTALSCORE_SIZE, "%d", currentScore);

    Score.resizeToCurrentText();
    totalScore.resizeToCurrentText();

    Score.invalidate();
    totalScore.invalidate();
}

void GSPlayView::InitializeGrid()
{
    // Hàng 0 luôn để TRỐNG, làm vùng đệm để viên đạn luôn có chỗ dính ngay từ đầu game.
    // Số hàng bóng ban đầu: Màn 1 có 2 hàng, Màn 2 có 3 hàng, Màn 3 có 4 hàng.
    int selectedStage = presenter->getSelectedStage();
    int maxInitialRows = selectedStage + 1; // Stage 1 -> 3, Stage 2 -> 4, Stage 3 -> 5

    oddOffset = false;

    for(int r = 0; r < ROWS; r++)
    {
        for(int c = 0; c < COLS; c++)
        {
            if(r == 0 || r >= maxInitialRows)
            {
                grid[r][c] = EMPTY_CELL;
                continue;
            }

            // Tìm màu sắc ngẫu nhiên sao cho không tạo thành cụm quá 2 quả cùng màu nằm cạnh nhau
            uint8_t validColors[4];
            int validCount = 0;

            for (uint8_t color = 0; color < 4; color++)
            {
                grid[r][c] = color;

                int dummyRows[ROWS * COLS];
                int dummyCols[ROWS * COLS];
                int groupSize = FindConnectedGroup(r, c, color, dummyRows, dummyCols);

                if (groupSize <= 2)
                {
                    validColors[validCount++] = color;
                }
            }

            // Tạm thời đặt lại thành trống
            grid[r][c] = EMPTY_CELL;

            if (validCount > 0)
            {
                uint8_t chosenIndex = (Random() >> 16) % validCount;
                grid[r][c] = validColors[chosenIndex];
            }
            else
            {
                // Chọn màu có cụm nhỏ nhất để giảm thiểu liên kết cùng loại
                uint8_t bestColor = 0;
                int minSize = 999;
                for (uint8_t color = 0; color < 4; color++)
                {
                    grid[r][c] = color;
                    int dummyRows[ROWS * COLS];
                    int dummyCols[ROWS * COLS];
                    int groupSize = FindConnectedGroup(r, c, color, dummyRows, dummyCols);
                    if (groupSize < minSize)
                    {
                        minSize = groupSize;
                        bestColor = color;
                    }
                }
                grid[r][c] = bestColor;
            }
        }
    }
}
