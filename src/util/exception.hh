/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
// 常用异常处理工具

#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include <system_error>
#include <iostream>
#include <typeinfo>

#include <cxxabi.h>

class tagged_error : public std::system_error
{
private:
    std::string attempt_and_error_;

public:
    tagged_error( const std::error_category & category,
                  const std::string & s_attempt,
                  const int error_code )
        : system_error( error_code, category ),
          attempt_and_error_( s_attempt + ": " + std::system_error::what() )
    {}

    const char * what( void ) const noexcept override
    {
        return attempt_and_error_.c_str();
    }
};

class unix_error : public tagged_error
{
public:
    unix_error( const std::string & s_attempt,
                const int s_errno = errno )
        : tagged_error( std::system_category(), s_attempt, s_errno )
    {}
};

inline void print_exception( const std::exception & e, std::ostream & output = std::cerr )
{
    output << "Died on " << abi::__cxa_demangle( typeid( e ).name(), nullptr, nullptr, nullptr ) << ": " << e.what() << std::endl;
}

/* error-checking wrapper for most syscalls 针对大多数系统调用时的 《封装出错核查》*/
// inline 表示是内联函数，就是在类内部展开，调用时没有入栈和出栈的过程，比较便捷
// SystemCall检查return_value是否大于零
inline int SystemCall( const std::string & s_attempt, const int return_value )
{
  if ( return_value >= 0 ) { // 看似检查是否返回值大于0，其实只是检查函数调用是否正确
    return return_value;
  }

  throw unix_error( s_attempt );
}

#endif
