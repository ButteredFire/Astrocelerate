/* StringUtils - Utilities pertaining to string manipulation, comparison, etc..
*/

#pragma once

#include <string>
#include <vector>
#include <random>
#include <chrono>


namespace StringUtils {
    /* Generates a random alphanumeric string of the specified length.
       @param length: The length of the random string to be generated.

       @return: A random alphanumeric string.
    */
    inline std::string RandomString(size_t length) {
        const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string randomString;
        std::mt19937 generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<> distribution(0, characters.length() - 1);

        for (size_t i = 0; i < length; ++i) {
            randomString += characters[distribution(generator)];
        }
        return randomString;
    }


    /* Does a string begin with a substring? */
    inline bool BeginsWith(const std::string &srcString, const std::string &substr) {
        return (srcString.compare(0, substr.length(), substr) == 0);
    }


    /* Does a string end with a substring? */
    inline bool EndsWith(const std::string &srcString, const std::string &substr) {
        return srcString.ends_with(substr);
    }
}