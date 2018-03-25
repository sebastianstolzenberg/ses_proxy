#include <string>

#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote.hpp"

#include "util/log.hpp"

namespace cryptonote
{
std::vector<uint8_t> convert_blob(const std::vector<uint8_t>& templateBlob)
{
  std::vector<uint8_t> result;

  blobdata input =
      std::string(reinterpret_cast<const char*>(templateBlob.data()), templateBlob.size());
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
        //TODO
//            return THROW_ERROR_EXCEPTION("Failed to create mining block");
    }
  }
  else
  {
      //TODO
//    return THROW_ERROR_EXCEPTION("Failed to parse block");
  }

//  LOG_INFO << __FUNCTION__
//           << " converted template of size " << templateBlob.size()
//           << " to miner block of size " << result.size();
  return result;
}

//std::vector<uint8_t> construct_block_blob(const std::vector<uint8_t>& templateBlob, uint32_t nonce)
//{
//    std::vector<uint8_t> result;
//
//    blobdata block_template_blob =
//        std::string(reinterpret_cast<const char*>(templateBlob.data()), templateBlob.size());
//    blobdata output = "";
//
//    block b = AUTO_VAL_INIT(b);
//    if (parse_and_validate_block_from_blob(block_template_blob, b))
//    {
//        b.nonce = nonce;
//        if (block_to_blob(b, output))
//        {
//            result = std::vector<uint8_t>(output.begin(), output.end());
//        }
//        else
//        {
//            LOG_DEBUG << "Failed to convert block to blob";
////            return THROW_ERROR_EXCEPTION("Failed to convert block to blob");
//        }
//    }
//    else
//    {
//        LOG_DEBUG << "Failed to parse block";
////        return THROW_ERROR_EXCEPTION("Failed to parse block");
//    }
//    return result;
//}

}