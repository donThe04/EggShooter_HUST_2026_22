#include <gui/gsplay_screen/GSPlayView.hpp>
#include <images/BitmapDatabase.hpp>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include <math.h>

extern uint32_t gSeed;

GSPlayView::GSPlayView()
{
    yOffset = -250;
    isDropping = true;
    tickCounter = 0;
}

uint32_t GSPlayView::Random()
{
    gSeed = gSeed * 1103515245 + 12345;
    return gSeed;
}

void GSPlayView::setupScreen()
{
    GSPlayViewBase::setupScreen();

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (r == 0 && c == 0)
            {
                grid[r][c] = (Random() >> 16) % 4;
            }
            else
            {
                if (((Random() >> 16) % 100) < 70)
                {
                    if (c > 0)
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

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            containerGrid.add(eggs[index]);
            index++;
        }
    }

    for (int i = 0; i < 8; i++)
    {
        aimDots[i].setBitmap(touchgfx::Bitmap(BITMAP_DOTS_ID));
        add(aimDots[i]);
    }

    UpdateAim();
    RenderGrid();
}

void GSPlayView::RenderGrid()
{
    int index = 0;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            switch (grid[r][c])
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

            if (y > -32 && y < 200)
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
    if (!isDropping)
        return;

    tickCounter++;
    gunAngle += 1;

    if (gunAngle > 150)
    {
        gunAngle = 30;
    }

    UpdateAim();

    if (tickCounter >= 7)
    {
        tickCounter = 0;
        yOffset += 2;

        if (yOffset >= FINAL_OFFSET)
        {
            yOffset = FINAL_OFFSET;
            isDropping = false;
        }

        RenderGrid();
    }
}

void GSPlayView::UpdateAim()
{
    float rad = gunAngle * 3.1415926f / 180.0f;

    int startX = 104;
    int startY = 250;

    for (int i = 0; i < 8; i++)
    {
        int distance = (i + 1) * 20;

        int x = startX + cosf(rad) * distance;
        int y = startY - sinf(rad) * distance;

        aimDots[i].setXY(x, y);
        aimDots[i].invalidate();
    }
}

void GSPlayView::tearDownScreen()
{
    GSPlayViewBase::tearDownScreen();
}
