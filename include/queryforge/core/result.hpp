#pragma once

#include <string>
#include <utility>
#include <variant>

namespace queryforge {

struct Error {
    std::string message;
};

template <typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value)); }
    static Result fail(std::string message) { return Result(Error{std::move(message)}); }

    bool has_value() const { return std::holds_alternative<T>(data_); }
    explicit operator bool() const { return has_value(); }

    const T& value() const& { return std::get<T>(data_); }
    T& value() & { return std::get<T>(data_); }
    T&& value() && { return std::get<T>(std::move(data_)); }

    const Error& error() const { return std::get<Error>(data_); }

private:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(Error error) : data_(std::move(error)) {}

    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    static Result ok() { return Result(true); }
    static Result fail(std::string message) { return Result(Error{std::move(message)}); }

    bool has_value() const { return ok_; }
    explicit operator bool() const { return ok_; }

    const Error& error() const { return error_; }

private:
    explicit Result(bool ok) : ok_(ok) {}
    explicit Result(Error error) : ok_(false), error_(std::move(error)) {}

    bool ok_ = false;
    Error error_;
};

}  // namespace queryforge
