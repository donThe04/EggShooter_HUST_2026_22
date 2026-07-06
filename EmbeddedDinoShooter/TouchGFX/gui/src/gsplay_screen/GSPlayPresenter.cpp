#include <gui/gsplay_screen/GSPlayView.hpp>
#include <gui/gsplay_screen/GSPlayPresenter.hpp>
#include <gui/model/Model.hpp>

GSPlayPresenter::GSPlayPresenter(GSPlayView& v)
    : view(v)
{

}

void GSPlayPresenter::activate()
{

}

void GSPlayPresenter::deactivate()
{

}

int GSPlayPresenter::getHighScore() const
{
    return model->getHighScore();
}

void GSPlayPresenter::saveHighScore(int score)
{
    model->saveHighScore(score);
}

int GSPlayPresenter::getSelectedStage() const
{
    return model->getSelectedStage();
}
