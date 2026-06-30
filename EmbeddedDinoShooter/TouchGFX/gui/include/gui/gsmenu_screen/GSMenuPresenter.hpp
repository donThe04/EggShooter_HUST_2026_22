#ifndef GSMENUPRESENTER_HPP
#define GSMENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class GSMenuView;

class GSMenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    GSMenuPresenter(GSMenuView& v);

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

    virtual ~GSMenuPresenter() {}

private:
    GSMenuPresenter();

    GSMenuView& view;
};

#endif // GSMENUPRESENTER_HPP
