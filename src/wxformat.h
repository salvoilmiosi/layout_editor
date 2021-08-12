#ifndef __WXFORMAT_H__
#define __WXFORMAT_H__

#include "translations.h"
#include "enums.h"

#include <wx/string.h>

namespace util {
    inline wxString to_wx(std::string_view str) {
        return wxString::FromUTF8(str.data(), str.size());
    }
}

namespace intl {
    template<typename T, typename ... Ts>
    inline wxString wxformat(T &&fmt_str, Ts && ... args) {
        return util::to_wx(format(std::forward<T>(fmt_str), std::forward<Ts>(args) ...));
    }
}

namespace enums {
    template<enums::is_enum T>
    inline auto get_label(T num) {
        return intl::wxformat(std::format("{}::{}", magic_enum::enum_type_name<T>(), magic_enum::enum_name(num)));
    }
}

#endif