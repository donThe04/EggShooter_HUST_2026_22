#ifndef GSPLAYVIEW_HPP
#define GSPLAYVIEW_HPP

#include <gui_generated/gsplay_screen/GSPlayViewBase.hpp>
#include <gui/gsplay_screen/GSPlayPresenter.hpp>

#include <touchgfx/widgets/Image.hpp>

class GSPlayView : public GSPlayViewBase
{
public:
    GSPlayView();
    virtual ~GSPlayView() {}

    virtual void setupScreen();

    virtual void tearDownScreen();

    virtual void handleTickEvent();

    virtual void pauseClicked() override;

    virtual void continueClicked() override;

    virtual void restartClicked() override;

    virtual void exitClicked() override;

    virtual void restartgameoverClicked() override;

    virtual void exitgameoverClicked() override;

    void playTone(uint32_t freq, uint32_t durationMs);
    void playGameOverSound();

    void ResetGame();

    virtual void UpdateAim();
    touchgfx::Image aimDots[8];

    float gunAngle = 90.0f;

    void RenderGrid();

private:
    uint32_t Random();

    bool isPaused = false;

    static const int ROWS = 8;
    static const int COLS = 7;

    static const int CELL_W = 32;
    static const int CELL_H = 24;

    static const int FINAL_OFFSET = 0;

    bool oddOffset;

    // Giá trị đặc biệt nghĩa là "ô trống" (đã nổ hoặc chưa từng có quả)
    static const uint8_t EMPTY_CELL = 255;

    uint8_t grid[ROWS][COLS];

    touchgfx::Image eggs[ROWS * COLS];

    int yOffset;
    bool isDropping;

    uint32_t tickCounter;

    static const uint32_t SCROLL_TICK_INTERVAL = 25;
    uint32_t scrollTickCounter;
    bool isGameOver;

    void AddNewRow();
    bool CheckGameOver();

    // ----- Viên bóng bắn ra -----
    touchgfx::Image bullet;
    bool isBulletFlying;
    float bulletX, bulletY;
    float bulletVX, bulletVY;
    uint8_t bulletColor;        // màu CỦA RIÊNG viên đạn đang bay (khác currentEggColor, vì currentEggColor đổi ngay khi bắn)

    bool wasButtonPressed;

    // ----- Màu quả hiện tại (currentEgg) và quả kế tiếp (nextEgg) -----
    uint8_t currentEggColor;
    uint8_t nextEggColor;

    void SetEggBitmap(touchgfx::Image& img, uint8_t color);
    void RollNextEgg();

    void FireBullet();
    void UpdateBullet();


    void PixelToCell(float x, float y, int &outRow, int &outCol);


    void CellToPixelCenter(int row, int col, float &outX, float &outY);

    bool CheckBulletCollision(int &hitRow, int &hitCol);

    int GetNeighbors(int row, int col, int outRows[6], int outCols[6]);


    int FindConnectedGroup(int row, int col, uint8_t color, int outRows[ROWS * COLS], int outCols[ROWS * COLS]);

    bool FindSnapCell(int hitRow, int hitCol,int &targetRow, int &targetCol);

    void RemoveGroup(int rows[], int cols[], int count);

    int currentScore;
    void UpdateScoreUI();
    void InitializeGrid();
};

#endif

