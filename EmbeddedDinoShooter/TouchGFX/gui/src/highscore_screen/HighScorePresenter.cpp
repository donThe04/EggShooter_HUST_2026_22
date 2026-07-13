#include <gui/highscore_screen/HighScoreView.hpp>
#include <gui/highscore_screen/HighScorePresenter.hpp>
#include <gui/model/Model.hpp>

HighScorePresenter::HighScorePresenter(HighScoreView& v) : view(v)
{
}

void HighScorePresenter::activate()
{
}

void HighScorePresenter::deactivate()
{
}

int HighScorePresenter::getHighScore() const
{
    return model->getHighScore();
}
