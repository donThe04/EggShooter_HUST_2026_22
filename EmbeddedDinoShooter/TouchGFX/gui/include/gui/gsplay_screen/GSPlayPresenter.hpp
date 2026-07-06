#ifndef GSPLAYPRESENTER_HPP
#define GSPLAYPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class GSPlayView;

class GSPlayPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    GSPlayPresenter(GSPlayView& v);

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

    virtual ~GSPlayPresenter() {}

    int getHighScore() const;
    void saveHighScore(int score);
    int getSelectedStage() const;

private:
    GSPlayPresenter();

    GSPlayView& view;
};

#endif // GSPLAYPRESENTER_HPP
