#ifndef STAGEPRESENTER_HPP
#define STAGEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class StageView;

class StagePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    StagePresenter(StageView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~StagePresenter() {}

    void setSelectedStage(int stage);

private:
    StagePresenter();

    StageView& view;
};

#endif // STAGEPRESENTER_HPP
