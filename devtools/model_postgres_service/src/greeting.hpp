#pragma once

#include <string>
#include <string_view>

namespace model_postgres_service {

enum class UserType { kFirstTime, kKnown };

std::string SayHelloTo(std::string_view name, UserType type);

}  // namespace model_postgres_service