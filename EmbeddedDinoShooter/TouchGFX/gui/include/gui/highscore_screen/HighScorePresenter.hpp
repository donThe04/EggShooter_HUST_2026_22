#ifndef HIGHSCOREPRESENTER_HPP
#define HIGHSCOREPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

class HighScoreView;

class HighScorePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    HighScorePresenter(HighScoreView& v);
    virtual ~HighScorePresenter() {}

    virtual void activate();
    virtual void deactivate();

    int getHighScore() const;

private:
    HighScorePresenter();
    HighScoreView& view;
};

#endif // HIGHSCOREPRESENTER_HPP
