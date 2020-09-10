#pragma once

#include <iostream> 
#include <cstring>
#include <sstream> 

class OsError : public std::runtime_error {
    int _errorCode;

    static const std::string GetMessage(const char* opName, int errorCode) {
        std::stringstream ss;
        ss << opName << " failed! return val / errno: " << errorCode;
        return ss.str();
    }

public:
    OsError(const char* opName, int errorCode)
        : std::runtime_error(GetMessage(opName, errorCode)), _errorCode(errorCode)
    {
    }

    const int ErrorCode() const {
        return _errorCode;
    }
};

void PressEnter2(const char* opName) {
    std::cout << "Press Enter to " << opName << '!' << std::endl;
    char dummy[128];
    std::cin.getline(dummy, 128);
}

template<typename F>
auto TryStuff(const char* opName, F&& lambda) -> decltype(lambda()) {
    auto result = lambda();
    if (result < 0) {
        int err = errno;
        throw OsError(opName, err);
    }
    return result;
}

template<typename F>
auto TryStuffExpectNegativeErrorCode(const char* opName, F&& lambda) -> decltype(lambda()) {
    auto result = lambda();
    if (result < 0) {
        throw OsError(opName, -result);
    }
    return result;
}

template<typename F>
auto TryStuffExpectNonZero(const char* opName, F&& lambda) -> decltype(lambda()) {
    auto result = lambda();
    if (!result) {
        throw OsError(opName, GetLastError());
    }
    return result;
}

template<typename F>
void TryStuffExpectZero(const char* opName, F&& lambda) {
    int err = lambda();

    if (err != 0) {
        throw OsError(opName, err);
    }
}

#define TRY( operation ) TryStuff( #operation , [&]() { return operation; })
#define TRYNE( operation ) TryStuffExpectNegativeErrorCode( #operation , [&]() { return operation; })
#define TRYZ( operation ) TryStuffExpectZero( #operation, [&]() { return operation; })
#define TRYNZ( operation ) TryStuffExpectNonZero( #operation , [&]() { return operation; })

template<typename T, typename C>
void _AppendFlagIfSet(std::basic_ostream<char>& ss, const T& flagSet, const C& checkFlag, const char* flagName) {
    if ((flagSet & checkFlag) == checkFlag) {
        ss << " | " << flagName;
    }
}

#define APPEND_FLAG( ss, flagSet, checkFlag ) _AppendFlagIfSet(ss, flagSet, checkFlag, #checkFlag )

template <typename TUnaryFunction>
void ThrowRuntimeError(TUnaryFunction&& f) {
    std::stringstream ss;
    f(ss);
    throw std::runtime_error(ss.str());
}

void Assert(bool condition, const char* errorMsg) {
    if (!condition) throw std::runtime_error(errorMsg);
}

#define ASSERT( conditionExpr ) Assert(conditionExpr, #conditionExpr );
