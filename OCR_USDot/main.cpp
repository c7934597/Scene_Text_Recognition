#include <string>

#include "Filter.h"
#include "OCR_USDot.h"

#include "LibraryTypes.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define CALLING_CONVENTION __cdecl
#else
#define EXPORT
#define CALLING_CONVENTION
#endif

extern "C"
{

    EXPORT bool CALLING_CONVENTION Initialize()
    {
        return true;
    }

    EXPORT void CALLING_CONVENTION Uninitialize()
    {
    }

    EXPORT LibraryType CALLING_CONVENTION GetLibType()
    {
        return LibraryType::Filter;
    }

    EXPORT const std::string CALLING_CONVENTION GetLibID()
    {
        return FilterType::kOCR_PlateRec;
    }

    EXPORT Filter *CALLING_CONVENTION CreateObject(const std::string &libID)
    {
        if (FilterType::kOCR_PlateRec == libID)
        {
            return new OCR_USDot;
        }
        else
        {
            return nullptr;
        }
    }

}
