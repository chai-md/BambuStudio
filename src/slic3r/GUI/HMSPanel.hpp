#ifndef slic3r_HMSPanel_hpp_
#define slic3r_HMSPanel_hpp_

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <slic3r/GUI/Widgets/Button.hpp>
#include <slic3r/GUI/DeviceManager.hpp>
#include <slic3r/GUI/Widgets/ScrolledWindow.hpp>
#include <wx/html/htmlwin.h>

namespace Slic3r {
namespace GUI {

class HMSNotifyItem : public wxPanel
{
    HMSItem &   m_hms_item;
    std::string m_url;

    wxPanel *       m_panel_hms;
    wxStaticBitmap *m_bitmap_notify;
    wxStaticBitmap *m_bitmap_arrow;
    wxStaticText *  m_hms_content;
    wxHtmlWindow *  m_html;
    wxPanel *       m_staticline;

    wxBitmap m_img_notify_lv1;
    wxBitmap m_img_notify_lv2;
    wxBitmap m_img_notify_lv3;
    wxBitmap m_img_arrow;

    void          init_bitmaps();
    wxBitmap &    get_notify_bitmap();

public:
     HMSNotifyItem(wxWindow *parent, HMSItem& item);
    ~HMSNotifyItem();

     void msw_rescale() {}
};

class HMSPanel : public wxPanel
{
protected:
    wxScrolledWindow *m_scrolledWindow;
    wxBoxSizer *      m_top_sizer;
    HMSNotifyItem *   m_notify_item;

    void append_hms_panel(HMSItem &item);
    void delete_hms_panels();

public:
    HMSPanel(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
    ~HMSPanel();

    void msw_rescale() {}

    bool Show(bool show = true) override;

    void update(MachineObject *obj_);

    MachineObject *obj { nullptr };
};

}
}

#endif
