#include <gui/highscore_screen/HighScoreView.hpp>

HighScoreView::HighScoreView()
{
}

void HighScoreView::setupScreen()
{
    HighScoreViewBase::setupScreen();

    int score = presenter->getHighScore();

    touchgfx::Unicode::snprintf(
        scoreBuffer,
        sizeof(scoreBuffer) / sizeof(Unicode::UnicodeChar),
        "%d",
        score
    );

    textAreaScore.setWildcard1(scoreBuffer);
    textAreaScore.invalidate();
}

void HighScoreView::tearDownScreen()
{
    HighScoreViewBase::tearDownScreen();
}
