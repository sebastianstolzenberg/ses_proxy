#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/limits.hpp>

#include "util/difficulty.hpp"
#include "util/hex.hpp"
#include "util/log.hpp"

#if BOOST_VERSION < 106000
namespace boost {
namespace multiprecision {
namespace detail
{
template <class Backend, class Unsigned>
void assign_bits(Backend& val, Unsigned bits, unsigned bit_location, unsigned chunk_bits, const mpl::false_& tag)
{
  unsigned limb = bit_location / (sizeof(limb_type) * CHAR_BIT);
  unsigned shift = bit_location % (sizeof(limb_type) * CHAR_BIT);

  limb_type mask = chunk_bits >= sizeof(limb_type) * CHAR_BIT ? ~static_cast<limb_type>(0u) : (static_cast<limb_type>(1u) << chunk_bits) - 1;

  limb_type value = (static_cast<limb_type>(bits) & mask) << shift;
  if(value)
  {
    if(val.size() == limb)
    {
      val.resize(limb + 1, limb + 1);
      if(val.size() > limb)
        val.limbs()[limb] = value;
    }
    else if(val.size() > limb)
      val.limbs()[limb] |= value;
  }
  if(chunk_bits > sizeof(limb_type) * CHAR_BIT - shift)
  {
    shift = sizeof(limb_type) * CHAR_BIT - shift;
    chunk_bits -= shift;
    bit_location += shift;
    bits >>= shift;
    if(bits)
      assign_bits(val, bits, bit_location, chunk_bits, tag);
  }
}
template <class Backend, class Unsigned>
void assign_bits(Backend& val, Unsigned bits, unsigned bit_location, unsigned chunk_bits, const mpl::true_&)
{
  typedef typename Backend::local_limb_type local_limb_type;
  //
  // Check for possible overflow, this may trigger an exception, or have no effect
  // depending on whether this is a checked integer or not:
  //
  if((bit_location >= sizeof(local_limb_type) * CHAR_BIT) && bits)
    val.resize(2, 2);
  else
  {
    local_limb_type mask = chunk_bits >= sizeof(local_limb_type) * CHAR_BIT ? ~static_cast<local_limb_type>(0u) : (static_cast<local_limb_type>(1u) << chunk_bits) - 1;
    local_limb_type value = (static_cast<local_limb_type>(bits) & mask) << bit_location;
    *val.limbs() |= value;
    //
    // Check for overflow bits:
    //
    bit_location = sizeof(local_limb_type) * CHAR_BIT - bit_location;
    bits >>= bit_location;
    if(bits)
      val.resize(2, 2); // May throw!
  }
}
template <unsigned MinBits, unsigned MaxBits, cpp_integer_type SignType, cpp_int_check_type Checked, class Allocator>
inline void resize_to_bit_size(cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>& newval, unsigned bits, const mpl::false_&)
{
  unsigned limb_count = static_cast<unsigned>(bits / (sizeof(limb_type) * CHAR_BIT));
  if(bits % (sizeof(limb_type) * CHAR_BIT))
    ++limb_count;
  static const unsigned max_limbs = MaxBits ? MaxBits / (CHAR_BIT * sizeof(limb_type)) + ((MaxBits % (CHAR_BIT * sizeof(limb_type))) ? 1 : 0) : (std::numeric_limits<unsigned>::max)();
  if(limb_count > max_limbs)
    limb_count = max_limbs;
  newval.resize(limb_count, limb_count);
  std::memset(newval.limbs(), 0, newval.size() * sizeof(limb_type));
}
template <unsigned MinBits, unsigned MaxBits, cpp_integer_type SignType, cpp_int_check_type Checked, class Allocator>
inline void resize_to_bit_size(cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>& newval, unsigned, const mpl::true_&)
{
  *newval.limbs() = 0;
}
}
template <unsigned MinBits, unsigned MaxBits, cpp_integer_type SignType, cpp_int_check_type Checked, class Allocator, expression_template_option ExpressionTemplates, class Iterator>
number<cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>, ExpressionTemplates>&
import_bits(
    number<cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>, ExpressionTemplates>& val, Iterator i, Iterator j, unsigned chunk_size = 0, bool msv_first = true)
{
  typename number<cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>, ExpressionTemplates>::backend_type newval;

  typedef typename std::iterator_traits<Iterator>::value_type       value_type;
  typedef typename boost::make_unsigned<value_type>::type           unsigned_value_type;
  typedef typename std::iterator_traits<Iterator>::difference_type  difference_type;
  typedef typename boost::make_unsigned<difference_type>::type      size_type;
  typedef typename cpp_int_backend<MinBits, MaxBits, SignType, Checked, Allocator>::trivial_tag tag_type;

  if(!chunk_size)
    chunk_size = std::numeric_limits<value_type>::digits;

  size_type limbs = std::distance(i, j);
  size_type bits = limbs * chunk_size;

  detail::resize_to_bit_size(newval, static_cast<unsigned>(bits), tag_type());

  difference_type bit_location = msv_first ? bits - chunk_size : 0;
  difference_type bit_location_change = msv_first ? -static_cast<difference_type>(chunk_size) : chunk_size;

  while(i != j)
  {
    detail::assign_bits(newval, static_cast<unsigned_value_type>(*i), static_cast<unsigned>(bit_location), chunk_size, tag_type());
    ++i;
    bit_location += bit_location_change;
  }

  newval.normalize();

  val.backend().swap(newval);
  return val;
}
}
}
#endif // BOOST_VERSION < 106000

namespace ses {
namespace util {

namespace
{
inline std::string toHex(const boost::multiprecision::uint256_t& t)
{
  return ses::util::toHex<boost::multiprecision::limb_type>(t.backend().limbs(),
                                                            t.backend().size());
}

boost::multiprecision::uint256_t uint256FromTarget(uint64_t target)
{
  boost::multiprecision::uint256_t
      bigNum = std::numeric_limits<boost::multiprecision::uint256_t>::max();
  bigNum = std::numeric_limits<boost::multiprecision::uint256_t>::max();
  uint8_t* limb = reinterpret_cast<uint8_t*>(bigNum.backend().limbs());
  size_t limbSize = sizeof(boost::multiprecision::limb_type) * bigNum.backend().size();
  uint8_t* targetInLimb = limb + limbSize - sizeof(target);
  memcpy(targetInLimb, &target, sizeof(target));
  memset(limb, 0, limbSize - sizeof(target));
  return bigNum;
}

uint64_t extractTargetFromUint256(const boost::multiprecision::uint256_t& bigNum)
{
  uint64_t target;
  const uint8_t* limb = reinterpret_cast<const uint8_t*>(bigNum.backend().limbs());
  size_t limbSize = sizeof(boost::multiprecision::limb_type) * bigNum.backend().size();
  const uint8_t* targetInLimb = limb + limbSize - sizeof(target);
  memcpy(&target, targetInLimb, sizeof(target));
  return target;
}
}

uint32_t targetToDifficulty(const Target& target)
{
  return (std::numeric_limits<boost::multiprecision::uint256_t>::max() /
          uint256FromTarget(target.getRaw())).convert_to<uint32_t>();
}

Target difficultyToTarget(uint32_t difficulty)
{
  return Target(extractTargetFromUint256(std::numeric_limits<boost::multiprecision::uint256_t>::max() / difficulty));
}

uint32_t difficultyFromHashBuffer(const uint8_t* data, size_t size)
{
  boost::multiprecision::uint256_t base =
    std::numeric_limits<boost::multiprecision::uint256_t>::max();
  boost::multiprecision::uint256_t hash;
  boost::multiprecision::import_bits(hash, data, data + size, 0, false);
//  LOG_DEBUG << "targetToDifficulty(), hash, " << hash;
//  LOG_DEBUG << "targetToDifficulty(), hex(hash), " << toHex(hash);
  base /= hash;
//  LOG_DEBUG << "targetToDifficulty(), base/hash, " << base;
//  LOG_DEBUG << "targetToDifficulty(), hex(base/hash), " << toHex(base);
  return base.convert_to<uint32_t>();
}

} // namespace util
} // namespace ses
