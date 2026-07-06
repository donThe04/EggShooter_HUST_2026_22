#include <gui/stage_screen/StageView.hpp>
#include <gui/stage_screen/StagePresenter.hpp>
#include <gui/model/Model.hpp>

StagePresenter::StagePresenter(StageView& v)
    : view(v)
{

}

void StagePresenter::activate()
{

}

void StagePresenter::deactivate()
{

}

void StagePresenter::setSelectedStage(int stage)
{
    model->setSelectedStage(stage);
}
