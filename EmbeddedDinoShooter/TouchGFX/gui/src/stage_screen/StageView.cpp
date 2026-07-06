#include <gui/stage_screen/StageView.hpp>

StageView::StageView()
{

}

void StageView::setupScreen()
{
    StageViewBase::setupScreen();
}

void StageView::tearDownScreen()
{
    StageViewBase::tearDownScreen();
}

void StageView::setStage1()
{
    presenter->setSelectedStage(1);
}

void StageView::setStage2()
{
    presenter->setSelectedStage(2);
}

void StageView::setStage3()
{
    presenter->setSelectedStage(3);
}
