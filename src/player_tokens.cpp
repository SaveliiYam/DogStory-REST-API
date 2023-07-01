#include "player_tokens.h"
#include <sstream>

constexpr size_t default_token_size_ = 32;

std::string PlayerTokens::GetToken()
{
	auto hexConverter = [](const auto& value) -> std::string
			{
                 std::stringstream sstream;
                 sstream << std::hex << value;
                 return sstream.str();
			};

   std::string token;
   do {
         token = hexConverter(generator1_()) + hexConverter(generator2_());
   }while(token.size() != default_token_size_);

   return token;
}
