//
//  MediaFilePanel.h
//  libslic3r_gui
//
//  Created by cmguo on 2021/12/7.
//

#ifndef MediaFilePanel_h
#define MediaFilePanel_h

#include "GUI_Utils.hpp"
#include "wxExtensions.hpp"

#include <wx/frame.h>

class Button;
class SwitchButton;
class Label;
class StaticBox;
class PrinterFileSystem;

namespace Slic3r {

class MachineObject;

namespace GUI {

class ImageGrid;

class MediaFilePanel : public wxPanel
{
public:
    MediaFilePanel(wxWindow * parent);
    
    ~MediaFilePanel();

    void SetMachineObject(MachineObject * obj);

public:
    void Rescale();

private:
    void modeChanged(wxCommandEvent & e);

    void fetchUrl(boost::weak_ptr<PrinterFileSystem> fs);

private:
    ScalableBitmap m_bmp_loading;
    ScalableBitmap m_bmp_failed;
    ScalableBitmap m_bmp_empty;

    ::StaticBox *m_time_panel = nullptr;
    ::Button    *m_button_year = nullptr;
    ::Button    *m_button_month = nullptr;
    ::Button    *m_button_all = nullptr;
    ::Label     *m_switch_label = nullptr;

    ::StaticBox *   m_type_panel    = nullptr;
    ::Button *      m_button_video   = nullptr;
    ::Button *      m_button_timelapse  = nullptr;

    ::StaticBox *m_manage_panel        = nullptr;
    ::Button *   m_button_delete     = nullptr;
    ::Button *m_button_download = nullptr;
    ::Button *m_button_management = nullptr;

    std::string m_machine;
    ImageGrid * m_image_grid = nullptr;

    int m_last_mode = 0;
    int m_last_type = 0;
};


class MediaFileFrame : public DPIFrame
{
public:
    MediaFileFrame(wxWindow * parent);

    MediaFilePanel * filePanel() { return m_panel; }

    virtual void on_dpi_changed(const wxRect& suggested_rect);

private:
    MediaFilePanel* m_panel;
};

}}
#endif /* MediaFilePanel_h */
