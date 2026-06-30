#ifndef GSMENUVIEW_HPP
#define GSMENUVIEW_HPP

#include <gui_generated/gsmenu_screen/GSMenuViewBase.hpp>
#include <gui/gsmenu_screen/GSMenuPresenter.hpp>

class GSMenuView : public GSMenuViewBase
{
public:
    GSMenuView();
    virtual ~GSMenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // GSMENUVIEW_HPP
