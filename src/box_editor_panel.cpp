#include "box_editor_panel.h"

#include "box_dialog.h"

#include <wx/choicdlg.h>
#include <wx/dcbuffer.h>

#include "move_page_dialog.h"

using namespace enums::flag_operators;

BEGIN_EVENT_TABLE(box_editor_panel, wxImagePanel)
    EVT_LEFT_DOWN(box_editor_panel::OnMouseDown)
    EVT_LEFT_UP(box_editor_panel::OnMouseUp)
    EVT_LEFT_DCLICK(box_editor_panel::OnDoubleClick)
    EVT_MOTION(box_editor_panel::OnMouseMove)
    EVT_KEY_DOWN(box_editor_panel::OnKeyDown)
    EVT_KEY_UP(box_editor_panel::OnKeyUp)
END_EVENT_TABLE()

box_editor_panel::box_editor_panel(wxWindow *parent, frame_editor *app) : wxImagePanel(parent), app(app) {
    info_dialog = new TextDialog(this, wxintl::translate("TEST_OUTPUT"));
}

static void clamp_rect(pdf_rect &rect) {
    if (rect.x < 0.f) {
        rect.w += rect.x;
        rect.x = 0.f;
    }
    if (rect.x + rect.w > 1.f) {
        rect.w = 1.f - rect.x;
    }
    if (rect.y < 0.f) {
        rect.h += rect.y;
        rect.y = 0.f;
    }
    if (rect.y + rect.h > 1.f) {
        rect.h = 1.f - rect.y;
    }
}

void box_editor_panel::render(wxDC &dc) {
    wxImagePanel::render(dc);

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    for (auto &box : app->layout) {
        if (box.page == app->getSelectedPage()) {
            if (selected_box == &box) {
                dc.SetPen(*wxBLACK_DASHED_PEN);
            } else {
                dc.SetPen(*wxBLACK_PEN);
            }
            pdf_rect r = box;
            clamp_rect(r);
            dc.DrawRectangle(
                r.x * scaled_width(),
                r.y * scaled_height(),
                r.w * scaled_width(),
                r.h * scaled_height()
            );
        }
    }

    switch (selected_tool) {
    case TOOL_NEWBOX:
    case TOOL_TEST:
        if (mouseIsDown) {
            dc.SetPen(*wxBLACK_PEN);
            dc.DrawRectangle(wxRect (
                wxPoint(start_pt.x * scaled_width(), start_pt.y * scaled_height()),
                wxPoint(end_pt.x * scaled_width(), end_pt.y * scaled_height())
            ));
        }
        break;
    }
}

template<typename T>
auto find_iterator(const std::list<T> &list, const T *ptr) {
    return std::ranges::find(list, ptr, [](const T &obj) { return &obj; });
};

layout_box *box_editor_panel::getBoxAt(float x, float y) {
    auto check_box = [&](const layout_box &box) {
        return (x > box.x && x < box.x + box.w && y > box.y && y < box.y + box.h && box.page == app->getSelectedPage());
    };
    if (selected_box && check_box(*selected_box)) return selected_box;
    for (auto &box : app->layout) {
        if (check_box(box)) return &box;
    }
    return nullptr;
};

resize_node box_editor_panel::getBoxResizeNode(float x, float y) {
    constexpr float RESIZE_TOLERANCE = 8.f;

    float nw = RESIZE_TOLERANCE / scaled_width();
    float nh = RESIZE_TOLERANCE / scaled_height();

    auto check_box = [&](const layout_box &box) {
        direction node{};
        if (box.page == app->getSelectedPage()) {
            if (y > box.y - nh && y < box.y + box.h + nh) {
                if (x > box.x - nw && x < box.x + nw) {
                    node |= direction::LEFT;
                } else if (x > box.x + box.w - nw && x < box.x + box.w + nw) {
                    node |= direction::RIGHT;
                }
            }
            if (x > box.x - nw && x < box.x + box.w + nw) {
                if (y > box.y - nh && y < box.y + nh) {
                    node |= direction::TOP;
                } else if (y > box.y + box.h - nh && y < box.y + box.h + nh) {
                    node |= direction::BOTTOM;
                }
            }
        }
        return node;
    };
    if (selected_box) {
        if (auto node = check_box(*selected_box); bool(node)) return {selected_box, node};
    }
    for (auto &box : app->layout) {
        if (auto node = check_box(box); bool(node)) return {&box, node};
    }
    return {};
};

void box_editor_panel::OnMouseDown(wxMouseEvent &evt) {
    start_pt = screen_to_layout(evt.GetPosition());
    if (raw_image.IsOk() && !mouseIsDown) {
        switch (selected_tool) {
        case TOOL_SELECT: {
            layout_box *box = getBoxAt(start_pt.x, start_pt.y);
            if (box) {
                app->selectBox(box);
                dragging_offset.x = selected_box->x - start_pt.x;
                dragging_offset.y = selected_box->y - start_pt.y;
                mouseIsDown = true;
            } else {
                app->selectBox(nullptr);
            }
            break;
        }
        case TOOL_NEWBOX:
        case TOOL_TEST:
            mouseIsDown = true;
            break;
        case TOOL_DELETEBOX: {
            auto *box = getBoxAt(start_pt.x, start_pt.y);
            if (box && box_dialog::closeDialog(*box)) {
                app->layout.erase(find_iterator(app->layout, box));
                app->updateLayout();
                Refresh();
            }
            break;
        }
        case TOOL_RESIZE: {
            if (auto node = getBoxResizeNode(start_pt.x, start_pt.y)) {
                app->selectBox(node.box);
                node_directions = node.directions;
                mouseIsDown = true;
            } else {
                app->selectBox(nullptr);
            }
            break;
        }
        case TOOL_MOVEPAGE: {
            layout_box *box = getBoxAt(start_pt.x, start_pt.y);
            if (box) {
                app->selectBox(box);
                MovePageDialog(app, box).ShowModal();
            } else {
                app->selectBox(nullptr);
            }
            break;
        }
        }
    }
    evt.Skip();
}

void box_editor_panel::OnMouseUp(wxMouseEvent &evt) {
    if (mouseIsDown) {
        mouseIsDown = false;

        end_pt = screen_to_layout(evt.GetPosition());
        if (end_pt != start_pt) {
            switch (selected_tool) {
            case TOOL_SELECT:
                if (selected_box && start_pt != end_pt) {
                    clamp_rect(*selected_box);
                    app->updateLayout();
                    app->selectBox(selected_box);
                }
                break;
            case TOOL_NEWBOX: {
                auto &box = *app->layout.emplace(selected_box ? find_iterator(app->layout, selected_box) : app->layout.end());
                box.x = std::min(start_pt.x, end_pt.x);
                box.y = std::min(start_pt.y, end_pt.y);
                box.w = std::abs(start_pt.x - end_pt.x);
                box.h = std::abs(start_pt.y - end_pt.y);
                clamp_rect(box);
                box.page = app->getSelectedPage();
                app->updateLayout();
                app->selectBox(&box);
                box_dialog::openDialog(app, box);
                break;
            }
            case TOOL_TEST: {
                static auto choices = []<read_mode ... Es> (enums::enum_sequence<Es...>) {
                    wxString strings[] = {wxintl::enum_label(Es) ... };
                    return wxArrayString(sizeof(strings) / sizeof(wxString), strings);
                } (enums::make_enum_sequence<read_mode>());
                wxSingleChoiceDialog diag(this, wxintl::translate("DIALOG_TEST_READ_MODE"), wxintl::translate("DIALOG_TEST_READ_TITLE"), choices);
                if (diag.ShowModal() == wxID_OK) {
                    pdf_rect box;
                    box.x = std::min(start_pt.x, end_pt.x);
                    box.y = std::min(start_pt.y, end_pt.y);
                    box.w = std::abs(start_pt.x - end_pt.x);
                    box.h = std::abs(start_pt.y - end_pt.y);
                    box.page = app->getSelectedPage();
                    box.mode = static_cast<read_mode>(diag.GetSelection());
                    box.rotate(app->getBoxRotation());
                    info_dialog->ShowText(wxintl::to_wx(app->getPdfDocument().get_text(box)));
                }
                break;
            }
            case TOOL_DELETEBOX:
                break;
            case TOOL_RESIZE:
                if (selected_box) {
                    if (start_pt != end_pt) {
                        if (selected_box->w < 0) {
                            selected_box->w = -selected_box->w;
                            selected_box->x -= selected_box->w;
                        }
                        if (selected_box->h < 0) {
                            selected_box->h = -selected_box->h;
                            selected_box->y -= selected_box->h;
                        }
                        app->updateLayout();
                        app->selectBox(selected_box);
                    }
                }
                OnMouseMove(evt); // changes cursor
                break;
            }
        }

        Refresh();
    }
    evt.Skip();
}

void box_editor_panel::OnDoubleClick(wxMouseEvent &evt) {
    switch (selected_tool) {
    case TOOL_SELECT:
        if (selected_box) {
            box_dialog::openDialog(app, *selected_box);
        }
    default:
        break;
    }
}

void box_editor_panel::OnMouseMove(wxMouseEvent &evt) {
    end_pt = screen_to_layout(evt.GetPosition());

    if (mouseIsDown) {
        switch (selected_tool) {
        case TOOL_SELECT:
            selected_box->x = dragging_offset.x + end_pt.x;
            selected_box->y = dragging_offset.y + end_pt.y;
            break;
        case TOOL_RESIZE: {
            if (bool(node_directions & direction::TOP)) {
                selected_box->h = selected_box->y + selected_box->h - end_pt.y;
                selected_box->y = end_pt.y;
            } else if (bool(node_directions & direction::BOTTOM)) {
                selected_box->h = end_pt.y - selected_box->y;
            }
            if (bool(node_directions & direction::LEFT)) {
                selected_box->w = selected_box->x + selected_box->w - end_pt.x;
                selected_box->x = end_pt.x;
            } else if (bool(node_directions & direction::RIGHT)) {
                selected_box->w = end_pt.x - selected_box->x;
            }
            break;
        }
        default:
            break;
        }
        Refresh();
        if (evt.Leaving()) {
            OnMouseUp(evt);
        }
    } else {
        if (selected_tool == TOOL_RESIZE) {
            switch (getBoxResizeNode(end_pt.x, end_pt.y).directions) {
            case direction::LEFT:
            case direction::RIGHT:
                SetCursor(wxStockCursor::wxCURSOR_SIZEWE);
                break;
            case direction::TOP:
            case direction::BOTTOM:
                SetCursor(wxStockCursor::wxCURSOR_SIZENS);
                break;
            case direction::LEFT | direction::TOP:
            case direction::RIGHT | direction::BOTTOM:
                SetCursor(wxStockCursor::wxCURSOR_SIZENWSE);
                break;
            case direction::RIGHT | direction::TOP:
            case direction::LEFT | direction::BOTTOM:
                SetCursor(wxStockCursor::wxCURSOR_SIZENESW);
                break;
            default:
                SetCursor(*wxSTANDARD_CURSOR);
                break;
            }
        }
    }
    evt.Skip();
}

void box_editor_panel::OnKeyDown(wxKeyEvent &evt) {
    constexpr float MOVE_AMT = 5.f;
    if (selected_box) {
        switch (evt.GetKeyCode()) {
            case WXK_LEFT: selected_box->x -= MOVE_AMT / scaled_width(); Refresh(); break;
            case WXK_RIGHT: selected_box->x += MOVE_AMT / scaled_width(); Refresh(); break;
            case WXK_UP: selected_box->y -= MOVE_AMT / scaled_height(); Refresh(); break;
            case WXK_DOWN: selected_box->y += MOVE_AMT / scaled_height(); Refresh(); break;
        }
    }
}

void box_editor_panel::OnKeyUp(wxKeyEvent &evt) {
    if (selected_box) {
        switch (evt.GetKeyCode()) {
        case WXK_LEFT:
        case WXK_RIGHT:
        case WXK_UP:
        case WXK_DOWN:
            app->updateLayout();
            app->selectBox(selected_box);
        }
    }
}