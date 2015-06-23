#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/rawbmp.h>

#include "inputsource.h"

class wxImagePanel : public wxScrolled<wxPanel>
{
private:
    InputSource *input_source = nullptr;
    float *input_data = nullptr;
    int old_width;
    int old_height;

public:
    wxImagePanel(wxFrame *parent, InputSource *input_source);
    ~wxImagePanel();

    void paintEvent(wxPaintEvent &evt);
    void paintNow();

    void OnDraw(wxDC &dc);
    void OnMouseWheel(wxMouseEvent &event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(wxImagePanel, wxScrolled<wxPanel>)
    EVT_MOUSEWHEEL(wxImagePanel::OnMouseWheel)
wxEND_EVENT_TABLE()

class MyApp: public wxApp
{
public:
    virtual bool OnInit();
};

class MyFrame: public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
    void OnExit(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnExit)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

wxImagePanel::wxImagePanel(wxFrame *parent, InputSource *input_source) : wxScrolled<wxPanel>(parent)
{
    this->input_source = input_source;
    SetVirtualSize(input_source->GetWidth(), input_source->GetHeight());
    SetScrollRate(10, 10);
}

wxImagePanel::~wxImagePanel()
{
    free(input_data);
}

int clamp(int a, int b, int c) {
    if (a < b) return b;
    if (a > c) return c;
    return a;
}

void wxImagePanel::OnDraw(wxDC &dc)
{
    int x, y;
    int xunit, yunit;
    int width, height;
    GetViewStart(&x, &y);
    GetScrollPixelsPerUnit(&xunit, &yunit);
    GetClientSize(&width, &height);

    // TODO: maybe change this
    width = input_source->GetWidth();

    x *= xunit;
    y *= yunit;

    if (input_data == nullptr || width != old_width || height != old_height)
        input_data = (float*)realloc(input_data, width * height * sizeof(float));
    input_source->GetViewport(input_data, 0, y, width, height, 0);
    old_width = width;
    old_height = height;

    wxBitmap image(width, height, 24);
    wxNativePixelData pixel_data(image);
    wxNativePixelData::Iterator pix(pixel_data);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pix.Blue() = clamp(256 - input_data[i*width + j] * -2, 0, 255);
            pix.Red() = 0;
            pix.Green() = 0;
            pix++;
        }
    }

    dc.DrawBitmap(image, 0, y, false);
}

void wxImagePanel::OnMouseWheel(wxMouseEvent &event)
{
    if (event.ControlDown() && event.GetWheelRotation() != 0) {
        bool forward = event.GetWheelRotation() > 0;
        int x, y;
        GetViewStart(&x, &y);
        if (forward) {
            if (input_source->ZoomIn()) {
                Scroll(x, y*2);
            }
        } else {
            if (input_source->ZoomOut()) {
                Scroll(x, y/2);
            }
        }

        SetVirtualSize(input_source->GetWidth(), input_source->GetHeight());
        Refresh();
    } else {
        event.Skip();
    }
}


bool MyApp::OnInit()
{
    wxInitAllImageHandlers();
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    MyFrame *frame = new MyFrame( "inspectrum", wxPoint(50, 50), wxSize(1024, 500) );

    if (argc < 2)
    {
        printf("Missing filename\n");
        printf("Usage: %s <filename> [fft size]\n", argv[0].mb_str().data());
        return false;
    }

    int fft_size = 1024;
    if (argc > 2) {
        int size = atoi(argv[2]);
        if (size > 0)
            fft_size = size;
    }

    InputSource *is = new InputSource(argv[1], fft_size);

    wxImagePanel *impanel = new wxImagePanel(frame, is);
    sizer->Add(impanel, 1, wxALL | wxEXPAND, 0);
    frame->SetSizer(sizer);

    frame->Show( true );
    return true;
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size)
{
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}
