
#include "pdf_document.h"

#include <wx/image.h>

using namespace bls;

wxImage pdf_to_image(const pdf_document &doc, int page, int rotation = 0);