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

    // Giá trị đặc biệt nghĩa là "ô trống" (đã nổ hoặc chưa từng có quả)
    static const uint8_t EMPTY_CELL = 255;

    uint8_t grid[ROWS][COLS];

    touchgfx::Image eggs[ROWS * COLS];

    int yOffset;
    bool isDropping;

    uint32_t tickCounter;

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

    // ----- Logic va chạm + nổ bóng -----
    // Chuyển toạ độ pixel (x, y) -> chỉ số (row, col) gần nhất trong lưới
    void PixelToCell(float x, float y, int &outRow, int &outCol);

    // Toạ độ trung tâm pixel của 1 ô (row, col) trong lưới (đã cộng yOffset)
    void CellToPixelCenter(int row, int col, float &outX, float &outY);

    // Kiểm tra viên đạn có va chạm với quả nào trong lưới hoặc "trần" trên cùng không
    // Trả về true nếu có va chạm (và đã xử lý xong: dính lưới hoặc nổ nhóm)
    bool CheckBulletCollision();

    // Lấy danh sách 6 ô lân cận hợp lệ của (row, col) trong hex-grid lệch hàng
    // outNeighbors phải có sức chứa >= 6, trả về số lượng lân cận hợp lệ
    int GetNeighbors(int row, int col, int outRows[6], int outCols[6]);

    // Flood-fill tìm nhóm các ô liên kết cùng màu bắt đầu từ (row, col)
    // outRows/outCols là buffer đủ lớn (ROWS*COLS), trả về số ô tìm được
    int FindConnectedGroup(int row, int col, uint8_t color, int outRows[ROWS * COLS], int outCols[ROWS * COLS]);

    // Xoá 1 nhóm ô khỏi lưới (set EMPTY_CELL + ẩn Image tương ứng)
    void RemoveGroup(int rows[], int cols[], int count);
};

#endif
