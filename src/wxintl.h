#ifndef __WXFORMAT_H__
#define __WXFORMAT_H__

#include "utils/translations.h"
#include <wx/string.h>

namespace wxintl {
    inline wxString to_wx(std::string_view str) {
        return wxString::FromUTF8(str.data(), str.size());
    }

    template<typename ... Ts>
    inline wxString translate(Ts && ... args) {
        return to_wx(intl::translate(std::forward<Ts>(args) ...));
    }

    template<enums::reflected_enum E>
    inline wxString enum_label(E value) {
        return to_wx(intl::enum_label(value));
    }
}

#endif