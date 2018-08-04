#ifndef SQUAWKERRC_H
#define SQUAWKERRC_H

#include <system_error>

enum squawk_errc {
    UNKNOWN,
};

namespace std {
template<> struct is_error_condition_enum<squawk_errc> : public true_type {};
}//namespace std

class squawk_category_t : public std::error_category {
public:

    //virtual const char* name() const noexcept;
    virtual const char* name() const noexcept { return "av"; }
    //virtual std::error_condition default_error_condition ( int ev ) const noexcept;
    virtual std::error_condition default_error_condition ( int ev ) const noexcept {
//        if ( ev == static_cast< int > ( AV_BSF_NOT_FOUND ) )
//        {  }
        return std::error_condition ( UNKNOWN );
    }
    //virtual bool equivalent ( const std::error_code& code, int condition ) const noexcept;
    virtual bool equivalent ( const std::error_code& code, int condition ) const noexcept {
        return *this==code.category() &&
               static_cast< int > ( default_error_condition ( code.value() ).value() ) == condition;
    }
    //virtual std::string message ( int ev ) const;
    std::string message ( int ev ) const {
        return "Unknown Error.";
    }
} static squawk_category;

inline std::error_condition make_error_condition ( squawk_errc e ) {
    return std::error_condition ( static_cast<int> ( e ), squawk_category );
}
inline std::error_code make_error_code ( int error ) {
    switch ( error ) {
    case UNKNOWN:
        return std::error_code ( error, squawk_category );
    }

    return std::error_code ( abs ( error ), std::generic_category() );
}
#endif // SQUAWKERRC_H
