#ifndef __WXFORMAT_H__
#define __WXFORMAT_H__

#include "translations.h"
#include "enums.h"

#include <wx/string.h>

namespace wxintl {
    inline wxString to_wx(std::string_view str) {
        return wxString::FromUTF8(str.data(), str.size());
    }

    template<typename ... Ts>
    inline wxString translate(Ts && ... args) {
        return to_wx(intl::translate(std::forward<Ts>(args) ...));
    }

    template<enums::is_enum T>
    inline auto enum_label(T num) {
        return translate(std::format("{}::{}", magic_enum::enum_type_name<T>(), magic_enum::enum_name(num)));
    }
}

#endif