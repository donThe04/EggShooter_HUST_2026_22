#include <gui/gsplay_screen/GSPlayView.hpp>
#include <images/BitmapDatabase.hpp>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include <math.h>

extern uint32_t gSeed;

extern "C" void Joystick_Read(uint8_t *outX, uint8_t *outY);
extern "C" uint8_t Button_IsPressed(void);

// Tốc độ bay của viên bóng (pixel/tick)
static const float BULLET_SPEED = 4.0f;

// Số quả tối thiểu trong 1 nhóm cùng màu để nổ
static const int MIN_GROUP_TO_POP = 3;

GSPlayView::GSPlayView()
{
    yOffset = -250;
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

    // Hàng 0 luôn để TRỐNG, làm vùng đệm để viên đạn luôn có chỗ dính ngay từ đầu game.
    // Chỉ lấp trứng cho một số hàng đầu (maxInitialRows), CÁC HÀNG CÒN LẠI ĐỂ TRỐNG
    // -> đảm bảo luôn có ô trống gần khu vực bắn để đạn snap vào, tránh bị "đè" lên quả cũ.
    const int maxInitialRows = 3; // có thể chỉnh 3-5 tùy độ khó mong muốn

    for(int r = 0; r < ROWS; r++)
    {
        for(int c = 0; c < COLS; c++)
        {
            if(r == 0 || r >= maxInitialRows)
            {
                grid[r][c] = EMPTY_CELL;
                continue;
            }

            if(r == 1 && c == 0)
            {
                grid[r][c] = (Random() >> 16) % 4;
            }
            else
            {
                if(((Random() >> 16) % 100) < 70)
                {
                    if(c > 0)
                        grid[r][c] = grid[r][c - 1];
                    else
                        grid[r][c] = grid[r - 1][c];
                }
                else
                {
                    grid[r][c] = (Random() >> 16) % 4;
                }
            }
        }
    }

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

            int x = c * CELL_W + ((r % 2) ? 16 : 0);
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

    int startX = 104;
    int startY = 250;

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

        // 4. render grid
        RenderGrid();

        // 5. Kiểm tra nhóm cùng màu để nổ (PHẦN TRƯỚC ĐÂY BỊ THIẾU)
        int groupRows[ROWS * COLS];
        int groupCols[ROWS * COLS];
        int groupCount = FindConnectedGroup(tr, tc, bulletColor, groupRows, groupCols);

        if(groupCount >= MIN_GROUP_TO_POP)
        {
            RemoveGroup(groupRows, groupCols, groupCount);
            RenderGrid();
        }

        // 6. Kiểm tra thua: nếu quả vừa dính nằm ở hàng đáy cùng -> DỪNG GAME
        if(CheckGameOver())
        {
            isGameOver = true;
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

    float colOffset = (row % 2) ? 16.0f : 0.0f;
    int col = (int)roundf((x - colOffset) / (float)CELL_W);

    if(col < 0) col = 0;
    if(col >= COLS) col = COLS - 1;

    outRow = row;
    outCol = col;
}

void GSPlayView::CellToPixelCenter(int row, int col, float &outX, float &outY)
{
    float colOffset = (row % 2) ? 16.0f : 0.0f;

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

int GSPlayView::GetNeighbors(int row, int col, int outRows[6], int outCols[6])
{
    int count = 0;

    // Lân cận cùng hàng: trái, phải
    int sameRowOffsets[2][2] = { {0, -1}, {0, 1} };

    // Lân cận hàng trên/dưới: phụ thuộc parity (hàng chẵn/lẻ) do lưới lệch
    // Hàng lẻ (offset +16, lệch phải so với hàng chẵn): hàng trên/dưới lân cận là (col, col+1)
    // Hàng chẵn: hàng trên/dưới lân cận là (col-1, col)
    int diagOffsets[2];
    if(row % 2 == 0)
    {
        diagOffsets[0] = -1; // col - 1
        diagOffsets[1] = 0;  // col
    }
    else
    {
        diagOffsets[0] = 0;  // col
        diagOffsets[1] = 1;  // col + 1
    }

    // Cùng hàng
    for(int i = 0; i < 2; i++)
    {
        int nr = row + sameRowOffsets[i][0];
        int nc = col + sameRowOffsets[i][1];
        if(nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS)
        {
            outRows[count] = nr;
            outCols[count] = nc;
            count++;
        }
    }

    // Hàng trên (row - 1)
    for(int i = 0; i < 2; i++)
    {
        int nr = row - 1;
        int nc = col + diagOffsets[i];
        if(nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS)
        {
            outRows[count] = nr;
            outCols[count] = nc;
            count++;
        }
    }

    // Hàng dưới (row + 1)
    for(int i = 0; i < 2; i++)
    {
        int nr = row + 1;
        int nc = col + diagOffsets[i];
        if(nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS)
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

    // 2. Sinh hàng 0 (hàng mới trên cùng) - dùng cùng cách random như setupScreen
    for(int c = 0; c < COLS; c++)
    {
        if(c == 0)
        {
            grid[0][c] = (Random() >> 16) % 4;
        }
        else
        {
            if(((Random() >> 16) % 100) < 70)
            {
                grid[0][c] = grid[0][c - 1];
            }
            else
            {
                grid[0][c] = (Random() >> 16) % 4;
            }
        }
    }

    // (Không cần gọi RenderGrid() ở đây nữa - handleTickEvent() sẽ gọi lại
    //  ngay sau khi hàm này return, tránh render trùng lặp trong cùng 1 tick.)

    // 3. Kiểm tra thua: nếu hàng đáy cùng (ROWS - 1) đã có trứng -> DỪNG GAME
    if(CheckGameOver())
    {
        isGameOver = true;
        // TODO: nếu có màn Game Over / màn Menu riêng, gọi chuyển màn tại đây, ví dụ:
        // application().gotoGSMenuScreenBlockTransition();
    }
}

// Trả về true nếu hàng đáy cùng (ROWS - 1) đã có ít nhất 1 quả trứng -> thua
bool GSPlayView::CheckGameOver()
{
    for(int c = 0; c < COLS; c++)
    {
        if(grid[ROWS - 1][c] != EMPTY_CELL)
        {
            return true;
        }
    }
    return false;
}

void GSPlayView::tearDownScreen()
{
    GSPlayViewBase::tearDownScreen();
}
