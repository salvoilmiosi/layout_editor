#ifndef __RESOURCES_H__
#define __RESOURCES_H__

#include <string_view>
#include <wx/bitmap.h>

#define RESOURCE_DATA(name) __resource__##name
#define RESOURCE_LENGTH(name) __resource__##name##_length

#define DECLARE_RESOURCE(name) \
    extern const char RESOURCE_DATA(name)[]; \
    extern const int RESOURCE_LENGTH(name); \
    std::string_view name { RESOURCE_DATA(name), size_t(RESOURCE_LENGTH(name)) };

static inline wxBitmap loadPNG(std::string_view resource) {
    return wxBitmap::NewFromPNGData(resource.data(), resource.size());
}

#endif