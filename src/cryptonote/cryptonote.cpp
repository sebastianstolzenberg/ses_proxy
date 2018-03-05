#include <string>

#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote.hpp"

namespace cryptonote
{
std::vector<uint8_t> convert_blob(const std::vector<uint8_t>& blob)
{
    std::vector<uint8_t> result;

    blobdata input = std::string(reinterpret_cast<const char*>(blob.data()), blob.size());
    blobdata output = "";

    //convert
    block b = AUTO_VAL_INIT(b);
    if (parse_and_validate_block_from_blob(input, b))
    {
        if (get_block_hashing_blob(b, output))
        {
            result = std::vector<uint8_t>(output.begin(), output.end());
        }
        else
        {
            std::cout << "Failed to create mining block" << std::endl;
//            return THROW_ERROR_EXCEPTION("Failed to create mining block");
        }
    }
    else
    {
        std::cout << "Failed to parse block" << std::endl;
//    return THROW_ERROR_EXCEPTION("Failed to parse block");
    }
    return result;
}
}