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

    virtual void UpdateAim();

    touchgfx::Image aimDots[8];
    float gunAngle = 90.0f;

    void RenderGrid();

private:
    uint32_t Random();

    static const int ROWS = 8;
    static const int COLS = 7;

    static const int CELL_W = 32;
    static const int CELL_H = 24;

    static const int FINAL_OFFSET = 0;

    uint8_t grid[ROWS][COLS];
    touchgfx::Image eggs[ROWS * COLS];

    int yOffset;
    bool isDropping;
    uint32_t tickCounter;
};

#endif
