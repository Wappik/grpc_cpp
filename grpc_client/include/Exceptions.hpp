#ifndef EXCEPTIONS_HPP
# define EXCEPTIONS_HPP
#include "Poco/Exception.h"

#include <sstream>

namespace exceptions {

    static std::string displayText(const Poco::Exception& ex)
    {
        std::stringstream ss;
        ss << ex.displayText();

        const Poco::Exception* nested = ex.nested();
        while (nested != nullptr)
        {
            ss << " : " << nested->displayText();
            nested = nested->nested();
        }
        return ss.str();
    }
//POCO_DECLARE_EXCEPTION(, Parse, Poco::Exception)

}

#endif