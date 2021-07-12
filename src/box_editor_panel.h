#ifndef __BOX_EDITOR_PANEL_H__
#define __BOX_EDITOR_PANEL_H__

#include "image_panel.h"
#include "editor.h"
#include "text_dialog.h"

using namespace bls;

DEFINE_ENUM_FLAGS(direction,
    (TOP)
    (LEFT)
    (BOTTOM)
    (RIGHT)
)

struct resize_node {
    layout_box_list::iterator box;
    enums::bitset<direction> directions;
};

class box_editor_panel : public wxImagePanel {
public:
    box_editor_panel(wxWindow *parent, class frame_editor *app);

    void setSelectedTool(int tool) {
        selected_tool = tool;
    }

    void setSelectedBox(layout_box *box) {
        selected_box = box;
    }

protected:
    void render(wxDC &dc) override;

    void OnMouseDown(wxMouseEvent &evt);
    void OnMouseUp(wxMouseEvent &evt);
    void OnDoubleClick(wxMouseEvent &evt);
    void OnMouseMove(wxMouseEvent &evt);
    void OnKeyDown(wxKeyEvent &evt);
    void OnKeyUp(wxKeyEvent &evt);

private:
    wxRealPoint screen_to_layout(const wxPoint &pt) {
        auto [xx, yy] = CalcUnscrolledPosition(pt);
        return wxRealPoint(
            std::clamp(xx / scaled_width(), 0.0, 1.0),
            std::clamp(yy / scaled_height(), 0.0, 1.0)
        );
    }

    layout_box_list::iterator getBoxAt(float x, float y);
    resize_node getBoxResizeNode(float x, float y);

private:
    class frame_editor *app;

    TextDialog *info_dialog;

    wxRealPoint start_pt, end_pt, dragging_offset;
    layout_box *selected_box = nullptr;
    enums::bitset<direction> node_directions;
    bool mouseIsDown = false;

    int selected_tool = TOOL_SELECT;

private:
    DECLARE_EVENT_TABLE()
};

#endif